#include "storage/DocumentStorage.h"
#include "storage/EntityCodec.h"
#include "blocks/BlockEntity.h"
#include <QFile>
#include <QSaveFile>
#include <QSet>
namespace qcad_more {
namespace {
constexpr quint32 Magic = 0x51424c4b; // QBLK
constexpr quint32 Version = 1;
constexpr quint32 MaxRecords = 100000;
constexpr qint64 MaxFileBytes = 128 * 1024 * 1024;
OperationResult invalid() { return {ErrorCode::InvalidFormat, QStringLiteral("文件格式错误、版本不支持或块引用不完整。")}; }
void writeSnapshot(QDataStream& stream, const EntitySnapshot& snapshot)
{ stream << qint32(snapshot.entityType) << snapshot.payload; }
EntitySnapshot readSnapshot(QDataStream& stream)
{ EntitySnapshot snapshot; qint32 type = 0; stream >> type >> snapshot.payload; snapshot.entityType = type; return snapshot; }
bool readCount(QDataStream& stream, quint32& count)
{ stream >> count; return stream.status() == QDataStream::Ok && count <= MaxRecords; }
OperationResult validateLibrary(const BlockLibrary& library)
{
    QSet<QUuid> ids;
    QSet<QString> names;
    if (library.definitions.size() > int(MaxRecords)) return invalid();
    for (const auto& block : library.definitions) {
        if (block.id.isNull() || ids.contains(block.id) || block.name.trimmed().isEmpty() ||
            names.contains(block.name) || !finitePoint(block.sourceBasePoint) ||
            block.entities.isEmpty() || block.entities.size() > int(MaxRecords)) return invalid();
        ids.insert(block.id); names.insert(block.name);
        for (const auto& snapshot : block.entities) {
            std::unique_ptr<MEntity> entity;
            auto result = decodeEntity(snapshot, entity);
            if (!result.ok()) return result;
        }
    }
    return {};
}
}
OperationResult DocumentStorage::save(const QString& path, QCADView& view, const BlockLibrary& library) const
{
    if (path.isEmpty()) return {ErrorCode::InvalidInput, QStringLiteral("保存路径为空。")};
    auto result = validateLibrary(library);
    if (!result.ok()) return result;
    const auto entities = view.GetEntityList();
    if (entities.size() > int(MaxRecords)) return invalid();
    QByteArray bytes;
    QDataStream out(&bytes, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_5_12);
    out << Magic << Version << quint32(library.definitions.size());
    for (const auto& block : library.definitions) {
        out << block.id << block.name << block.sourceBasePoint << quint32(block.entities.size());
        for (const auto& snapshot : block.entities) writeSnapshot(out, snapshot);
    }
    out << quint32(entities.size());
    QSet<QUuid> instanceIds;
    for (auto* entity : entities) {
        out << qint32(entity->GetType());
        if (auto* block = dynamic_cast<BlockEntity*>(entity)) {
            const auto& instance = block->instance;
            if (instance.instanceId.isNull() || instanceIds.contains(instance.instanceId) ||
                !findBlock(library, instance.definitionId) || !finitePoint(instance.position)) return invalid();
            instanceIds.insert(instance.instanceId);
            out << instance.instanceId << instance.definitionId << instance.position;
        } else {
            EntitySnapshot snapshot;
            result = encodeEntity(*entity, snapshot);
            if (!result.ok()) return result;
            out << snapshot.payload;
        }
    }
    if (out.status() != QDataStream::Ok || bytes.size() > MaxFileBytes)
        return {ErrorCode::IoError, QStringLiteral("文档编码失败或超过 128 MiB。")};
    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly) || file.write(bytes) != bytes.size() || !file.commit())
        return {ErrorCode::IoError, QStringLiteral("保存失败：") + file.errorString()};
    return {};
}
OperationResult DocumentStorage::load(const QString& path, QCADView& view, BlockLibrary& library) const
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) return {ErrorCode::IoError, file.errorString()};
    if (file.size() > MaxFileBytes) return invalid();
    QDataStream in(&file); in.setVersion(QDataStream::Qt_5_12);
    quint32 magic = 0, version = 0, count = 0;
    in >> magic >> version;
    if (magic != Magic || version != Version || !readCount(in, count)) return invalid();
    BlockLibrary loaded;
    for (quint32 i = 0; i < count; ++i) {
        BlockDefinition block; quint32 entityCount = 0;
        in >> block.id >> block.name >> block.sourceBasePoint;
        if (!readCount(in, entityCount)) return invalid();
        for (quint32 j = 0; j < entityCount; ++j) {
            block.entities.push_back(readSnapshot(in));
            if (in.status() != QDataStream::Ok) return invalid();
        }
        loaded.definitions.push_back(std::move(block));
    }
    auto result = validateLibrary(loaded);
    if (!result.ok()) return result;
    if (!readCount(in, count)) return invalid();
    std::vector<std::unique_ptr<MEntity>> entities;
    QSet<QUuid> instanceIds;
    for (quint32 i = 0; i < count; ++i) {
        qint32 type = 0; in >> type;
        if (type == etBlock) {
            BlockInstance instance;
            in >> instance.instanceId >> instance.definitionId >> instance.position;
            const auto* definition = findBlock(loaded, instance.definitionId);
            if (!definition || instance.instanceId.isNull() || instanceIds.contains(instance.instanceId)) return invalid();
            instanceIds.insert(instance.instanceId);
            std::unique_ptr<BlockEntity> block;
            result = makeBlockEntity(*definition, instance, block);
            if (!result.ok()) return result;
            entities.push_back(std::move(block));
        } else {
            EntitySnapshot snapshot; snapshot.entityType = type; in >> snapshot.payload;
            std::unique_ptr<MEntity> entity;
            result = decodeEntity(snapshot, entity);
            if (!result.ok()) return result;
            entities.push_back(std::move(entity));
        }
        if (in.status() != QDataStream::Ok) return invalid();
    }
    if (in.status() != QDataStream::Ok || !in.atEnd()) return invalid();
    // Nothing before this point changes the current document.
    QList<MEntity*> transferred;
    for (const auto& entity : entities) transferred.push_back(entity.get());
    view.replaceEntities(transferred);
    for (auto& entity : entities) entity.release();
    library = std::move(loaded);
    view.setModified(false);
    return {};
}
}

#include "storage/EntityCodec.h"
#include <QBuffer>
#include <cmath>
#include "blocks/BlockEntity.h"
namespace qcad_more {
bool finitePoint(const QPointF& p) { return std::isfinite(p.x()) && std::isfinite(p.y()); }
namespace {
thread_local int codecDepth = 0;
struct DepthGuard { DepthGuard() { ++codecDepth; } ~DepthGuard() { --codecDepth; } };
std::unique_ptr<MEntity> makeEntity(int type)
{
    switch (type) {
    case etLine: return std::make_unique<MLine>();
    case etRectangle: return std::make_unique<MRectangle>();
    case etCircle: return std::make_unique<CCircle>();
    case etArc: return std::make_unique<CArc>();
    case etEllipse: return std::make_unique<MEllipse>();
    case etPolygon: return std::make_unique<MPolygon>();
    case etText: return std::make_unique<MText>();
    case etBlock: return std::make_unique<BlockEntity>();
    default: return {};
    }
}
}
OperationResult encodeEntity(MEntity& entity, EntitySnapshot& output)
{
    DepthGuard depth;
    if (codecDepth > 33) return {ErrorCode::InvalidInput, QStringLiteral("嵌套块层数不能超过 32 层。")};
    if (!makeEntity(entity.GetType()))
        return {ErrorCode::UnsupportedEntity, QStringLiteral("该图元暂不支持。")};
    EntitySnapshot snapshot;
    snapshot.entityType = entity.GetType();
    QDataStream stream(&snapshot.payload, QIODevice::WriteOnly);
    stream.setVersion(QDataStream::Qt_5_12);
    entity.Serialize(stream, true);
    if (stream.status() != QDataStream::Ok)
        return {ErrorCode::IoError, QStringLiteral("图元编码失败。")};
    output = std::move(snapshot);
    return {};
}
OperationResult decodeEntity(const EntitySnapshot& snapshot, std::unique_ptr<MEntity>& output)
{
    DepthGuard depth;
    if (codecDepth > 33) return {ErrorCode::InvalidFormat, QStringLiteral("嵌套块层数超过限制。")};
    auto entity = makeEntity(snapshot.entityType);
    if (!entity) return {ErrorCode::UnsupportedEntity, QStringLiteral("文件包含不支持的图元。")};
    QDataStream stream(snapshot.payload);
    stream.setVersion(QDataStream::Qt_5_12);
    entity->Serialize(stream, false);
    QRectF box;
    entity->GetBox(box);
    if (stream.status() != QDataStream::Ok || !stream.atEnd() ||
        !finitePoint(box.topLeft()) || !finitePoint(box.bottomRight()))
        return {ErrorCode::InvalidFormat, QStringLiteral("图元数据损坏或不完整。")};
    output = std::move(entity);
    return {};
}
}

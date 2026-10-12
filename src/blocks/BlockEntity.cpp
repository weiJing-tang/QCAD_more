#include "blocks/BlockEntity.h"
#include "storage/EntityCodec.h"
namespace qcad_more {
const BlockDefinition* findBlock(const BlockLibrary& library, const QUuid& id)
{
    for (const auto& block : library.definitions) if (block.id == id) return &block;
    return nullptr;
}
OperationResult makeBlockEntity(const BlockDefinition& definition, const BlockInstance& instance,
                                std::unique_ptr<BlockEntity>& output)
{
    if (definition.entities.isEmpty() || definition.id != instance.definitionId ||
        !finitePoint(instance.position) || !std::isfinite(instance.rotationDegrees) ||
        !std::isfinite(instance.scale) || instance.scale <= 0 || instance.scale > 10000)
        return {ErrorCode::InvalidInput, QStringLiteral("块定义或插入位置无效。")};
    auto block = std::make_unique<BlockEntity>();
    block->instance = instance;
    for (const auto& snapshot : definition.entities) {
        std::unique_ptr<MEntity> entity;
        auto result = decodeEntity(snapshot, entity);
        if (!result.ok()) return result;
        entity->Move(QPointF(), instance.position);
        block->children.push_back(std::move(entity));
    }
    output = std::move(block);
    return {};
}
MEntity* BlockEntity::Copy()
{
    auto copy = std::make_unique<BlockEntity>();
    copy->instance = instance;
    copy->instance.instanceId = QUuid::createUuid();
    for (const auto& child : children) copy->children.emplace_back(child->Copy());
    return copy.release();
}
void BlockEntity::Draw(QCADView* view, int mode)
{
    const auto previous = view->drawingTransform;
    const auto origin = view->WorldtoScreen(QPointF());
    QTransform worldToScreen(view->GetScale(), 0, 0, -view->GetScale(), origin.x(), origin.y());
    // Conjugate the world transform into screen space; retain any parent block transform.
    view->drawingTransform = worldToScreen.inverted() * worldTransform() * worldToScreen * previous;
    for (const auto& child : children) child->Draw(view, mode);
    view->drawingTransform = previous;
}
bool BlockEntity::Pick(const QPointF& point, double radius)
{
    QRectF box; GetBox(box);
    return box.adjusted(-radius, -radius, radius, radius).contains(point);
}
void BlockEntity::GetBox(QRectF& box)
{
    if (children.empty()) { box = {}; return; }
    children.front()->GetBox(box);
    box = box.normalized();
    for (size_t i = 1; i < children.size(); ++i) {
        QRectF next; children[i]->GetBox(next); next = next.normalized();
        box = QRectF(QPointF(qMin(box.left(), next.left()), qMin(box.top(), next.top())),
                     QPointF(qMax(box.right(), next.right()), qMax(box.bottom(), next.bottom())));
    }
    box = worldTransform().mapRect(box);
}
void BlockEntity::Move(const QPointF& from, const QPointF& to, bool temporary)
{
    for (const auto& child : children) child->Move(from, to, temporary);
    instance.position += to - from;
}
QTransform BlockEntity::worldTransform() const
{
    QTransform transform;
    transform.translate(instance.position.x(), instance.position.y());
    transform.rotate(instance.rotationDegrees);
    transform.scale(instance.scale, instance.scale);
    transform.translate(-instance.position.x(), -instance.position.y());
    return transform;
}
void BlockEntity::collectTexts(QVector<MText*>& texts, QStringList& labels, const QString& prefix)
{
    for (size_t i = 0; i < children.size(); ++i) {
        const auto path = prefix + QString::number(i + 1);
        if (auto* text = dynamic_cast<MText*>(children[i].get())) {
            texts.push_back(text); labels.push_back(path + QStringLiteral(": ") + text->GetText().left(40));
        } else if (auto* block = dynamic_cast<BlockEntity*>(children[i].get())) {
            block->collectTexts(texts, labels, path + "/");
        }
    }
}
void BlockEntity::Serialize(QDataStream& stream, bool save)
{
    MEntity::Serialize(stream, save);
    if (save) {
        stream << instance.instanceId << instance.definitionId << instance.position
               << instance.rotationDegrees << instance.scale << quint32(children.size());
        for (const auto& child : children) {
            EntitySnapshot snapshot;
            if (!encodeEntity(*child, snapshot).ok()) { stream.setStatus(QDataStream::WriteFailed); return; }
            stream << qint32(snapshot.entityType) << snapshot.payload;
        }
    } else {
        quint32 count = 0;
        stream >> instance.instanceId >> instance.definitionId >> instance.position
               >> instance.rotationDegrees >> instance.scale >> count;
        if (count == 0 || count > 100000 || instance.instanceId.isNull() || instance.definitionId.isNull() ||
            !finitePoint(instance.position) || !std::isfinite(instance.rotationDegrees) ||
            !std::isfinite(instance.scale) || instance.scale <= 0 || instance.scale > 10000) {
            stream.setStatus(QDataStream::ReadCorruptData); return;
        }
        children.clear();
        for (quint32 i = 0; i < count; ++i) {
            EntitySnapshot snapshot; qint32 type = 0;
            stream >> type >> snapshot.payload; snapshot.entityType = type;
            std::unique_ptr<MEntity> child;
            if (stream.status() != QDataStream::Ok || !decodeEntity(snapshot, child).ok()) {
                stream.setStatus(QDataStream::ReadCorruptData); return;
            }
            children.push_back(std::move(child));
        }
    }
}
}

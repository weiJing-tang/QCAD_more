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
        !finitePoint(instance.position))
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
    for (const auto& child : children) child->Draw(view, mode);
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
    for (const auto& child : children) {
        QRectF next; child->GetBox(next); next = next.normalized();
        box = QRectF(QPointF(qMin(box.left(), next.left()), qMin(box.top(), next.top())),
                     QPointF(qMax(box.right(), next.right()), qMax(box.bottom(), next.bottom())));
    }
}
void BlockEntity::Move(const QPointF& from, const QPointF& to, bool temporary)
{
    for (const auto& child : children) child->Move(from, to, temporary);
    instance.position += to - from;
}
}

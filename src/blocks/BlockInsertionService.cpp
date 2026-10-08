#include "blocks/BlockInsertionService.h"
#include "blocks/BlockEntity.h"
namespace qcad_more {
namespace {
OperationResult prepare(const BlockLibrary& library, const InsertBlockRequest& request,
                        std::unique_ptr<BlockEntity>& output)
{
    const auto* definition = findBlock(library, request.definitionId);
    if (!definition) return {ErrorCode::BlockNotFound, QStringLiteral("找不到所选块定义。")};
    return makeBlockEntity(*definition, {QUuid::createUuid(), request.definitionId, request.position}, output);
}
}
OperationResult BlockInsertionService::previewAt(QCADView& view, const BlockLibrary& library,
                                                 const InsertBlockRequest& request)
{
    std::unique_ptr<BlockEntity> block;
    auto result = prepare(library, request, block);
    if (result.ok()) view.setPreview(std::move(block));
    return result;
}
OperationResult BlockInsertionService::insertAt(QCADView& view, const BlockLibrary& library,
                                                const InsertBlockRequest& request, QUuid& instanceId)
{
    std::unique_ptr<BlockEntity> block;
    auto result = prepare(library, request, block);
    if (!result.ok()) return result;
    const auto id = block->instance.instanceId;
    view.addEntity(block.release());
    view.setPreview(nullptr);
    instanceId = id;
    return {};
}
OperationResult BlockInsertionService::cancelPreview(QCADView& view)
{
    view.setPreview(nullptr);
    return {};
}
}

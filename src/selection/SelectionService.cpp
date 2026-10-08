#include "selection/SelectionService.h"
#include "qcad/ENTITY.H"
#include "storage/EntityCodec.h"
namespace qcad_more {
OperationResult SelectionService::captureSelected(QCADView& view, QList<MEntity*>& output) const
{
    const auto selected = view.GetSelectedEntityList();
    if (selected.isEmpty()) return {ErrorCode::EmptySelection, QStringLiteral("请先选择图形。")};
    output = selected;
    return {};
}
OperationResult SelectionService::selectInRectangle(QCADView& view, const QRectF& worldRect,
                                                     SelectionMode mode) const
{
    if (!finitePoint(worldRect.topLeft()) || !finitePoint(worldRect.bottomRight()))
        return {ErrorCode::InvalidInput, QStringLiteral("选择框坐标无效。")};
    const auto rect = worldRect.normalized();
    if (mode == SelectionMode::Replace) view.ClearSelections();
    for (auto* entity : view.GetEntityList()) {
        QRectF box;
        entity->GetBox(box);
        box = box.normalized();
        // Point containment also handles zero-height horizontal/vertical lines.
        if (rect.contains(box.topLeft()) && rect.contains(box.bottomRight()))
            view.AddSelection(entity);
    }
    view.update();
    return {};
}
}

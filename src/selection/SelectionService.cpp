#include "selection/SelectionService.h"

namespace qcad_more {

OperationResult SelectionService::captureSelected(QCADView&, QList<MEntity*>&) const
{
    // TODO: S1 - read QCADView::GetSelectedEntityList(), reject an empty selection.
    return OperationResult::notImplemented("SelectionService::captureSelected");
}

OperationResult SelectionService::selectInRectangle(QCADView&, const QRectF&,
                                                    SelectionMode) const
{
    // TODO: S1 - reuse MEntity::GetBox and QCADView selection methods.
    return OperationResult::notImplemented("SelectionService::selectInRectangle");
}

} // namespace qcad_more

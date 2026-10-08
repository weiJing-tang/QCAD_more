#include "blocks/BlockInsertionService.h"

namespace qcad_more {

OperationResult BlockInsertionService::previewAt(QCADView&, const BlockLibrary&,
                                                 const InsertBlockRequest&)
{
    // TODO: I1 - resolve definition and draw temporary geometry at world position.
    return OperationResult::notImplemented("BlockInsertionService::previewAt");
}

OperationResult BlockInsertionService::insertAt(QCADView&, const BlockLibrary&,
                                                const InsertBlockRequest&, QUuid&)
{
    // TODO: I1 - create a block instance with the existing entity/command lifecycle.
    return OperationResult::notImplemented("BlockInsertionService::insertAt");
}

OperationResult BlockInsertionService::cancelPreview(QCADView&)
{
    // TODO: I1 - release transient preview state and refresh the view.
    return OperationResult::notImplemented("BlockInsertionService::cancelPreview");
}

} // namespace qcad_more

#pragma once

#include "blocks/BlockTypes.h"
#include "common/OperationResult.h"

class QCADView;

namespace qcad_more {

// This service is stateless. QCADView owns a separate preview entity;
// temporary geometry is never appended to the document entity list.
class BlockInsertionService {
public:
    OperationResult previewAt(QCADView& view, const BlockLibrary& library,
                              const InsertBlockRequest& request);

    // Insert one block entity and return its ID, then clear preview on success.
    // Failure leaves the document and output unchanged; definition is immutable.
    OperationResult insertAt(QCADView& view, const BlockLibrary& library,
                             const InsertBlockRequest& request, QUuid& instanceId);

    // Esc/right-click cancels preview only; committed instances remain untouched.
    OperationResult cancelPreview(QCADView& view);
};

} // namespace qcad_more

#pragma once

#include "blocks/BlockTypes.h"
#include "common/OperationResult.h"

class QCADView;

namespace qcad_more {

// One instance per active insertion command. Preview state belongs here when
// implemented; rendering must not append temporary entities to the document.
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

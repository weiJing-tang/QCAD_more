#pragma once

#include "blocks/BlockTypes.h"
#include "common/OperationResult.h"

class QCADView;

namespace qcad_more {

class DocumentStorage {
public:
    // Save ordinary entities, block definitions, instances and text properties.
    // Save must never mutate live entities. Use atomic replacement on success.
    OperationResult save(const QString& path, QCADView& view,
                         const BlockLibrary& library) const;

    // Read and validate into temporary state before replacing the whole document.
    // Failure preserves the current drawing and library. Clear stale selection
    // and previews only when committing a successfully loaded document.
    OperationResult load(const QString& path, QCADView& view,
                         BlockLibrary& library) const;
};

} // namespace qcad_more

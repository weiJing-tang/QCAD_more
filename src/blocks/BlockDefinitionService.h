#pragma once

#include "blocks/BlockTypes.h"
#include "common/OperationResult.h"
#include <QList>

class MEntity;

namespace qcad_more {

class BlockDefinitionService {
public:
    // Copies selected geometry into local coordinates without changing originals.
    // Reject empty selections, blank/duplicate names and unsupported entity types.
    // Publish the definition and output ID only after every snapshot succeeds.
    OperationResult defineBlock(const QList<MEntity*>& selected,
                                const DefineBlockRequest& request,
                                BlockLibrary& library, QUuid& definitionId) const;
};

} // namespace qcad_more

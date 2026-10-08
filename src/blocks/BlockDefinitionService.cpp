#include "blocks/BlockDefinitionService.h"

namespace qcad_more {

OperationResult BlockDefinitionService::defineBlock(const QList<MEntity*>&,
                                                   const DefineBlockRequest&,
                                                   BlockLibrary&, QUuid&) const
{
    // TODO: B1 - validate, copy geometry, offset by base point, store value snapshots.
    return OperationResult::notImplemented("BlockDefinitionService::defineBlock");
}

} // namespace qcad_more

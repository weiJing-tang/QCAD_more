#include "storage/DocumentStorage.h"

namespace qcad_more {

OperationResult DocumentStorage::save(const QString&, QCADView&,
                                      const BlockLibrary&) const
{
    // TODO: P1 - versioned QDataStream format + QSaveFile, after fixing legacy codec.
    // Do not open/truncate any file in this scaffold.
    return OperationResult::notImplemented("DocumentStorage::save");
}

OperationResult DocumentStorage::load(const QString&, QCADView&, BlockLibrary&) const
{
    // TODO: P1 - restore definitions first, then entities and block references.
    return OperationResult::notImplemented("DocumentStorage::load");
}

} // namespace qcad_more

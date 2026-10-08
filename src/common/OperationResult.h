#pragma once
#include <QString>
namespace qcad_more {
enum class ErrorCode { None, Cancelled, InvalidInput, EmptySelection, DuplicateBlockName,
                       BlockNotFound, UnsupportedEntity, IoError, InvalidFormat };
struct OperationResult {
    ErrorCode code = ErrorCode::None;
    QString message;
    bool ok() const { return code == ErrorCode::None; }
};
}

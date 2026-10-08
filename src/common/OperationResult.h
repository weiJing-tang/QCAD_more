#pragma once

#include <QString>

namespace qcad_more {

enum class ErrorCode {
    None,
    NotImplemented,
    Cancelled,
    InvalidInput,
    EmptySelection,
    DuplicateBlockName,
    BlockNotFound,
    UnsupportedEntity,
    IoError,
    InvalidFormat
};

struct OperationResult {
    ErrorCode code = ErrorCode::NotImplemented;
    QString message;

    bool ok() const { return code == ErrorCode::None; }

    static OperationResult notImplemented(const char* operation)
    {
        return {ErrorCode::NotImplemented,
                QString::fromLatin1(operation) + QStringLiteral(": not implemented")};
    }
};

} // namespace qcad_more

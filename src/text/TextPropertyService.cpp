#include "text/TextPropertyService.h"

namespace qcad_more {

OperationResult TextPropertyService::read(MText&, TextProperties&) const
{
    // TODO: T1 - add MText::GetFont() upstream, then read all supported properties.
    return OperationResult::notImplemented("TextPropertyService::read");
}

OperationResult TextPropertyService::apply(MText&, const TextProperties&) const
{
    // TODO: T1 - use a QString setter; preserve font/color during Copy/Serialize.
    return OperationResult::notImplemented("TextPropertyService::apply");
}

} // namespace qcad_more

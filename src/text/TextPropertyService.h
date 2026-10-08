#pragma once

#include "common/OperationResult.h"
#include "text/TextProperties.h"

class MText;

namespace qcad_more {

class TextPropertyService {
public:
    // MText's existing getters are non-const, so use a borrowed mutable reference.
    // Reading must not change the entity. Failure leaves output unchanged.
    OperationResult read(MText& text, TextProperties& output) const;

    // Validate the full property set before updating. Failure changes nothing.
    // The caller refreshes the view and marks the document modified on success.
    OperationResult apply(MText& text, const TextProperties& properties) const;
};

} // namespace qcad_more

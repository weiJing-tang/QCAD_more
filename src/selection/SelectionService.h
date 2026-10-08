#pragma once

#include "common/OperationResult.h"
#include <QList>
#include <QRectF>

class MEntity;
class QCADView;

namespace qcad_more {

enum class SelectionMode { Replace, Add };

class SelectionService {
public:
    // Output borrows live entities from view; use immediately on the GUI thread.
    // Empty selection returns EmptySelection. Failure leaves output unchanged.
    OperationResult captureSelected(QCADView& view, QList<MEntity*>& output) const;

    // Select entities fully contained in a normalized world-space rectangle.
    // Failure leaves the existing selection unchanged.
    OperationResult selectInRectangle(QCADView& view, const QRectF& worldRect,
                                      SelectionMode mode) const;
};

} // namespace qcad_more

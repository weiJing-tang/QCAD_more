#pragma once

#include <QColor>
#include <QFont>
#include <QRectF>
#include <QString>

namespace qcad_more {

// A complete property set, not a partial update. Preserve QString Unicode text.
struct TextProperties {
    QString content;
    QFont font;
    QColor color = Qt::black;
    QRectF bounds; // Normalized rectangle in world coordinates.
};

} // namespace qcad_more

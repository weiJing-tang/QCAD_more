#pragma once
#include "editing/DocumentHistory.h"
#include <QWidget>
#include <QTransform>
#include <QPen>
#include <QBrush>
#include <QFont>
class QMenu;
class MEntity;
namespace qcad_more {
class EditingActions {
public:
    enum class Align { Left, HCenter, Right, Top, VCenter, Bottom };
    EditingActions(QCADView& view, BlockLibrary& library, QWidget& parent);
    ~EditingActions();
    void addMenus();
    void copy(); void cut(); void paste(); void duplicate(); void remove();
    void translate(QPointF delta);
    void transform(double degrees,double scale,bool flipH=false,bool flipV=false);
    void transformDialog();
    void align(Align mode); void distribute(bool horizontal);
    void order(bool front); void group(); void ungroup();
    void style(const QPen* pen,const QBrush* brush,const QFont* font=nullptr,const QColor* textColor=nullptr);
    DocumentHistory history;
private:
    struct Clipboard { QVector<EntitySnapshot> entities; BlockLibrary library; };
    Clipboard selectedSnapshots() const;
    void paste(const Clipboard& data,QPointF offset);
    QList<MEntity*> selection();
    QRectF selectionBox() const;
    QCADView& view_;
    BlockLibrary& library_;
    QWidget& parent_;
    QByteArray lastClipboard_;
    int pasteCount_=0;
};
}

#pragma once
#include "qcad/MCommand.h"
#include <QRubberBand>
#include <QCursor>
#include <QList>
#include <memory>
class MEntity;
namespace qcad_more {
class SelectCommand final : public MCommand {
public:
    explicit SelectCommand(QCADView* view) : MCommand(view) {}
    int GetType() override { return ctSelect; }
    int OnLButtonDown(QMouseEvent*) override;
    int OnLButtonUp(QMouseEvent*) override;
    int OnMouseMove(QMouseEvent*) override;
    int OnRButtonDown(QMouseEvent*) override { return Cancel(); }
    int Cancel() override;
private:
    QPoint origin_;
    bool pressed_ = false;
    bool append_ = false;
    QPointF dragOrigin_;
    QList<MEntity*> dragTargets_;
    MEntity* dragPreview_ = nullptr; // Borrowed from the view's preview slot.
    QCursor previousCursor_;
    std::unique_ptr<QRubberBand> band_;
};
}

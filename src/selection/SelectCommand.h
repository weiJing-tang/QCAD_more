#pragma once
#include "qcad/MCommand.h"
#include <QRubberBand>
#include <memory>
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
    std::unique_ptr<QRubberBand> band_;
};
}

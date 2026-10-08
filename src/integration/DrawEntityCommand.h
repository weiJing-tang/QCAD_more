#pragma once
#include "qcad/MCommand.h"
#include "qcad/ENTITY.H"
#include <memory>
namespace qcad_more {
// Shared preview/commit lifetime for the course project's basic drawing tools.
class DrawEntityCommand final : public MCommand {
public:
    DrawEntityCommand(QCADView* view, int type);
    int GetType() override { return type_; }
    int OnLButtonDown(QMouseEvent*) override;
    int OnMouseMove(QMouseEvent*) override;
    int OnLButtonDblClk(QMouseEvent*) override;
    int OnRButtonDown(QMouseEvent*) override { return Cancel(); }
    int Cancel() override;
private:
    std::unique_ptr<MEntity> make(const QPointF& point) const;
    int type_;
    QVector<QPointF> points_;
};
class MoveSelectionCommand final : public MCommand {
public:
    explicit MoveSelectionCommand(QCADView* view) : MCommand(view) {}
    int GetType() override { return ctMove; }
    int OnLButtonDown(QMouseEvent*) override;
    int OnRButtonDown(QMouseEvent*) override { return Cancel(); }
    int Cancel() override { m_nStep = 0; return 0; }
private:
    QPointF start_;
};
}

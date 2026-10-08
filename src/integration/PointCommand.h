#pragma once
#include "qcad/MCommand.h"
#include "common/OperationResult.h"
#include <functional>
namespace qcad_more {
class PointCommand final : public MCommand {
public:
    using Handler = std::function<OperationResult(const QPointF&)>;
    PointCommand(QCADView* view, Handler commit, Handler preview = {}, bool repeat = false);
    int GetType() override { return 30; }
    int OnLButtonDown(QMouseEvent*) override;
    int OnMouseMove(QMouseEvent*) override;
    int OnRButtonDown(QMouseEvent*) override;
    int Cancel() override;
private:
    Handler commit_, preview_;
    bool repeat_, finished_ = false;
};
}

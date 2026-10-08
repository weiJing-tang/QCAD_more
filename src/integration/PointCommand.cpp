#include "integration/PointCommand.h"
#include "qcad/ENTITY.H"
#include "qcad/mainwindow.h"
#include <QMouseEvent>
#include <QTimer>
#include <QMessageBox>
namespace qcad_more {
PointCommand::PointCommand(QCADView* view, Handler commit, Handler preview, bool repeat)
    : MCommand(view), commit_(std::move(commit)), preview_(std::move(preview)), repeat_(repeat) {}
int PointCommand::OnLButtonDown(QMouseEvent* event)
{
    if (finished_) return 0;
    const auto result = commit_(m_pDC->ScreentoWorld(event->pos()));
    if (!result.ok()) QMessageBox::warning(m_pDC, QStringLiteral("操作未完成"), result.message);
    else {
        m_pDC->setModified(true); m_pDC->update();
        Prompt(repeat_ ? QStringLiteral("已插入；继续单击可再次插入，Esc / 右键结束。") : QStringLiteral("块定义成功。"));
        if (!repeat_) {
            finished_ = true;
            QTimer::singleShot(0, m_pDC, [view=m_pDC] { view->selectEntity(); });
        }
    }
    return 0;
}
int PointCommand::OnMouseMove(QMouseEvent* event)
{
    if (!finished_ && preview_) {
        auto result = preview_(m_pDC->ScreentoWorld(event->pos()));
        if (!result.ok()) Prompt(result.message);
    }
    return 0;
}
int PointCommand::OnRButtonDown(QMouseEvent*)
{
    Cancel();
    QTimer::singleShot(0, m_pDC, [view=m_pDC] { view->selectEntity(); });
    return 0;
}
int PointCommand::Cancel() { finished_ = true; m_pDC->setPreview(nullptr); return 0; }
}

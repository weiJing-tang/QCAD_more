#include "selection/SelectCommand.h"
#include "selection/SelectionService.h"
#include "qcad/ENTITY.H"
#include "qcad/mainwindow.h"
#include <QMouseEvent>
namespace qcad_more {
int SelectCommand::OnLButtonDown(QMouseEvent* event)
{
    origin_ = event->pos(); pressed_ = true;
    append_ = event->modifiers().testFlag(Qt::ShiftModifier);
    band_ = std::make_unique<QRubberBand>(QRubberBand::Rectangle, m_pDC);
    band_->setGeometry(QRect(origin_, QSize()));
    return 0;
}
int SelectCommand::OnMouseMove(QMouseEvent* event)
{
    if (pressed_) {
        band_->setGeometry(QRect(origin_, event->pos()).normalized());
        band_->show();
    }
    return 0;
}
int SelectCommand::OnLButtonUp(QMouseEvent* event)
{
    if (!pressed_) return 0;
    if ((origin_ - event->pos()).manhattanLength() > 4) {
        SelectionService().selectInRectangle(*m_pDC,
            QRectF(m_pDC->ScreentoWorld(origin_), m_pDC->ScreentoWorld(event->pos())),
            append_ ? SelectionMode::Add : SelectionMode::Replace);
    } else {
        if (!append_) m_pDC->ClearSelections();
        const auto entities = m_pDC->GetEntityList();
        const auto position = m_pDC->ScreentoWorld(event->pos());
        for (auto it = entities.crbegin(); it != entities.crend(); ++it) {
            if ((*it)->Pick(position, 5.0/m_pDC->GetScale())) {
                if (append_ && m_pDC->GetSelectedEntityList().contains(*it)) m_pDC->RemoveSelection(*it);
                else m_pDC->AddSelection(*it);
                break;
            }
        }
    }
    Cancel();
    Prompt(QStringLiteral("已选择 %1 个图元；Shift 可追加选择。").arg(m_pDC->GetSelectedEntityList().size()));
    m_pDC->update();
    return 0;
}
int SelectCommand::Cancel() { pressed_ = false; band_.reset(); return 0; }
}

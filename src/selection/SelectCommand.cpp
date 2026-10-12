#include "selection/SelectCommand.h"
#include "selection/SelectionService.h"
#include "qcad/ENTITY.H"
#include "qcad/mainwindow.h"
#include "blocks/BlockEntity.h"
#include <QApplication>
#include <QMouseEvent>
namespace qcad_more {
int SelectCommand::OnLButtonDown(QMouseEvent* event)
{
    origin_ = event->pos(); pressed_ = true;
    append_ = event->modifiers().testFlag(Qt::ShiftModifier);
    dragOrigin_ = m_pDC->ScreentoWorld(origin_);
    if (!append_) {
        const auto entities = m_pDC->GetEntityList();
        for (auto it = entities.crbegin(); it != entities.crend(); ++it) {
            if (!(*it)->Pick(dragOrigin_, 5.0 / m_pDC->GetScale())) continue;
            if ((*it)->GetType() == etBlock) {
                if (!m_pDC->GetSelectedEntityList().contains(*it)) {
                    m_pDC->ClearSelections(); m_pDC->AddSelection(*it);
                }
                dragTargets_ = m_pDC->GetSelectedEntityList();
                m_pDC->update();
            }
            break;
        }
    }
    if (!dragTargets_.isEmpty()) return 0;
    band_ = std::make_unique<QRubberBand>(QRubberBand::Rectangle, m_pDC);
    band_->setGeometry(QRect(origin_, QSize()));
    return 0;
}
int SelectCommand::OnMouseMove(QMouseEvent* event)
{
    if (pressed_) {
        if (!dragTargets_.isEmpty()) {
            const auto position = m_pDC->ScreentoWorld(event->pos());
            const auto delta = position - dragOrigin_;
            if (!dragPreview_) {
                if ((origin_ - event->pos()).manhattanLength() < QApplication::startDragDistance()) return 0;
                auto preview = std::make_unique<BlockEntity>();
                for (auto* entity : dragTargets_) {
                    std::unique_ptr<MEntity> copy(entity->Copy());
                    if (!copy) return 0;
                    preview->children.push_back(std::move(copy));
                }
                previousCursor_ = m_pDC->cursor();
                m_pDC->setCursor(Qt::ClosedHandCursor);
                dragPreview_ = preview.get();
                m_pDC->setPreview(std::move(preview), dragTargets_);
            }
            // Preview coordinates are reset by moving from their last anchor to the new one.
            auto* preview = static_cast<BlockEntity*>(dragPreview_);
            preview->Move(preview->instance.position, delta);
            m_pDC->update();
            Prompt(QStringLiteral("拖动块：松开提交，Esc / 右键取消。"));
            return 0;
        }
        band_->setGeometry(QRect(origin_, event->pos()).normalized());
        band_->show();
    }
    return 0;
}
int SelectCommand::OnLButtonUp(QMouseEvent* event)
{
    if (!pressed_) return 0;
    if (!dragTargets_.isEmpty()) {
        OnMouseMove(event); // Include the final release position, even without a move event.
        const auto delta = m_pDC->ScreentoWorld(event->pos()) - dragOrigin_;
        if (dragPreview_ && !delta.isNull()) {
            for (auto* entity : dragTargets_) entity->Move(QPointF(), delta);
            m_pDC->setModified(true);
        }
        Cancel();
        Prompt(QStringLiteral("块位置已确定；按住块可再次拖动。"));
        return 0;
    }
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
int SelectCommand::Cancel()
{
    if (dragPreview_) {
        m_pDC->setPreview(nullptr);
        m_pDC->setCursor(previousCursor_);
        dragPreview_ = nullptr;
    }
    dragTargets_.clear(); pressed_ = false; band_.reset();
    m_pDC->update();
    return 0;
}
}

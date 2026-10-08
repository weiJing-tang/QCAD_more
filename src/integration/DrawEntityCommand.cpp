#include "integration/DrawEntityCommand.h"
#include "qcad/mainwindow.h"
#include <QInputDialog>
#include <QMouseEvent>
#include <QLineF>
namespace qcad_more {
DrawEntityCommand::DrawEntityCommand(QCADView* view, int type) : MCommand(view), type_(type)
{ Prompt(QStringLiteral("单击指定点；多边形双击结束；Esc / 右键取消。")); }
std::unique_ptr<MEntity> DrawEntityCommand::make(const QPointF& point) const
{
    if (points_.isEmpty()) return {};
    std::unique_ptr<MEntity> entity;
    switch (type_) {
    case ctCreateLine: entity = std::make_unique<MLine>(points_.first(), point); break;
    case ctCreateRectangle: case ctCreateText:
        entity = std::make_unique<MRectangle>(points_.first(), point); break;
    case ctCreateEllipse: entity = std::make_unique<MEllipse>(points_.first(), point); break;
    case ctCreateCircle: entity = std::make_unique<CCircle>(points_.first(), QLineF(points_.first(), point).length()); break;
    case ctCreateArc:
        if (points_.size() < 2) entity = std::make_unique<MLine>(points_.first(), point);
        else if (QLineF(points_[0], points_[1]).length() > 1e-6)
            entity = std::make_unique<CArc>(points_[0], points_[1], point);
        break;
    case ctCreatePolygon: {
        auto vertices = points_;
        if (vertices.last() != point) vertices.push_back(point);
        entity = std::make_unique<MPolygon>(vertices);
        break;
    }
    }
    if (entity) {
        QPen pen(m_pDC->lineColor()); pen.setStyle(m_pDC->penStyle()); pen.setWidthF(m_pDC->penWidth());
        QBrush brush(m_pDC->brushColor());
        entity->SetAttrib(&pen, &brush);
    }
    return entity;
}
int DrawEntityCommand::OnLButtonDown(QMouseEvent* event)
{
    const auto point = m_pDC->ScreentoWorld(event->pos());
    if (points_.isEmpty() || type_ == ctCreatePolygon || (type_ == ctCreateArc && points_.size() < 2)) {
        if (points_.isEmpty() || points_.last() != point) points_.push_back(point);
        return 0;
    }
    auto entity = make(point);
    if (type_ == ctCreateText) {
        const auto bounds = QRectF(points_.first(), point).normalized();
        if (bounds.width() <= 0 || bounds.height() <= 0) return 0;
        bool accepted = false;
        const auto content = QInputDialog::getMultiLineText(m_pDC, QStringLiteral("创建 Text"),
            QStringLiteral("文本内容："), {}, &accepted);
        if (!accepted) return Cancel();
        auto text = std::make_unique<MText>();
        text->SetText(content); text->SetFont(m_pDC->font()); text->SetTextColor(m_pDC->textColor());
        text->SetLeftTopPos(bounds.topLeft()); text->SetRightBottomPos(bounds.bottomRight());
        entity = std::move(text);
    }
    if (entity) m_pDC->addEntity(entity.release());
    return Cancel();
}
int DrawEntityCommand::OnMouseMove(QMouseEvent* event)
{ m_pDC->setPreview(make(m_pDC->ScreentoWorld(event->pos()))); return 0; }
int DrawEntityCommand::OnLButtonDblClk(QMouseEvent* event)
{
    if (type_ == ctCreatePolygon && points_.size() >= 3) {
        auto entity = make(m_pDC->ScreentoWorld(event->pos()));
        if (entity) m_pDC->addEntity(entity.release());
        Cancel();
    }
    return 0;
}
int DrawEntityCommand::Cancel() { points_.clear(); m_pDC->setPreview(nullptr); return 0; }
int MoveSelectionCommand::OnLButtonDown(QMouseEvent* event)
{
    const auto position = m_pDC->ScreentoWorld(event->pos());
    if (m_pDC->GetSelectedEntityList().isEmpty()) { Prompt(QStringLiteral("请先选择要移动的图形。")); return 0; }
    if (m_nStep == 0) { start_ = position; m_nStep = 1; Prompt(QStringLiteral("请选择移动目标点。")); }
    else {
        for (auto* entity : m_pDC->GetSelectedEntityList()) entity->Move(start_, position);
        m_pDC->setModified(true); m_pDC->update(); Cancel();
    }
    return 0;
}
}

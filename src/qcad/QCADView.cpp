#include "integration/DrawEntityCommand.h"
#include "selection/SelectCommand.h"
#include <QKeyEvent>
#include <QPaintEvent>
#include <QMouseEvent>
#include <algorithm>
#include "QCADView.h"
#include "ENTITY.H"
#include "MCommand.H"
#include "mainwindow.h"
#include "MZoomPan.h"
#include "MZoomWindow.h"

#include <QPainter>
#include <QStatusBar>

QCADView::QCADView()
{
	m_pCmd = NULL;
    m_textColor = Qt::black;
    setFocusPolicy(Qt::StrongFocus);
    setMouseTracking(true);
	m_lineColor = QColor(0, 0, 0);
	m_penStyle = Qt::SolidLine;
	m_penWidth = 1;
	m_brushColor = QColor(255, 255, 255);

	m_scale = 1.0;
	m_Xorg = this->rect().width() / 2;
	m_Yorg = this->rect().height() / 2;
}

QCADView::~QCADView()
{
    setCommand(nullptr);
    qDeleteAll(m_EntityList);
}

 void QCADView::paintEvent(QPaintEvent* event)
{
	QPainter dc(this);

	//绘制画布
	/* // 将原点移动到缩放中心点
	dc.translate(m_offsetX, m_offsetY);
	// 设置缩放比例
	dc.scale(m_scale, m_scale);
	dc.translate(-m_offsetX, -m_offsetY);

	dc.translate(Xorg, Yorg);
	*/

	//坐标系统
	dc.setPen(QPen(Qt::black, 1));


	// 获取窗口的宽度和高度
	int width = this->width();
	int height = this->height();

	// 计算中心位置
	int centerX = m_Xorg;// width / 2;
	int centerY = m_Yorg;// height / 2;
	//m_Xorg = centerX;
	//m_Yorg = centerY;

	// 清除背景
	dc.fillRect(event->rect(), Qt::white);

	// 设置坐标轴长度（可以根据需求调整比例）
	int axisLength = std::min(width, height) * 0.2; // 坐标轴长度为窗口宽度或高度的20%

	// 绘制坐标轴
	dc.setPen(Qt::black);
	// 绘制向上的 Y 轴
	dc.drawLine(centerX, centerY, centerX, centerY - axisLength); // Y 轴

	// 绘制向左的 X 轴
	dc.drawLine(centerX, centerY, centerX + axisLength, centerY); // X 轴

	// 绘制原点
	dc.setBrush(Qt::red);
	dc.drawEllipse(centerX - 5, centerY - 5, 10, 10); // 原点

	// 绘制坐标值
	dc.drawText(centerX - 15, centerY + 2, "O");
	// 绘制标志文本
	dc.drawText(centerX + 5, centerY - axisLength + 10, "Y"); // Y轴标志
	dc.drawText(centerX + axisLength + 10, centerY - 5, "X"); // X轴标志

	dc.end();

    //绘制全部实体
	foreach(MEntity * pEnt, m_EntityList)
	{
		//如果实体在选择集中，则显示状态
		if (!m_SelectEntityList.contains(pEnt))
		{
			pEnt->Draw(this, dmNormal);// dc.setPen(pEnt->GetPen());
			//dc.setBrush(pEnt->GetBrush());
		}
		else {
			pEnt->Draw(this, dmSelect);
		}
	}
    if (preview_) preview_->Draw(this, dmSelect);
}
void QCADView::resizeEvent(QResizeEvent* event)
{
	// 获取窗口的宽度和高度
	m_Xorg = this->width()/2;
	m_Yorg = this->height()/2;

	update();
}


void QCADView::mousePressEvent(QMouseEvent* mouseEvent)
{
	if (mouseEvent->button() == Qt::LeftButton && m_pCmd)
	{
		m_pCmd->OnLButtonDown(mouseEvent);
	}
	if (mouseEvent->button() == Qt::RightButton && m_pCmd)
	{
		m_pCmd->OnRButtonDown(mouseEvent);
	}
}

void QCADView::mouseMoveEvent(QMouseEvent* mouseEvent)
{
	if (m_pCmd)
	{
		m_pCmd->OnMouseMove(mouseEvent);
		return;
	}

	//显示屏幕坐标和世界坐标
	QPoint pos = mouseEvent->pos();
	//QPointF sPos = mouseEvent->screenPos();
	//QPointF lPos = mouseEvent->localPos();
	//QPointF gPos = mouseEvent->globalPos();

	MainWindow* pMain = g_pMainWnd;
	QPointF scnPos = this->ScreentoWorld(pos);
	QString sScreenPosX = QString::number(scnPos.x(), 'f', 2);
	QString sScreenPosY = QString::number(scnPos.y(), 'f', 2);
	QString sScnPos = QStringLiteral("当前坐标：");
	sScnPos += sScreenPosX;
	sScnPos += ", ";
	sScnPos += sScreenPosY;

	if (m_pCmd == NULL)
		if (pMain) pMain->statusBar()->showMessage(sScnPos);
}

void QCADView::mouseReleaseEvent(QMouseEvent* mouseEvent)
{
    if (m_pCmd && mouseEvent->button() == Qt::LeftButton) m_pCmd->OnLButtonUp(mouseEvent);
}

void QCADView::mouseDoubleClickEvent(QMouseEvent* mouseEvent)
{
	if (mouseEvent->button() != Qt::LeftButton)
	{
		return;
	}

	if (m_pCmd)
	{
		m_pCmd->OnLButtonDblClk(mouseEvent);
	}
}

void QCADView::addEntity(MEntity* pEnt)
{
    if (pEnt) { m_EntityList.push_back(pEnt); setModified(true); update(); }
}

void QCADView::removeEntity(MEntity* pEnt)
{
    m_SelectEntityList.removeAll(pEnt);
    if (m_EntityList.removeOne(pEnt)) { setModified(true); update(); }
}

void QCADView::removeLastEntity()
{
	m_EntityList.pop_back();
}

void QCADView::selectEntity()
{
    setCommand(new qcad_more::SelectCommand(this));
}

void QCADView::moveEntity()
{
    setCommand(new qcad_more::MoveSelectionCommand(this));
}

void QCADView::AddSelection(MEntity* pEnt)
{
    if (m_EntityList.contains(pEnt) && !m_SelectEntityList.contains(pEnt)) m_SelectEntityList.push_back(pEnt);
}

void QCADView::RemoveSelection(MEntity* pEnt)
{
	m_SelectEntityList.removeOne(pEnt);
}

void QCADView::drawLine()
{
    setCommand(new qcad_more::DrawEntityCommand(this, ctCreateLine));
}

void QCADView::drawCircle()
{
    setCommand(new qcad_more::DrawEntityCommand(this, ctCreateCircle));
}

void QCADView::drawArc()
{
    setCommand(new qcad_more::DrawEntityCommand(this, ctCreateArc));
}

void QCADView::drawRectangle()
{
    setCommand(new qcad_more::DrawEntityCommand(this, ctCreateRectangle));
}

void QCADView::drawPolygon()
{
    setCommand(new qcad_more::DrawEntityCommand(this, ctCreatePolygon));
}

void QCADView::drawEllipse()
{
    setCommand(new qcad_more::DrawEntityCommand(this, ctCreateEllipse));
}

void QCADView::drawText()
{
    setCommand(new qcad_more::DrawEntityCommand(this, ctCreateText));
}

void QCADView::setScale(double scale)
{
    if (std::isfinite(scale)) m_scale = qBound(0.01, scale, 100.0);
}

//窗口功能
void QCADView::Scale(double scale)
{
	qreal scale_new = m_scale * scale;
	setScale(scale_new);
	update();
}

void QCADView::zoomRect(QRectF rect)
{
    rect = rect.normalized();
    const double width = qMax(rect.width(), 1.0), height = qMax(rect.height(), 1.0);
    setScale(qMin((this->width()-40)/width, (this->height()-40)/height));
    const QPointF center = rect.center();
    m_Xorg = this->width()/2 - center.x()*m_scale;
    m_Yorg = this->height()/2 + center.y()*m_scale;
    update();
}
//center是屏幕坐标系
void QCADView::ScaleCenter(double scale, QPoint center)
{
	int X_Cen = this->width() / 2;
	int Y_cen = this->height() / 2;

	m_Xorg -= center.x() - X_Cen;
	m_Yorg -= center.y() - Y_cen;
	m_scale *= scale;
	update();
}

void QCADView::MoveOrg(int delX, int delY)
{
	m_Xorg += delX;
	m_Yorg += delY;
}

void QCADView::setOrg(QPoint cps)
{
	m_Xorg = cps.x();
	m_Yorg = cps.y();
}

double QCADView::GetScale()
{
	return m_scale;
}

QPoint QCADView::WorldtoScreen(QPointF pos)
{
	qreal x = pos.x();
	qreal y = pos.y();
	QPoint ScreenPt;
	ScreenPt.setX( x * m_scale + m_Xorg );
	ScreenPt.setY( -y * m_scale + m_Yorg );

	return ScreenPt;
}

QPointF QCADView::ScreentoWorld(QPointF pos)
{
	QPointF WorldPt;
	qreal x = pos.x();
	qreal y = pos.y();

	WorldPt.setX((x - m_Xorg) / m_scale);
	WorldPt.setY((-y + m_Yorg) / m_scale);

	return WorldPt;
}

QPointF QCADView::ScreentoWorld(QPoint pos)
{
	QPointF WorldPt;
	int x = pos.x();
	int y = pos.y();

	/*double scale = this->GetScale();
	WorldPt.setX((x - m_offsetX) / scale + m_offsetX - Xorg);
	WorldPt.setY((y - m_offsetY) / scale + m_offsetY - Yorg);*/

	WorldPt.setX((x - m_Xorg) / m_scale);
	WorldPt.setY((-y + m_Yorg) / m_scale);

	return WorldPt;
}

void QCADView::zoomPan()
{
    setCommand(new MZoomPan(this));
}
//窗显
void QCADView::zoomWindow()
{
    setCommand(new MZoomWindow(this));
}

void QCADView::setCommand(MCommand* command)
{
    if (m_pCmd) m_pCmd->Cancel();
    delete m_pCmd;
    m_pCmd = command;
    preview_.reset();
    setFocus();
    update();
}
void QCADView::setPreview(std::unique_ptr<MEntity> entity)
{
    preview_ = std::move(entity);
    update();
}
void QCADView::replaceEntities(QList<MEntity*> entities)
{
    setCommand(nullptr);
    ClearSelections();
    qDeleteAll(m_EntityList);
    m_EntityList = std::move(entities);
    selectEntity();
    update();
}
void QCADView::setModified(bool modified)
{
    modified_ = modified;
    if (modifiedChanged) modifiedChanged(modified);
}
void QCADView::keyPressEvent(QKeyEvent* event)
{
    if (event->key() == Qt::Key_Escape) { selectEntity(); return; }
    QWidget::keyPressEvent(event);
}

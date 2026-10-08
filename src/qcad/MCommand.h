#pragma once

#include <QPoint>

class QMouseEvent;
class QCADView;
//class QRubberBand;

enum ECommandType	// 命令类
{
	ctSelect = 0,

	//  创建对象命令类
	ctCreateLine = 1,		// 创建直线段
	ctCreateRectangle = 2,	// 创建矩形
	ctCreateCircle = 3,		// 创建圆
	ctCreateArc = 4,		// 创建圆弧
	ctCreateEllipse = 5,    //创建椭圆
	ctCreatePolygon = 6,    //多边形
	ctCreateText = 7,       //创建文本

	//  修改命令类
	ctMove = 11,			// 移动
	ctRotate = 12,			// 旋转
	ctMirror = 13,			// 镜像

	//修改视图命令类
	ctZoomPan = 20,
	ctZoomWindow = 21
};


class MCommand
{
protected:
	int		m_nStep; // 命令操作步
	bool    m_bCaptured; //捕捉
	QPointF m_ptCapture; //捕捉点
	//QRubberBand* rubberBand; //橡皮筋

	void     ShowCapturePoint(const QPointF& pos, bool bShow = false);
	bool     GetNearKeyPoint(QPointF& pt);
	QCADView* m_pDC = nullptr;
public:
	MCommand() {
		m_nStep = 0;
		m_bCaptured = false;
	}
	MCommand(QCADView* pScene) {
		m_nStep = 0;
		m_bCaptured = false;
		m_pDC = pScene;
	}
	virtual ~MCommand() {}

	virtual int GetType() = 0; // 返回命令类型 ECommandType
	virtual int OnLButtonDown(QMouseEvent* mouseEvent) = 0;
	virtual int OnLButtonUp(QMouseEvent*) { return 0; }
	virtual int OnMouseMove(QMouseEvent* mouseEvent);
	virtual int OnRButtonDown(QMouseEvent* mouseEvent) = 0;
	virtual int OnLButtonDblClk(QMouseEvent* mouseEvent) { return 0; }
	virtual int Cancel() = 0;

};

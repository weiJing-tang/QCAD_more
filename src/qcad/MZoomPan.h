#pragma once
#include "MCommand.h"
#include <QtWidgets>
#include <QRubberBand>

class MZoomPan :
    public MCommand
{
private:
    QPointF m_begin;
    QPointF m_end;
    QPointF m_tempPos;
    QRubberBand* rubberBand;
    QPoint origin;

public:
    MZoomPan(QCADView* pDiagramScene);
    ~MZoomPan();

    int GetType();
    int OnLButtonDown(QMouseEvent* mouseEvent);
    int OnMouseMove(QMouseEvent* mouseEvent);
    int OnRButtonDown(QMouseEvent* mouseEvent);
    //int OnLButtonDblClk(QMouseEvent* mouseEvent);
    int Cancel();

};

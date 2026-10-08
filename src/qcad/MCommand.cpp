#include "MCommand.h"
#include "math.h"

#include <QStatusBar>
#include <QMouseEvent>

//show a circle
void MCommand::ShowCapturePoint(const QPointF& pos, bool bShow)
{
	if (bShow)
	{
		//CDC*	pDC = g_pView->GetDC(); // 得到设备环境指针
		//CCircle*	pTempCircle = new CCircle(pos, 3);
		//pTempCircle->Draw(pDC, dmDrag);
		//delete pTempCircle;

		//g_pView->ReleaseDC(pDC); // 释放设备环境指针
	}
}


bool MCommand::GetNearKeyPoint(QPointF& pt)
{
	return false;

	//CVCadDoc *pDoc = (CVCadDoc *)g_pView->GetDocument();
	//POSITION	pos = pDoc->m_EntityList.GetHeadPosition();
	//while(pos!=NULL)
	//{
	//	MEntity*	pEntity = (MEntity *) pDoc->m_EntityList.GetNext(pos);
	//	/////////////////////////////////////////////////////////
	//	if(pEntity->m_nOperationNum <= g_pView->m_nCurrentOperation) {
	//		MEntity * pEntShow = pEntity->GetShowEnt();
	//		if(pEntShow && pEntShow->m_nOperationNum >= 0 )
	//		{
	//			if(pEntity->GetSnapPos(pt))
	//			{
	//				return TRUE;
	//			}
	//		}
	//	}

	//}
}
//
int MCommand::OnMouseMove(QMouseEvent* mouseEvent)
{
	QPointF pt = mouseEvent->pos();

	if (GetNearKeyPoint(pt))
	{
		if (!m_bCaptured)
		{
			ShowCapturePoint(pt, true);
		}

		m_bCaptured = true;
		m_ptCapture = pt;
	}
	else if (m_bCaptured)
	{
		ShowCapturePoint(m_ptCapture, true);

		m_bCaptured = true;
	}

	return 0;
}

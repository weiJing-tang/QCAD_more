#include "math.h"
#include "ENTITY.H"

//////////////////////////////////
// API function
//
double Distance(QPointF& posSrc1, QPointF& posSrc2)
{
	double dis = sqrt((posSrc1.x() - posSrc2.x()) * (posSrc1.x() - posSrc2.x()) +\
		 (posSrc1.y() - posSrc2.y()) * (posSrc1.y() - posSrc2.y()));

	return dis;
}

double GetAngleToXAxis(QPointF& posSrc1, QPointF& posSrc2)
{
	QPointF pos1(posSrc1), pos2(posSrc2);
	//计算两点的距离
	double len = Distance(pos1,pos2);
	if (len < DISTANCE_ZERO)
		return	0;

	QPointF dp = pos2 - pos1;
	//与x轴夹角的余弦值和正弦值
	double cosv = dp.x() / len;
	double sinv = dp.y() / len;
	//通过反余弦函数，求得角度值
	if (sinv >= 0)
		return acos(cosv);
	else if (sinv < 0)
		return 2. * PI - acos(cosv);
	return 0;
}

bool IsSame(QPointF& pointSrc, QPointF& pointDest)
{
	if (Distance(pointSrc, pointDest) < DISTANCE_ZERO)
		return true;
	return false;
}

//判断是否在包围盒内
bool IsPointInBox(QPointF & objPos, QRect & desBox)
{
	if (objPos.x() < desBox.left() || objPos.x() > desBox.right() || \
		objPos.y() < desBox.bottom() || objPos.y() > desBox.top())
		return false;
	return true;
}

void Offset(QPointF	&pnt, double deltax, double deltay)
{
	pnt.setX(pnt.x() + deltax);
	pnt.setY(pnt.y() + deltay);
}

//位置类对象的平移操作
void Offset(QPointF	&pnt, QPointF off)
{
	pnt.setX(pnt.x() + off.x());
	pnt.setY(pnt.y() + off.y());
}

//位置类对象的旋转操作
 void Rotate(QPointF& pos, QPointF& basePos, double angle)
{
	double cosv = cos(angle); // cos value
	double sinv = sin(angle);
	double xc = pos.x();
	double yc = pos.y();
	pos.setX( xc * cosv - yc * sinv + (1. - cosv) * basePos.x() + basePos.y() * sinv );
	pos.setY ( sinv * xc + cosv * yc + (1. - cosv) * basePos.y() - sinv * basePos.x() );

 }

//位置类对象的镜向操作
 void Mirror(QPointF & pos, QPointF p1, QPointF p2)
{
	double	length = Distance(p1,p2);
	if (length <= DISTANCE_ZERO)
		return ;

	double angle = ::GetAngleToXAxis(p1, p2);
	double cos_v = cos(angle); // cos value
	double sin_v = sin(angle);

	double t1 = 2. * cos_v * sin_v;
	double t2 = cos_v * cos_v - sin_v * sin_v;

	double desX = pos.x() * t2 + pos.y() * t1 + p1.x() * (-t2) - p1.y() * t1 + p1.x();
	double desY = pos.x() * t1 + pos.y() * (-t2) + p1.y() * t2 - p1.x() * t1 + p1.y();

	pos.setX(desX);
	pos.setY(desY);
}

// 根据绘图模式设置绘图环境：设置设备环境的模式
void SetDrawEnvir(QPainter* pDC, int drawMode)
{
	switch (drawMode)
	{
	case dmSelect: // 选中状态
	{
		pDC->setCompositionMode(QPainter::CompositionMode_Source);//R2_COPYPEN

		break;
	}
	case dmPrompt: // 提示状态
	{
		pDC->setCompositionMode(QPainter::CompositionMode_Source);

		break;
	}
	case dmDrag: // 拖动状态,异或模式
	{
		pDC->setCompositionMode(QPainter::CompositionMode_Xor); //R2_XORPEN

		break;
	}
	case dmInvalid: // 擦除状态
	{
		pDC->setCompositionMode(QPainter::CompositionMode_Source);

		break;
	}
	case dmNormal:
	default:
	{
		pDC->setCompositionMode(QPainter::CompositionMode_Source);

		break;
	}
	}
}

//原版本
void SetDrawEnvir(QPainter* pDC, int drawMode, QPen* pPen)
{
	int			 lineWidth;
	Qt::PenStyle lineStyle;
	QColor  	color;
	switch (drawMode)
	{
	case dmSelect: // 选中状态
	{
		pDC->setCompositionMode(QPainter::CompositionMode_Source);//R2_COPYPEN
		lineStyle = Qt::SolidLine;
		lineWidth = 1;
		color = QColor(255, 0, 0);
		break;
	}
	case dmPrompt: // 提示状态
	{
		pDC->setCompositionMode(QPainter::CompositionMode_Source);
		lineStyle = Qt::DashLine;
		lineWidth = 1;
		color = QColor(0, 255, 255);
		break;
	}
	case dmDrag: // 拖动状态,异或模式
	{
		pDC->setCompositionMode(QPainter::CompositionMode_Xor); //R2_XORPEN
		lineStyle = Qt::SolidLine;
		lineWidth = 1;
		color = QColor(192, 192, 0);
		break;
	}
	case dmInvalid: // 擦除状态
	{
		pDC->setCompositionMode(QPainter::CompositionMode_Source);
		lineStyle = Qt::SolidLine;
		lineWidth = 1;
		color = pDC->background().color(); // 用背景色画
		break;
	}
	case dmNormal:
	default:
	{
		pDC->setCompositionMode(QPainter::CompositionMode_Source);
		lineStyle = Qt::SolidLine;
		lineWidth = 1;
		color = QColor(0, 0, 0);
		break;
	}
	}
	pPen = new QPen(color);
	pPen->setWidth(lineWidth);
	pPen->setStyle(lineStyle);
}

MEntity::MEntity()
{
	next = nullptr;
    m_type = etUnknow;		// EEntityType
	m_nOperationNum = 0;
	//Init();
}

MEntity::MEntity(const MEntity& entity)
{
	next = nullptr;
    m_brush = entity.m_brush;
    m_pen = entity.m_pen;
	m_nOperationNum = 0;
	m_type = entity.m_type;
}

MEntity::~MEntity()
{
}

void MEntity::Init()
{
	next = NULL;
}

void MEntity::SetAttrib(QPen* pPen, QBrush* pBrush)
{
	m_pen = *pPen;
	m_brush = *pBrush;
}

MEntity MEntity::operator = (const MEntity& entity)
{
	next = nullptr;
    m_brush = entity.m_brush;
    m_pen = entity.m_pen;
	m_type = entity.m_type;
	return *this;
}

void MEntity::Serialize(QDataStream& ar, bool bSave)
{
    if (bSave) ar << m_pen << m_brush;
    else ar >> m_pen >> m_brush;
}

void MEntity::Append(MEntity* pEnt)
{
	while (next) {
		next = next->next;
	}

	next = pEnt;
}

MEntity* MEntity::GetCurrentEnt()
{
	MEntity* pEntShow = this;
	//if(pEntShow->m_nOperationNum > g_pView->m_nCurrentOperation)
	//	return NULL;

	MEntity* pNext;
	while (pEntShow) {
		pNext = pEntShow->next;
		if (pNext == NULL)
			return pEntShow;
		//if( pNext->m_nOperationNum > g_pView->m_nCurrentOperation)  {
		//	return pEntShow;
		//}

		pEntShow = pNext;
	}

	return pEntShow;
}

MEntity* MEntity::GetShowEnt()
{
	/*		MEntity * pEntShow = this;
			if(pEntShow->m_nOperationNum == 0)
				return NULL;

			if(pEntShow->m_nOperationNum > g_pView->m_nCurrentOperation)
				return NULL;

			MEntity * pPrev = pEntShow;
			while(pEntShow->next) {
				if( pEntShow->m_nOperationNum < g_pView->m_nCurrentOperation)  {
					pPrev = pEntShow;
					pEntShow = pEntShow->next;
				}
				else //==
					break;
			}

			if(pEntShow->m_nOperationNum == 0)
				return pPrev;
			else
				return pEntShow;
				*/
	MEntity* pEntShow = GetCurrentEnt();
	if (pEntShow == NULL)
		return NULL;

	if (pEntShow->GetType() == etUnknow)
		return NULL;
	else
		return pEntShow;
}

void MEntity::Remove()//清除
{
	MEntity* pNext = this;
	while (pNext) {
		MEntity* pTem = pNext;
		pNext = pNext->next;
		delete pTem;
	}
}

MEntity* MEntity::Erase()//临时删除
{
	MEntity* pShow = GetShowEnt();
	if (pShow) {
		MEntity* pNew = new MEntity();
		pShow->next = pNew;
		//pNew->m_bErased = TRUE;

		return pNew;
	}

	return NULL;
}

void MEntity::SetPen(QPen& pen)
{
	m_pen = pen;
}

void MEntity::SetBrush(QBrush& brush)
{
	m_brush = brush;
}

QPen MEntity::GetPen() const
{
	return m_pen;
}

QBrush MEntity::GetBrush() const
{
	return m_brush;
}

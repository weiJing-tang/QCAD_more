#include "Entity.h"
#include <math.h>

// 构造函数，初始化弧的属性
CArc::CArc()
{
    Init();  // 初始化成员变量
}

// 复制构造函数，复制另一个弧对象的属性
CArc::CArc(const CArc& arc)
{
    Init();
    m_center = arc.m_center;
    m_begin = arc.m_begin;
    m_end = arc.m_end;
}

// 通过圆心、起点和终点构造弧对象
CArc::CArc(const QPointF& center, const QPointF& startPos, const QPointF& endPos)
{
    Init();
    m_center = center;
    m_begin = startPos;
    m_end = endPos;
}

// 析构函数，清理资源
CArc::~CArc()
{
}

// 初始化函数，设置弧的初始属性
void CArc::Init()
{
    MEntity::Init();  // 初始化基类
    m_type = etArc;   // 设置类型为弧形
    m_center = QPointF(0, 0);  // 初始化中心点为 (0, 0)
    m_begin = QPointF(0, 0);   // 初始化起点
    m_end = QPointF(0, 0);     // 初始化终点
}

// 类型为 etArc
int CArc::GetType()
{
    return etArc;
}

// 复制当前弧对象，返回一个指向新对象的指针
MEntity* CArc::Copy()
{
    return new CArc(*this);
}

// 设置弧的绘制属性，包括画笔和画刷
void CArc::SetAttrib(QPen* pPen, QBrush* pBrush)
{
    m_pen = *pPen;
    // 弧一般不需要画刷
}

// 返回弧的起点坐标
QPointF CArc::GetStartPos()
{
    return m_begin;
}

// 返回弧的终点坐标
QPointF CArc::GetEndPos()
{
    return m_end;
}

// 返回弧的中心点坐标
QPointF CArc::GetCenterPos()
{
    return m_center;
}

// 判断某个点是否接近弧的关键点（中心点、起点或终点），用于捕捉功能
bool CArc::GetSnapPos(QPointF& pos)
{
    // 判断是否接近中心点
    if (fabs(QLineF(pos, m_center).length()) < SNAP_DIS) {
        pos = m_center;
        return true;
    }
    // 判断是否接近起点
    if (fabs(QLineF(pos, m_begin).length()) < SNAP_DIS) {
        pos = m_begin;
        return true;
    }
    // 判断是否接近终点
    if (fabs(QLineF(pos, m_end).length()) < SNAP_DIS) {
        pos = m_end;
        return true;
    }
    return false;
}

void CArc::SetEndPos(QPointF& pt)
{
    m_end = pt;
}

// 使用 `QPainter` 绘制弧形
void CArc::Draw(QCADView* pView, int drawMode)
{
    QPainter pDC(pView);

    // 计算弧的半径
    qreal radius = sqrt(pow(m_center.x() - m_begin.x(), 2) + pow(m_center.y() - m_begin.y(), 2));

    // 计算圆的外接矩形
    QPoint topLeft(m_center.x() - radius, m_center.y() + radius);
    QPoint bottomRight(m_center.x() + radius, m_center.y() - radius);
    topLeft = pView->WorldtoScreen(topLeft);
    bottomRight = pView->WorldtoScreen(bottomRight);
    QRectF rect(topLeft, bottomRight);

    // 计算起始角度和终止角度
    qreal startAngle = acos((m_begin.x() - m_center.x()) / radius) * 180 / PI;
    if (m_begin.y() > m_center.y()) startAngle = -startAngle;
    qreal endAngle = acos((m_end.x() - m_center.x()) / radius) * 180 / PI;
    if (m_end.y() > m_center.y()) endAngle = -endAngle;

    // 设置画笔
    if (drawMode == dmSelect)
    {
        QPen pen(Qt::DashLine);
        pen.setColor(Qt::green);
        pen.setWidth(1);
        pDC.setPen(pen);
    }
    else {
        pDC.setPen(m_pen);
    }
    // 弧不填充

    // 绘制弧
    pDC.drawArc(rect, -startAngle * 16, (-endAngle + startAngle) * 16);  // `* 16` 是因为 `drawArc` 使用的是 1/16 度的单位
}
// 判断点 `pos` 是否位于弧的边缘附近（用于拾取操作），允许的误差范围为 `pick_radius`
bool CArc::Pick(const QPointF& pos, const double pick_radius)
{
    // 判断该点是否在弧的边缘附近，可以根据投影判断
    qreal distToCenter = sqrt(pow(pos.x() - m_center.x(), 2) + pow(pos.y() - m_center.y(), 2));
    qreal radius = sqrt(pow(m_center.x() - m_begin.x(), 2) + pow(m_center.y() - m_begin.y(), 2));

    // 检查距离是否在允许误差范围内
    return fabs(distToCenter - radius) <= pick_radius;
}

// 平移弧的位置，从 basePos 移动到 desPos，调整中心点、起点和终点的位置，三点确定弧的特征
void CArc::Move(const QPointF& basePos, const QPointF& desPos, bool bTemp)
{
    QPointF offset = desPos - basePos;
    m_center += offset;
    m_begin += offset;
    m_end += offset;
}

// 旋转弧形，绕 `basePos` 点旋转指定的角度 `angle`
void CArc::Rotate(const QPointF& bp, const double angle)
{
    QPointF basePos(bp);
    ::Rotate(m_center, basePos, angle);
    ::Rotate(m_begin, basePos, angle);
    ::Rotate(m_end, basePos, angle);
}

// 对弧形进行镜像操作，镜像轴由 `FstPos` 和 `SecPos` 的直线定义
void CArc::Mirror(const QPointF& FstPos, const QPointF& SecPos)
{
    ::Mirror(m_center, FstPos, SecPos);
    ::Mirror(m_begin, FstPos, SecPos);
    ::Mirror(m_end, FstPos, SecPos);
}

// 获取弧的最小包围盒（矩形），通过引用 `pBox` 返回矩形边界
void CArc::GetBox(QRectF& pBox)
{
    qreal radius = sqrt(pow(m_center.x() - m_begin.x(), 2) + pow(m_center.y() - m_begin.y(), 2));
    pBox.setLeft(m_center.x() - radius);
    pBox.setTop(m_center.y() - radius);
    pBox.setRight(m_center.x() + radius);
    pBox.setBottom(m_center.y() + radius);
}

// 序列化函数，用于保存和加载弧的几何数据
void CArc::Serialize(QDataStream& ar, bool bSave)
{
    MEntity::Serialize(ar, bSave);  // 调用基类的序列化函数
    if (bSave) {
        ar << m_center << m_begin << m_end;  // 保存弧的中心点、起点和终点
    }
    else {
        ar >> m_center >> m_begin >> m_end;  // 加载弧的中心点、起点和终点
    }
}

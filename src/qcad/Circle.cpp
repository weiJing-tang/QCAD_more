// 包含基本的几何实体类定义
#include "Entity.h"
#include <math.h>

// CCircle类的实现，用于表示圆形几何实体

// 默认构造函数，初始化圆心和半径
CCircle::CCircle()
{
    Init();  // 调用初始化函数

}

// 创建圆的副本
CCircle::CCircle(const CCircle& circle)
{
    Init();  // 调用初始化函数
    MEntity::operator = (circle);
    m_center = circle.m_center;  // 复制圆心
    m_dRadius = circle.m_dRadius;  // 复制半径
}

// 构造函数，通过圆心和半径创建圆
CCircle::CCircle(const QPointF& center, const double& radius)
{
    Init();  // 调用初始化函数
    m_center = center;  // 设置圆心
    m_dRadius = radius;  // 设置半径
}

// 构造函数，通过圆心和圆周上的任意一点创建圆，计算出半径
CCircle::CCircle(const QPointF& center, const QPointF& around)
{
    Init();  // 调用初始化函数
    m_center = center;  // 设置圆心
    m_dRadius = sqrt(pow(center.x() - around.x(), 2) + pow(center.y() - around.y(), 2));
}

CCircle::~CCircle()
{
}

// 初始化函数，设置圆的基本属性
void CCircle::Init()
{
    MEntity::Init();  // 调用基类 MEntity 的初始化方法
    m_type = etCircle;  // 设置实体类型为圆形
    m_center = QPointF(0, 0);  // 初始化圆心为 (0, 0)
    m_dRadius = 0;  // 初始化半径为 0
}

// 设置圆的绘图属性，包括画笔和画刷
void CCircle::SetAttrib(QPen* pPen, QBrush* pBrush)
{
    m_pen = *pPen;
    m_brush = *pBrush;
}
// 返回当前实体的类型-圆形
int CCircle::GetType()
{
    return etCircle;
}

QPointF CCircle::GetCenterPos() {
    return m_center;
}

double CCircle::GetRadius() {
    return m_dRadius;
}

void CCircle::SetRadius(double r)
{
    m_dRadius = r;
}

MEntity* CCircle::Copy()
{
    return new CCircle(*this);
}

// 绘制圆形的函数，使用 QPainter 椭圆功能进行绘制，但是构造边界矩形为正方形
void CCircle::Draw(QCADView* pView, int drawMode)
{
    QPainter pDC(pView);
    QPointF topLeft(m_center.x() - m_dRadius, m_center.y() + m_dRadius);
    QPointF bottom_right(m_center.x() + m_dRadius, m_center.y() - m_dRadius);
    topLeft = pView->WorldtoScreen(topLeft);
    bottom_right = pView->WorldtoScreen(bottom_right);
    QPen pOldPen = pDC.pen();
    QBrush pOldBrush = pDC.brush();
    // 通过圆心和半径计算出圆的边界矩形
    QRectF circleRect(topLeft, bottom_right);// cen_scn.x() - m_dRadius, cen_scn.y() - m_dRadius, 2 * m_dRadius, 2 * m_dRadius);

    if (drawMode == dmSelect)
    {
        QPen pen(Qt::DashLine);
        pen.setColor(Qt::green);
        pen.setWidth(1);
        pDC.setPen(pen);
        QBrush brush(Qt::white);
        pDC.setBrush(brush);
    }
    else {
        pDC.setPen(m_pen);  // 设置画笔
        pDC.setBrush(m_brush);  // 设置画刷
    }

    pDC.drawEllipse(circleRect); // 绘制在这种情况下是圆的椭圆
    pDC.setPen(pOldPen);
    pDC.setBrush(pOldBrush);
}


// 判断某个点是否在圆的边缘附近，用于拾取操作
bool CCircle::Pick(const QPointF& pos, const double pick_radius)
{
    // 计算该点到圆心的距离
    qreal dist = sqrt(pow(pos.x() - m_center.x(), 2) + pow(pos.y() - m_center.y(), 2));
    // 判断该点是否在圆的边缘附近，允许的误差范围为 pick_radius
    return fabs(dist - m_dRadius) <= pick_radius;
}

// 获取圆的最小包围盒，返回矩形区域
void CCircle::GetBox(QRectF& pBox)
{
    // 通过圆心和半径计算出最小包围盒的边界
    pBox.setLeft(m_center.x() - m_dRadius);    // 设置左边界
    pBox.setTop(m_center.y() - m_dRadius);     // 设置上边界
    pBox.setRight(m_center.x() + m_dRadius);   // 设置右边界
    pBox.setBottom(m_center.y() + m_dRadius);  // 设置下边界
}
//移动时跟圆心的位置有关，跟半径无关
// 平移圆的位置，从 basePos 移动到 desPos
void CCircle::Move(const QPointF& basePos, const QPointF& desPos, bool bTemp)
{
    // 计算偏移量
    QPointF offset = desPos - basePos;
    // 更新圆心位置
    m_center += offset;
}

// 旋转圆的中心点
void CCircle::Rotate(const QPointF& bp, const double angle)
{
    QPointF basePos(bp);
    // 使用Rotate 函数旋转圆心
    ::Rotate(m_center, basePos, angle);
}

// 对圆心进行镜像操作
void CCircle::Mirror(const QPointF& pos1, const QPointF& pos2)
{
    // 使用全局 Mirror 函数对圆心进行镜像
    ::Mirror(m_center, pos1, pos2);
}

// 捕捉功能，判断给定的点是否接近圆心，返回捕捉结果
bool CCircle::GetSnapPos(QPointF& pos)
{
    // 如果该点到圆心的距离小于 SNAP_DIS（捕捉距离阈值），则将该点捕捉到圆心
    if (fabs(QLineF(pos, m_center).length()) < SNAP_DIS) {
        pos = m_center;  // 将点设置为圆心
        return true;  // 捕捉成功
    }
    return false;  // 未捕捉到
}

// 序列化函数，用于保存和加载圆的数据
void CCircle::Serialize(QDataStream& ar, bool bSave)
{
    // 调用基类的序列化函数
    MEntity::Serialize(ar, bSave);
    if (bSave) {
        // 如果是保存操作，将圆心和半径写入数据流
        ar << m_center << m_dRadius;
    }
    else {
        // 如果是加载操作，从数据流中读取圆心和半径
        ar >> m_center >> m_dRadius;
    }
}

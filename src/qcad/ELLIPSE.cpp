#include "math.h"
#include "Entity.h"
#include <iostream>

// 构造函数
MEllipse::MEllipse() {
    Init();
}

MEllipse::MEllipse(const MEllipse& elps) {
    Init();
    m_LeftTop = elps.m_LeftTop;
    m_RightBottom = elps.m_RightBottom;
}

MEllipse::MEllipse(const QPointF& lt, const QPointF& rb) {
    Init();
    m_LeftTop = lt;
    m_RightBottom = rb;
    std::cout << "MEllipse created with points: (" << m_LeftTop.x() << ", " << m_LeftTop.y() << ") and ("
        << m_RightBottom.x() << ", " << m_RightBottom.y() << ")" << std::endl;
}

// 析构函数
MEllipse::~MEllipse() {}

// 初始化成员变量
void MEllipse::Init() {
    MEntity::Init();
    m_type = etEllipse;
    m_LeftTop = QPointF(0,0);
    m_RightBottom = QPointF(0, 0);
}

// 返回类型
int MEllipse::GetType() {
    return m_type;
}

// 设置属性
void MEllipse::SetAttrib(QPen* pPen, QBrush* pBrush) {
    if (pPen) {
        m_pen = *pPen;
    }
    if (pBrush) {
        m_brush = *pBrush;
    }
    /*if (pPen == nullptr) {
        // 处理 pPen 为空的情况
        return;
    }
    m_pen = *pPen;
    m_brush = *pBrush;*/
}

// 返回左上角坐标
QPointF MEllipse::GetLTPos() {
    return m_LeftTop;
}

// 返回右下角坐标
QPointF MEllipse::GetRBPos() {
    return m_RightBottom;
}

// 绘制椭圆
void MEllipse::Draw(QCADView* pView, int drawMode)
{
    QPainter pDC(pView);
    //if (drawMode == dmInvalid) return;

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

    // 创建椭圆的矩形边界并绘制椭圆
    QPoint lt = pView->WorldtoScreen(m_LeftTop);
    QPoint rb = pView->WorldtoScreen(m_RightBottom);
    QRectF ellipseRect(lt, rb);
    pDC.drawEllipse(ellipseRect);

    //QPen penToUse;
    //if (drawMode == dmNormal) {
    //    penToUse = m_pen;
    //    pDC.setBrush(m_brush);
    //}
    //else {
    //    penToUse.setColor(Qt::black);
    //    penToUse.setWidthF(1.0);
    //}

    //// 设置画笔
    //pDC.setPen(penToUse);

    //// 绘制椭圆
    //pDC.drawEllipse(ellipseRect);

    //// 恢复之前的画笔和画刷
    //pDC.setPen(oldPen);
    //pDC.setBrush(oldBrush);
}

// 选取椭圆边界
bool MEllipse::Pick(const QPointF& pos, const double pick_radius) {
    double pick_adjusted = pick_radius + 2;

    // 先获取包围盒并进行扩大，用于初步判断
    QRectF sourceBox;
    GetBox(sourceBox);
    QPointF objPos = pos;
    QRectF desBox = sourceBox;
    desBox.adjust(-pick_adjusted, -pick_adjusted, pick_adjusted, pick_adjusted);

    // 判断拾取点是否在测试包围盒中，如果不是，则图元未被选中
    if (objPos.x() < desBox.left() || objPos.x() > desBox.right() || \
        objPos.y() > desBox.bottom() || objPos.y() < desBox.top())
        return false;

    // 计算椭圆的中心点
    QPointF center((m_LeftTop.x() + m_RightBottom.x()) / 2, (m_LeftTop.y() + m_RightBottom.y()) / 2);

    // 计算原始椭圆的长半轴和短半轴
    double halfWidth = fabs(m_RightBottom.x() - center.x());
    double halfHeight = fabs(m_RightBottom.y() - center.y());

    // 计算外椭圆的长半轴和短半轴
    double inflatedHalfWidth = halfWidth + pick_radius;
    double inflatedHalfHeight = halfHeight + pick_radius;

    // 计算内椭圆的长半轴和短半轴
    double deflatedHalfWidth = halfWidth - pick_radius;
    double deflatedHalfHeight = halfHeight - pick_radius;

    // 计算点到椭圆中心的偏移
    double dx = pos.x() - center.x();
    double dy = pos.y() - center.y();

    // 判断点是否在外椭圆内
    double outerEllipseEquation = (dx * dx) / (inflatedHalfWidth * inflatedHalfWidth) + (dy * dy) / (inflatedHalfHeight * inflatedHalfHeight);

    // 判断点是否在内椭圆内
    double innerEllipseEquation = (dx * dx) / (deflatedHalfWidth * deflatedHalfWidth) + (dy * dy) / (deflatedHalfHeight * deflatedHalfHeight);

    // 如果点在外椭圆内，但不在内椭圆内，则认为选中了椭圆的边界
    if (outerEllipseEquation <= 1.0 && innerEllipseEquation > 1.0) {
        return true;
    }

    return false;
}

// 获取包围盒
void MEllipse::GetBox(QRectF& pBox)
{
    pBox = QRectF(m_LeftTop, m_RightBottom).normalized();
}

// 平移操作
void MEllipse::Move(const QPointF& basePos, const QPointF& desPos, bool)
{
    m_LeftTop += desPos - basePos;
    m_RightBottom += desPos - basePos;
}

// 旋转操作
void MEllipse::Rotate(const QPointF& basePos, const double angle) {
    // 椭圆旋转的逻辑可以通过绕中心点旋转实现
    QPointF center = (m_LeftTop + m_RightBottom) / 2;
    ::Rotate(m_LeftTop, center, angle);
    ::Rotate(m_RightBottom, center, angle);
}

// 镜像操作
void MEllipse::Mirror(const QPointF& pos1, const QPointF& pos2) {
    ::Mirror(m_LeftTop, pos1, pos2);
    ::Mirror(m_RightBottom, pos1, pos2);
}

// 串行化
void MEllipse::Serialize(QDataStream& ar, bool bSave) {

    MEntity::Serialize(ar, bSave);

    if (bSave) {
        ar << m_LeftTop << m_RightBottom;
    }
    else {
        ar >> m_LeftTop >> m_RightBottom;
    }
}

MEntity* MEllipse::Copy()
{
    return new MEllipse(*this);
}

bool MEllipse::GetSnapPos(QPointF& pos) {
    // 椭圆的中心点
    QPointF center = (m_LeftTop + m_RightBottom) / 2;

    // 判断 pos 是否接近椭圆的中心
    if (::Distance(pos, center) < SNAP_DIS) {
        pos = center;
        return true;
    }

    // 其他点捕捉逻辑可以根据需求添加，例如椭圆的顶点
    return false;
}

void MEllipse::LoadPmtCursor() {
    // 假设我们定义了一个椭圆的光标资源 IDC_PROMPT_ELLIPSE
    //::SetCursor(AfxGetApp()->LoadCursor(IDC_PROMPT_ELLIPSE));
}

//bool MEllipse::IsInRect(const QPointF& rectBegin, const QPointF& rectEnd){
//    QRect rect(rectBegin.toPoint(), rectEnd.toPoint()); // 创建并规范化QRect
//    QRect ellipseBox;
//    GetBox(ellipseBox); // 获取椭圆的边界框
//    return rect.intersects(ellipseBox); // 检查两个矩形是否相交
//}

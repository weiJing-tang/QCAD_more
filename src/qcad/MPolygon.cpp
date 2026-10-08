#include <QIODevice>
#include "math.h"
#include "Entity.h"

MPolygon::MPolygon()
{
    Init();
}

MPolygon::MPolygon(const MPolygon& polygon)
    : MEntity(polygon)
{
    Init();
    m_begin = polygon.m_begin;
    m_end = polygon.m_end;
    m_numpts = polygon.m_numpts;
    m_pPositions = new QPointF[m_numpts];
    for (int i = 0; i < m_numpts; ++i) {
        m_pPositions[i] = polygon.m_pPositions[i];
    }
}

MPolygon::MPolygon(const QPointF& begin, const QPointF& end)
{
    Init();
    m_begin = begin;
    m_end = end;
}

MPolygon::MPolygon(int num, QPointF* pPos)
{
    Init();
    m_numpts = num;
    m_pPositions = new QPointF[num];
    for (int i = 0; i < num; ++i) {
        m_pPositions[i] = pPos[i];
    }
}

MPolygon::MPolygon(QVector<QPointF> pPositions)
{
    Init();
    m_numpts = pPositions.size();
    m_pPositions = new QPointF[m_numpts];
    for (int i = 0; i < m_numpts; ++i) {
        m_pPositions[i] = pPositions[i];
    }
}

MPolygon::~MPolygon()
{
    delete[] m_pPositions;
}

MPolygon& MPolygon::operator = (const MPolygon& polygon)
{
    // 处理自赋值
    if (this == &polygon)
        return *this;

    // 调用基类的赋值运算符
    MEntity::operator = (polygon);

    m_begin = polygon.m_begin;
    m_end = polygon.m_end;
    delete[] m_pPositions;
    m_numpts = polygon.m_numpts;
    m_pPositions = new QPointF[m_numpts];
    for (int i = 0; i < m_numpts; ++i) {
        m_pPositions[i] = polygon.m_pPositions[i];
    }

    return *this;
}

MEntity* MPolygon::Copy()
{
    return new MPolygon(*this);
}

void MPolygon::Init()
{
    MEntity::Init();
    m_type = etPolygon;
    m_begin = QPointF(0, 0);
    m_end = QPointF(0, 0);
    m_pPositions = nullptr;
    m_numpts = 0;
}

void MPolygon::SetAttrib(QPen* pPen, QBrush* pBrush)
{
    m_pen = *pPen;
    // 多边形可以使用画刷来填充
    m_brush = *pBrush;
}

int MPolygon::GetType()
{
    return etPolygon;
}

QPointF MPolygon::GetBeginPos()
{
    return m_begin;
}

QPointF MPolygon::GetEndPos()
{
    return m_end;
}

void MPolygon::Draw(QCADView* pView, int drawMode)
{
    QPainter pDC(pView);
    // 需要检查 m_pPositions 是否有足够的点来绘制多边形
    if (m_numpts < 3) return;

    QPolygon polygon;
    for (int i = 0; i < m_numpts; ++i) {
        polygon << pView->WorldtoScreen(m_pPositions[i]);
    }

    // 设置画笔和画刷
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

    // 绘制多边形
    pDC.drawPolygon(polygon);
}

bool MPolygon::Pick(const QPointF& pos, const double pick_radius)
{
    QRectF sourceBox, desBox;
    GetBox(sourceBox); // 得到多边形的最小包围盒
    // 将最小包围盒向四周放大，得到测试包围盒
    desBox.setLeft(sourceBox.left() - pick_radius);
    desBox.setBottom(sourceBox.bottom() - pick_radius);
    desBox.setRight(sourceBox.right() + pick_radius);
    desBox.setTop(sourceBox.top() + pick_radius);

    // 判断拾取点是否在测试包围盒中，如果不是，则图元未被选中
    if (pos.x() < desBox.left() || pos.x() > desBox.right() ||
        pos.y() < desBox.bottom() || pos.y() > desBox.top())
        return false;

    // 检查点是否在多边形边上
    for (int i = 0; i < m_numpts; ++i) {
        QPointF pt1 = m_pPositions[i];
        QPointF pt2 = m_pPositions[(i + 1) % m_numpts];
        // 移除 const 修饰符，创建一个非常量的临时对象
        double dist = Distance(const_cast<QPointF&>(pos), pt1);
        if (dist <= pick_radius) return true;

        dist = Distance(const_cast<QPointF&>(pos), pt2);
        if (dist <= pick_radius) return true;

        // 计算线段与点的距离
        double angle = GetAngleToXAxis(pt1, pt2);
        QPointF dp = pos - pt1;
        double len = Distance(const_cast<QPointF&>(pos), pt1);
        double projectionDistance = fabs(dp.x() * cos(angle) + dp.y() * sin(angle) - len);
        if (projectionDistance <= pick_radius &&
            (pt1.y() <= pos.y() && pt2.y() >= pos.y() ||
                pt1.y() >= pos.y() && pt2.y() <= pos.y()) &&
            (pos.x() <= pt1.x() + (pt2.x() - pt1.x()) * (pos.y() - pt1.y()) / (pt2.y() - pt1.y())))
            return true;
    }

    return false;
}


void MPolygon::GetBox(QRectF& pBox)
{
    double min0 = 1E20;
    double min1 = 1E20;
    double max0 = -1E20;
    double max1 = -1E20;

    for (int i = 0; i < m_numpts; ++i) {
        min0 = fmin(min0, m_pPositions[i].x());
        min1 = fmin(min1, m_pPositions[i].y());
        max0 = fmax(max0, m_pPositions[i].x());
        max1 = fmax(max1, m_pPositions[i].y());
    }
    pBox.setLeft(min0);
    pBox.setBottom(min1);
    pBox.setRight(max0);
    pBox.setTop(max1);
}

void MPolygon::Move(const QPointF& basePos, const QPointF& desPos, bool bTemp)
{
    for (int i = 0; i < m_numpts; ++i) {
        ::Offset(m_pPositions[i], desPos - basePos);
    }
    ::Offset(m_begin, desPos - basePos);
    ::Offset(m_end, desPos - basePos);
}

void MPolygon::Rotate(const QPointF& bp, const double angle)
{
    QPointF basePos(bp);
    for (int i = 0; i < m_numpts; ++i) {
        ::Rotate(m_pPositions[i], basePos, angle);
    }
    ::Rotate(m_begin, basePos, angle);
    ::Rotate(m_end, basePos, angle);
}

void MPolygon::Mirror(const QPointF& pos1, const QPointF& pos2)
{
    for (int i = 0; i < m_numpts; ++i) {
        ::Mirror(m_pPositions[i], pos1, pos2);
    }
    ::Mirror(m_begin, pos1, pos2);
    ::Mirror(m_end, pos1, pos2);
}

void MPolygon::LoadPmtCursor()
{
    //设置鼠标样式，需要传入窗口的指针
    //::SetCursor(AfxGetApp()->LoadCursor(IDC_PROMPT_LINE));
}

bool MPolygon::GetSnapPos(QPointF& pos)
{
    bool ret = false;
    for (int i = 0; i < m_numpts; ++i) {
        if (::Distance(pos, m_pPositions[i]) < SNAP_DIS) {
            pos = m_pPositions[i];
            ret = true;
            break;
        }
    }
    return ret;
}

void MPolygon::Serialize(QDataStream& ar, bool bSave)
{
    MEntity::Serialize(ar, bSave);
    if (bSave) {
        ar << m_numpts;
        for (int i = 0; i < m_numpts; ++i) {
            ar << m_pPositions[i];
        }
    }
    else {
        ar >> m_numpts;
        if (m_numpts < 0 || m_numpts > 1000000 ||
            qint64(m_numpts)*16 > ar.device()->bytesAvailable()) {
            m_numpts = 0; ar.setStatus(QDataStream::ReadCorruptData); return;
        }
        delete[] m_pPositions;
        m_pPositions = new QPointF[m_numpts];
        for (int i = 0; i < m_numpts; ++i) {
            ar >> m_pPositions[i];
        }
    }
}

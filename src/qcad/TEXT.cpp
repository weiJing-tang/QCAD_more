#include "ENTITY.H"



MText::MText()
{
	Init();
}

MText::MText(const MText& text)
	: MEntity(text)
{
	Init();
	m_Font = text.m_Font;
    m_TextColor = text.m_TextColor;
    m_Text = text.m_Text;
	m_LeftTop = text.m_LeftTop;
	m_RightBottom = text.m_RightBottom;
}

MText::MText(const QPointF& leftTop, const QPointF& rightBottom, const char* text)
{
	Init();
	m_LeftTop = leftTop;
	m_RightBottom = rightBottom;
	m_Text = text;
}

void MText::Init()
{
	MEntity::Init();
	m_type = etText;
	m_LeftTop = QPointF(0, 0);
	m_RightBottom = QPointF(0, 0);
	m_Text = "";
    m_TextColor = Qt::black;
    m_Font = QFont(QStringLiteral("Microsoft YaHei"), 12);
}

MText::~MText()
{
}

MText& MText::operator = (const MText& text)
{
	// 处理特殊情况：text1 = text1
	if (this == &text)
		return *this;
	// 一般情形：text2 = text1
	MEntity::operator = (text); // 调用基类的重载“=”操作
	m_Font = text.m_Font;
    m_TextColor = text.m_TextColor;
    m_Text = text.m_Text;
	m_LeftTop = text.m_LeftTop;
	m_RightBottom = text.m_RightBottom;

	return *this;
}

MEntity* MText::Copy()
{
    return new MText(*this);
}

int MText::GetType()
{
	return m_type;
}

void MText::SetAttrib(QPen* pPen, QBrush* pBrush)
{
	m_pen = *pPen;
	m_brush = *pBrush;
}

QPointF MText::GetLeftTopPos()
{
	return m_LeftTop;
}

void MText::SetLeftTopPos(QPointF pos)
{
	m_LeftTop = pos;
}

QPointF MText::GetRightBottomPos()
{
	return m_RightBottom;
}

void MText::SetRightBottomPos(QPointF pos)
{
	m_RightBottom = pos;
}

QString MText::GetText()
{
	return m_Text;
}

void MText::SetText(const char* text)
{
	m_Text = text;
}

void MText::SetFont(QFont font)
{
	m_Font = font;
}

QColor MText::GetTextColor()
{
	return m_TextColor;
}

void MText::SetTextColor(QColor color)
{
	m_TextColor = color;
}


void MText::Draw(QCADView* pView, int drawMode)
{
    QPainter painter(pView);
    const QRect rect = QRect(pView->WorldtoScreen(m_LeftTop), pView->WorldtoScreen(m_RightBottom)).normalized();
    QFont displayFont = m_Font;
    if (displayFont.pointSizeF() > 0) displayFont.setPointSizeF(displayFont.pointSizeF()*pView->GetScale());
    painter.setFont(displayFont);
    painter.setPen(m_TextColor);
    painter.drawText(rect, Qt::AlignCenter | Qt::TextWordWrap, m_Text);
    if (drawMode == dmSelect) {
        painter.setPen(QPen(Qt::darkGreen, 1, Qt::DashLine));
        painter.drawRect(rect);
    }
}

bool MText::Pick(const QPointF& pos, const double pick_radius)
{
    return QRectF(m_LeftTop, m_RightBottom).normalized().adjusted(-pick_radius,-pick_radius,pick_radius,pick_radius).contains(pos);
}

void MText::GetBox(QRectF& pBox)
{
    pBox = QRectF(m_LeftTop, m_RightBottom).normalized();
}

void MText::Move(const QPointF& basePos, const QPointF& desPos, bool bTemp)
{
	::Offset(m_LeftTop, desPos - basePos);
	::Offset(m_RightBottom, desPos - basePos);
}

void MText::Rotate(const QPointF& basePos, const double angle)
{
	// TODO
}

void MText::Mirror(const QPointF& pos1, const QPointF& pos2)
{
	::Mirror(m_LeftTop, pos1, pos2);
	::Mirror(m_RightBottom, pos1, pos2);
}

bool MText::GetSnapPos(QPointF& pos)
{
	bool res = false;

	QVector<QPointF> pPositions;
	pPositions.push_back(m_LeftTop);
	pPositions.push_back(m_RightBottom);

	for (auto& point : pPositions) {
		if (::Distance(pos, point) < SNAP_DIS) {
			pos = point;
			res = true;
			break;
		}
	}

	return res;
}

void MText::LoadPmtCursor()
{
	// TODO
}

void MText::Serialize(QDataStream& ar, bool bSave)
{
	MEntity::Serialize(ar, bSave);

	if (bSave) {
		ar << m_LeftTop << m_RightBottom << m_Text << m_Font << m_TextColor;
	}
	else {
		ar >> m_LeftTop >> m_RightBottom >> m_Text >> m_Font >> m_TextColor;
	}
}

#ifndef _ENTITY_H_
#define _ENTITY_H_

#include <QVector>
#include <vector>
#include <cmath>
#include <QPainter>
#include <QPen>
#include <QBrush>
#include <QPointF>
#include <QRect>
#include <QDataStream>
#include <QBitmap>
#include <QList>
#include "QCADView.h"

class QCADView;

#define PI 3.14159265
#define PICK_RADIUS 1 // 选取半径
#define DISTANCE_ZERO 1E-6 //距离等于0
#define ENTITY_NUM	12 // 图元的种类数
#define SNAP_DIS 3   //捕捉精度

enum EEntityType // 图元的类型
{
	etUnknow	= 0,
	etPoint		= 1,
	etLine		= 2,
	etRectangle = 3,
	etCircle	= 4,
	etArc		= 5,
	etBspline	= 6,
	etEllipse	= 7,
	etText		= 8,
	etPolygon   = 9,
	etBlock     = 10,
	etConnect   = 11
};

enum EDrawMode // 绘图模式
{
	dmNormal = 0, // 正常状态
	dmSelect = 1, // 选中状态
	dmPrompt = 2, // 提示状态
	dmDrag   = 3, // 拖动状态
	dmInvalid = 4 // 擦除状态
};

//////////////////////////
// define API function
//
void Rotate(QPointF& pos, QPointF& basePos, double angle);
void Offset(QPointF& pnt, QPointF off);
void Offset(QPointF& pnt, double deltax, double deltay);
void Mirror(QPointF& pos, QPointF p1, QPointF p2);
extern double Distance(QPointF& posSrc1, QPointF& posSrc2);
extern double GetAngleToXAxis(QPointF& posSrc1, QPointF& posSrc2);
extern void SetDrawEnvir(QPainter* pDC, int drawMode);
extern void SetDrawEnvir(QPainter*	pDC, int drawMode, QPen* pPen);

class MEntity //: public QObject
{
protected:
	// 成员变量
	int			m_type;		// 图元类型（EEntityType）
	QPen        m_pen;
	QBrush      m_brush;    //不是所有实体都要

public:
	int			m_nOperationNum;  //第几次操作声称的
	//bool		m_bErased;        //实体临时删除标志
	MEntity*   next;
	MEntity*   GetShowEnt();      //获得显示的实例
	MEntity*   GetCurrentEnt();
	MEntity*	Erase();
	void		Remove();

public:
	// 构造函数和析构函数
	MEntity() ;
	MEntity(const MEntity& entity);
	virtual ~MEntity();

	MEntity operator = (const MEntity& entity); // 重载等号
	virtual	MEntity*	Copy() { return NULL; } // 指针拷贝

	virtual	void	Init(); // 初始化成员变量值
	virtual void    SetAttrib(QPen* pPen, QBrush* pBrush); //设置画笔画刷
	virtual	int		GetType() { return m_type; }  // 返回图元的类型（EEntityType）
	QColor		    GetColor() { return m_pen.color(); } // 返回图元颜色
	void			SetColor(QColor &color) { m_pen.setColor(color); } // 设置图元颜色
	void            SetWidth(int nWidth) { m_pen.setWidth(nWidth); }
	void			Append(MEntity* pEnt);

	virtual void Draw(QCADView* pView, int drawMode = dmNormal ) {}; // 绘制图元对象

	// 给定一点及拾取半径，判断图元是否被选中
	virtual bool Pick(const QPointF& pos, const double pick_radius) { return false; }
	// 得到对象的最小包围盒，该包围盒将被用于图元的选取和全屏显示
	virtual void GetBox(QRectF& pBox){}
	// 给定一个基点和一个目标点平移图元
	virtual void Move(const QPointF& basePos,const QPointF& desPos, bool bTemp = false) {}
	// 给定一个基点和一个角度值旋转图元
	virtual void Rotate(const QPointF& basePos, const double angle) {}
	// 给定两点镜像图元，布尔变量bCopy用于确定是否删除原来位置的图元
	virtual void Mirror(const QPointF& pos1, const QPointF& pos2){}
	// 改变光标
	virtual	void LoadPmtCursor() {}
	// Get a feature position of a entity near the giving position.
	// Input : giving a pos
	// output : TRUE or false
	// Note : if true , the giving pos is reset to the feature position
	virtual bool GetSnapPos(QPointF& pos) { return false; }

	virtual void Serialize(QDataStream& ar, bool bSave) ; // 图元对象串行化

	void SetPen(QPen& pen);
	void SetBrush(QBrush& brush);
	QPen GetPen() const;
	QBrush GetBrush() const;
};

//直线，实例
class MLine : public MEntity
{
protected:
	QPointF	m_begin, m_end; // 起点和终点

public:
	MLine();
	MLine(const MLine& line);
	MLine(const QPointF& begin, const QPointF& end);
	~MLine();

	MLine& operator = (const MLine& line);
	MEntity* Copy();

	int			GetType();
	void		Init();
	void        SetAttrib(QPen* pPen, QBrush* pBrush);
	QPointF		GetBeginPos(); // 返回起点值
	QPointF		GetEndPos();   // 返回终点值

	void Draw(QCADView* pView, int drawMode = dmNormal);

	//对直线的编辑操作：拾取，平移，旋转，镜向和获得最小包围盒
	bool Pick(const QPointF& pos, const double pick_radius);
	void GetBox(QRectF& pBox);

	void Move(const QPointF& basePos, const QPointF& desPos, bool bTemp = false);
	void Rotate(const QPointF& basePos, const double angle);
	void Mirror(const QPointF& pos1, const QPointF& pos2);

	bool GetSnapPos(QPointF& pos);
	void LoadPmtCursor();

	void Serialize(QDataStream& ar, bool bSave);

	//增加接口
	void SetEndPos(QPointF& pos) { m_end = pos;  }
};

////////////////////////////
// CLASS MLines
//多线
class MLines : public MEntity
{
protected:
	QPointF	m_begin , m_end ; // 起点和终点
	//QPointF *  m_pPositions;
	QVector<QPointF> m_pPositions;
	//int			m_numpts;

public:
	MLines() ;
	MLines(const MLines& line);
	MLines(const QPointF& begin,const QPointF& end);
	MLines(std::vector<QPointF>& poses);
	MLines(int numpts, QPointF* poses);
	~MLines() ;

	MLines&	operator = (const MLines& line);
	MEntity*	Copy();

	int			GetType();
	void		Init();
    void    SetAttrib(QPen* pPen, QBrush* pBrush);
	QPointF	GetBeginPos(); // 返回起点值
	QPointF	GetEndPos();   // 返回终点值

	void Draw(QCADView* pView, int drawMode = dmNormal ) ;

	//对直线的编辑操作：拾取，平移，旋转，镜向和获得最小包围盒
	bool Pick(const QPointF& pos, const double pick_radius) ;
	void GetBox(QRectF& pBox);

	void Move(const QPointF& basePos,const QPointF& desPos, bool bTemp = false);
	void Rotate(const QPointF& basePos, const double angle);
	void Mirror(const QPointF& pos1, const QPointF& pos2);

	bool GetSnapPos(QPointF& pos);
	void LoadPmtCursor();

	void Serialize(QDataStream& ar, bool bSave) ;

	//增加接口
	int GetNumOfPoints() const {
		//return m_numpts;
		return m_pPositions.count();
	}
	QVector<QPointF> GetPoints() const {
		//return m_pPositions;
		return m_pPositions;
	}
};

////////////////////////////
// CLASS MConnect
//
class MConnect : public MEntity
{
protected:
	QPointF	m_begin , m_end ; // 起点和终点
	QPointF *  m_pPositions;
	int			m_numpts;
public:
	MConnect() ;
	MConnect(const MConnect& line);
	MConnect(const QPointF& begin,const QPointF& end);
	MConnect(int num, QPointF * pPos);
	~MConnect() ;

	MConnect&	operator = (const MConnect& line);
	MEntity*	Copy();
	void	    Manhadunize();

	int			GetType();
	void		Init();
	void    SetAttrib(QPen* pPen, QBrush* pBrush);
	QPointF	GetBeginPos(); // 返回起点值
	QPointF	GetEndPos();   // 返回终点值
	//void MConnect::SetBeginPos(const QPointF & pos);
	//void MConnect::SetEndPos(const QPointF & pos);

	void Draw(QCADView* pView, int drawMode = dmNormal ) ;

	//对直线的编辑操作：拾取，平移，旋转，镜向和获得最小包围盒
	bool Pick(const QPointF& pos, const double pick_radius) ;
	void GetBox(QRectF& pBox);

	void Move(const QPointF& basePos,const QPointF& desPos, bool bTemp = false);
	void Rotate(const QPointF& basePos, const double angle);
	void Mirror(const QPointF& pos1, const QPointF& pos2);

	bool GetSnapPos(QPointF& pos);
	void LoadPmtCursor();

	void Serialize(QDataStream& ar, bool bSave) ;
};

////////////////////////////
// CLASS MLines
//
class MPolygon : public MEntity
{
protected:
	QPointF	m_begin , m_end ; // 起点和终点
	QPointF *  m_pPositions;
	int			m_numpts;
public:
	MPolygon() ;
	MPolygon(const MPolygon& line);
	MPolygon(const QPointF& begin,const QPointF& end);
	MPolygon(int num, QPointF * pPos);
	MPolygon(QVector<QPointF> m_pPositions);
	~MPolygon() ;

	MPolygon&	operator = (const MPolygon& line);
	MEntity*	Copy();

	int			GetType();
	void		Init();
	void    SetAttrib(QPen* pPen, QBrush* pBrush);
	QPointF	GetBeginPos(); // 返回起点值
	QPointF	GetEndPos();   // 返回终点值

	void Draw(QCADView* pView, int drawMode = dmNormal ) ;

	//对直线的编辑操作：拾取，平移，旋转，镜向和获得最小包围盒
	bool Pick(const QPointF& pos, const double pick_radius) ;
	void GetBox(QRectF& pBox);

	void Move(const QPointF& basePos,const QPointF& desPos, bool bTemp = false);
	void Rotate(const QPointF& basePos, const double angle);
	void Mirror(const QPointF& pos1, const QPointF& pos2);

	bool GetSnapPos(QPointF& pos);
	void LoadPmtCursor();

	void Serialize(QDataStream& ar, bool bSave) ;
};

////////////////////////////
// CLASS MRectangle
//
class MRectangle : public MEntity
{
protected:
	QPointF	m_LeftTop , m_RightBottom ; // 起点和终点
public:
	MRectangle() ;
	MRectangle(const MRectangle& rect);
	MRectangle(const QPointF& LeftTop,const QPointF& RightBottom);
	~MRectangle() ;

	MRectangle&	operator = (const MRectangle& rect);
	MEntity*	Copy();

	int			GetType();
	void		Init();
	void    SetAttrib(QPen* pPen, QBrush* pBrush);
	QPointF	GetLeftTopPos();		// 返回左上角的值
	QPointF	GetRightBottomPos();	// 返回右下角的值

	void Draw(QCADView* pView, int drawMode = dmNormal ) ;

	//对直线的编辑操作：拾取，平移，旋转，镜向和获得最小包围盒
	bool Pick(const QPointF& pos, const double pick_radius) ;
	void GetBox(QRectF& pBox);

	void Move(const QPointF& basePos,const QPointF& desPos, bool bTemp = false);
	void Rotate(const QPointF& basePos, const double angle);
	void Mirror(const QPointF& pos1, const QPointF& pos2);

	bool GetSnapPos(QPointF& pos);
	void LoadPmtCursor() ;

	void Serialize(QDataStream& ar, bool bSave) ;
};

////////////////////////////
// CLASS MBlock
//
class MBlock : public MEntity
{
protected:
	QPointF	m_Import , m_Outport ; // 输入和输出
	QPointF	m_LeftTop , m_RightBottom ; // 起点和终点

	//CString		m_EntText;             //实体
	//CObList		m_EntList;             //实体连标
	QBitmap  *  m_Bitmap;

public:
	MBlock() ;
	MBlock(const MBlock & blk) ;
	MBlock(const QPointF & in, const QPointF & out, QBitmap * bmp);
	MBlock(const QPointF & in, const QPointF & out, const char * text);
	MBlock(const QPointF & in, const QPointF & out, QList<MEntity*> & ents);
	~MBlock() ;

	MBlock&	operator = (const MBlock& rect);
	MEntity*	Copy();

	int			GetType();
	void		Init();
	void    SetAttrib(QPen* pPen, QBrush* pBrush);
	QPointF	GetImportPos();		// 返回Import的值
	QPointF	GetOutportPos();	// 返回Outport的值
	QPointF	GetLeftTopPos();		// 返回左上角的值
	QPointF	GetRightBottomPos();	// 返回右下角的值
	QBitmap *	GetBitmap();
	void		SetBitmap(QBitmap * bmp);

	void Draw(QCADView* pView, int drawMode = dmNormal ) ;

	//对直线的编辑操作：拾取，平移，旋转，镜向和获得最小包围盒
	bool Pick(const QPointF& pos, const double pick_radius) ;
	void GetBox(QRectF& pBox);

	void Move(const QPointF& basePos,const QPointF& desPos, bool bTemp = false);
	void Rotate(const QPointF& basePos, const double angle);
	void Mirror(const QPointF& pos1, const QPointF& pos2);

	bool GetSnapPos(QPointF& pos);
	void LoadPmtCursor() ;

	void Serialize(QDataStream& ar, bool bSave) ;
};

////////////////////////////
// CLASS MText
//
class MText : public MEntity
{
protected:
	QPointF	m_LeftTop , m_RightBottom ; // 起点和终点
	QString		m_Text;                     //文本内容
	QFont  m_Font;						// 字体
	QColor m_TextColor;					// 文本颜色

public:
	MText() ;
	MText(const MText& rect);
	MText(const QPointF& LeftTop,const QPointF& RightBottom, const char * text);
	~MText() ;

	MText&	operator = (const MText& rect);
	MEntity*	Copy();

	int			GetType();
	void		Init();
	void        SetAttrib(QPen* pPen, QBrush* pBrush);
	void		SetLeftTopPos(QPointF pos);
	void		SetRightBottomPos(QPointF pos);

	QPointF	GetLeftTopPos();		// 返回左上角的值
	QPointF	GetRightBottomPos();	// 返回右下角的值
	QString		GetText();
	void		SetText(const char * text);
    void SetText(const QString& text) { m_Text = text; }
    QFont GetFont() const { return m_Font; }
	void		SetFont(QFont font);
	QColor		GetTextColor();
	void		SetTextColor(QColor color);

	void Draw(QCADView* pView, int drawMode = dmNormal ) ;

	//对直线的编辑操作：拾取，平移，旋转，镜向和获得最小包围盒
	bool Pick(const QPointF& pos, const double pick_radius) ;
	void GetBox(QRectF& pBox);

	void Move(const QPointF& basePos,const QPointF& desPos, bool bTemp = false);
	void Rotate(const QPointF& basePos, const double angle);
	void Mirror(const QPointF& pos1, const QPointF& pos2);

	bool GetSnapPos(QPointF& pos);
	void LoadPmtCursor() ;

	void Serialize(QDataStream& ar, bool bSave) ;
};

/////////////////
// Class CCircle
//
class CCircle : public MEntity
{
protected:
	//member variables
	QPointF	  m_center ;
	double	  m_dRadius ;
public:
	// constructor and destructor
	CCircle() ;
	CCircle(const CCircle& circle);
	CCircle(const QPointF& center,const double& radius);
	CCircle(const QPointF& center,const QPointF& aroud);
	~CCircle() ;

	MEntity* Copy();
	//-----------------------------------------------
	// member function
	// Attributes
	int			GetType();
	void		Init(); // initialize member variables
	void    SetAttrib(QPen* pPen, QBrush* pBrush);
	QPointF	GetCenterPos();
	double	GetRadius();
	void    SetRadius(double r);

	void Draw(QCADView* pView, int drawMode = dmNormal ) ;

	//对直线的编辑操作：拾取，平移，旋转，镜向和获得最小包围盒
	bool Pick(const QPointF& pos, const double pick_radius) ;
	void GetBox(QRectF& pBox);

	void Move(const QPointF& basePos,const QPointF& desPos, bool bTemp = false);
	void Rotate(const QPointF& basePos, const double angle);
	void Mirror(const QPointF& pos1, const QPointF& pos2);

	bool GetSnapPos(QPointF& pos);
	void LoadPmtCursor() {};

	void Serialize(QDataStream& ar, bool bSave) ;
};

/////////////////
// Class MEllipse
//
class MEllipse : public MEntity
{
protected:
	//member variables
	QPointF	m_LeftTop ;
	QPointF	m_RightBottom ;
public:
	// constructor and destructor
	MEllipse() ;
	MEllipse(const MEllipse& elps);
	MEllipse(const QPointF& lt,const QPointF& rb);
	~MEllipse() ;

	MEntity*	Copy();
	//-----------------------------------------------
	// member function
	// Attributes
	int			GetType();
	void		Init(); // initialize member variables
	void    SetAttrib(QPen* pPen, QBrush* pBrush);
	QPointF	GetLTPos();
	QPointF		GetRBPos();

	void Draw(QCADView* pView, int drawMode = dmNormal ) ;

	//对直线的编辑操作：拾取，平移，旋转，镜向和获得最小包围盒
	bool Pick(const QPointF& pos, const double pick_radius) ;
	void GetBox(QRectF& pBox);

	void Move(const QPointF& basePos,const QPointF& desPos, bool bTemp = false);
	void Rotate(const QPointF& basePos, const double angle);
	void Mirror(const QPointF& pos1, const QPointF& pos2);

	bool GetSnapPos(QPointF& pos);
	void LoadPmtCursor() ;

	void Serialize(QDataStream& ar, bool bSave) ;
};

////////////////////////////
// CLASS CArc
//
class CArc : public MEntity
{
protected:
	//member variables
	QPointF	m_center ;
	QPointF	m_begin  ;
	QPointF m_end    ;

public:
	// constructor and destructor
	CArc() ;
	CArc(const CArc& arc);
	CArc(const QPointF& center,const QPointF& startPos, const QPointF& endPos);
	~CArc() ;

	MEntity*	Copy();
	//-----------------------------------------------
	// member function
	// Attributes
	int			GetType();
	void		Init(); // initialize member variables
	void    SetAttrib(QPen* pPen, QBrush* pBrush);
	QPointF	GetStartPos();
	QPointF	GetEndPos();
	void    SetEndPos(QPointF& pt);
	QPointF	GetCenterPos() ;
	bool	GetSnapPos(QPointF& pos) ;

	void Draw(QCADView* pView, int drawMode = dmNormal ) ;

	//对直线的编辑操作：拾取，平移，旋转，镜向和获得最小包围盒
	bool Pick(const QPointF& pos, const double pick_radius) ;
	void Move(const QPointF& basePos,const QPointF& desPos, bool bTemp = false) ;
	void Rotate(const QPointF& basePos, const double angle) ;
	void Mirror(const QPointF& FstPos, const QPointF& SecPos);
	void GetBox(QRectF& pBox);

	void LoadPmtCursor() {}
	void Serialize(QDataStream& ar, bool bSave);
};
#endif

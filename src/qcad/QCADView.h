#pragma once
#include <QWidget>
#include <QList>
#include <memory>
#include <functional>

class MEntity;
class MCommand;

//图形视窗
class QCADView :
    public QWidget
{
private:
    QList<MEntity*> m_EntityList;
    QList<MEntity*> m_SelectEntityList;
    MCommand* m_pCmd;
    std::unique_ptr<MEntity> preview_;
    bool modified_ = false;

public:
    QCADView();
    ~QCADView();

protected:
    void keyPressEvent(QKeyEvent* event) override;
    void paintEvent(QPaintEvent* event);
    void resizeEvent(QResizeEvent* event);
    void mousePressEvent(QMouseEvent* mouseEvent);
    void mouseMoveEvent(QMouseEvent* mouseEvent);
    void mouseReleaseEvent(QMouseEvent* mouseEvent);
    void mouseDoubleClickEvent(QMouseEvent* mouseEvent);

public:
    void setCommand(MCommand* command);
    void setPreview(std::unique_ptr<MEntity> entity);
    bool hasPreview() const { return bool(preview_); }
    void replaceEntities(QList<MEntity*> entities);
    void setModified(bool modified);
    bool isModified() const { return modified_; }
    std::function<void(bool)> modifiedChanged;
    void addEntity(MEntity* pEnt);
    void removeEntity(MEntity* pEnt);
    void selectEntity();
    void RemoveSelection(MEntity* pEnt);
    void removeLastEntity();
    void moveEntity();
    void drawLine();
    void drawRectangle();
    void drawPolygon();
    void drawCircle();
    void drawArc();
    void drawEllipse();
    void drawText();

private:
    QColor m_lineColor;
    Qt::PenStyle m_penStyle;
    qreal  m_penWidth;
    QColor m_brushColor;
    QColor m_textColor;

    //窗口
    double m_scale; //世界坐标 / 屏幕坐标
    int m_Xorg;     //世界坐标系 原点在屏幕坐标中的位置
    int m_Yorg;

public:
    void setLineColor(QColor color) { m_lineColor = color; }
    void setPenStyle(Qt::PenStyle style) { m_penStyle = style; }
    void setPenWidth(qreal width) { m_penWidth = width; }
    void setBrushColor(QColor color) { m_brushColor = color; }
    void setTextColor(QColor color) { m_textColor = color; }

    QColor lineColor() { return m_lineColor; }
    Qt::PenStyle penStyle() { return m_penStyle; }
    qreal penWidth() { return m_penWidth; }
    QColor brushColor() { return m_brushColor; }
    QColor textColor() { return m_textColor; }

public:
    void AddSelection(MEntity* pEnt);
    void ClearSelections() { m_SelectEntityList.clear(); }
    QList<MEntity*> GetEntityList() const { return m_EntityList; }
    QList<MEntity*> GetSelectedEntityList() const { return m_SelectEntityList; }

public:
    void zoomPan();
    void zoomWindow();
    void zoomRect(QRectF rect);
    void Scale(double scale);
    void setScale(double scale);
    void ScaleCenter(double scale, QPoint center);
    void MoveOrg(int delX, int delY);
    void setOrg(QPoint cps);
    double GetScale();
    QPoint WorldtoScreen(QPointF pos);
    QPointF ScreentoWorld(QPoint pos);
    QPointF ScreentoWorld(QPointF pos);
};

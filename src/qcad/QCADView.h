#pragma once
#include <QWidget>
#include <QList>
#include <QTransform>
#include <QCursor>
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
    QList<MEntity*> previewHiddenEntities_;
    bool modified_ = false;
    bool panning_ = false;
    QPoint panOrigin_;
    QCursor panCursor_;

public:
    QTransform drawingTransform;
    QTransform screenTransform(const QTransform& world) const;
    QCADView();
    ~QCADView();

protected:
    void wheelEvent(QWheelEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;
    void paintEvent(QPaintEvent* event);
    void resizeEvent(QResizeEvent* event);
    void mousePressEvent(QMouseEvent* mouseEvent);
    void mouseMoveEvent(QMouseEvent* mouseEvent);
    void mouseReleaseEvent(QMouseEvent* mouseEvent);
    void mouseDoubleClickEvent(QMouseEvent* mouseEvent);

public:
    void setCommand(MCommand* command);
    void setPreview(std::unique_ptr<MEntity> entity, QList<MEntity*> hiddenEntities = {});
    bool hasPreview() const { return bool(preview_); }
    void replaceEntities(QList<MEntity*> entities);
    void setModified(bool modified);
    bool isModified() const { return modified_; }
    std::function<void(bool)> modifiedChanged;
    std::function<void()> editStarted, editFinished;
    std::function<void(QPointF)> nudgeRequested;
    std::function<void(double)> zoomChanged;
    void beginEdit() { if (editStarted) editStarted(); }
    void endEdit() { if (editFinished) editFinished(); }
    void reorderEntities(QList<MEntity*> entities);
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
    double m_Xorg;
    double m_Yorg;

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
    void zoomAt(QPointF screenPoint, double factor);
    QPoint WorldtoScreen(QPointF pos);
    QPointF ScreentoWorld(QPoint pos);
    QPointF ScreentoWorld(QPointF pos);
};

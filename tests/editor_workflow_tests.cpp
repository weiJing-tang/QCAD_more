#include <QtTest>
#include <QtWidgets>
#include "qcad/mainwindow.h"
#include "storage/EntityCodec.h"
#include "storage/DocumentStorage.h"
#include "blocks/BlockEntity.h"
#include "blocks/BlockDefinitionService.h"
#include "blocks/BlockInsertionService.h"
#include "text/TextPropertiesDialog.h"
#include "text/TextPropertyService.h"
using namespace qcad_more;
namespace {
std::unique_ptr<MEntity> primitive(int type)
{
    switch(type) {
    case etLine:return std::make_unique<MLine>(QPointF(0,0),QPointF(60,30));
    case etRectangle:return std::make_unique<MRectangle>(QPointF(0,0),QPointF(60,40));
    case etCircle:return std::make_unique<CCircle>(QPointF(30,20),15);
    case etArc:return std::make_unique<CArc>(QPointF(30,20),QPointF(45,20),QPointF(30,35));
    case etEllipse:return std::make_unique<MEllipse>(QPointF(0,0),QPointF(60,40));
    case etPolygon:return std::make_unique<MPolygon>(QVector<QPointF>{{0,0},{60,0},{30,40}});
    case etText:{auto text=std::make_unique<MText>();TextPropertyService().apply(*text,
        {QStringLiteral("中文 Text"),QFont("Microsoft YaHei",12),Qt::blue,QRectF(0,0,60,40)});return text;}
    }
    return {};
}
QByteArray payload(MEntity* entity) {EntitySnapshot value;if(!encodeEntity(*entity,value).ok()) return {};return value.payload;}
QByteArray scene(QCADView* view) {
    QByteArray result;QDataStream out(&result,QIODevice::WriteOnly);
    for(auto* entity:view->GetEntityList()) out<<payload(entity);return result;
}
void trigger(MainWindow& window,const char* name) {auto* action=window.findChild<QAction*>(name);Q_ASSERT(action);action->trigger();}
void showWindow(MainWindow& window) {window.resize(1200,800);window.show();QTest::qWait(50);QApplication::setActiveWindow(&window);window.canvas()->setFocus();}
void moveMouse(QWidget* widget,QPoint point,Qt::MouseButtons buttons=Qt::LeftButton) {
    QMouseEvent event(QEvent::MouseMove,point,Qt::NoButton,buttons,Qt::NoModifier);QApplication::sendEvent(widget,&event);
}
void respondToDialog(const std::function<void(QDialog*)>& response,const std::function<void()>& operation)
{
    QTimer timer;
    QObject::connect(&timer,&QTimer::timeout,[&]{
        if(auto* dialog=qobject_cast<QDialog*>(QApplication::activeModalWidget())) response(dialog);
    });timer.start(20);operation();timer.stop();
}
}
class EditorWorkflowTests:public QObject {
    Q_OBJECT
private slots:
    void initTestCase();
    void wheelZoomAndPan();
    void ordinaryDrag_data();void ordinaryDrag();
    void clipboardAndShortcuts();
    void alignAndDistribute_data();void alignAndDistribute();
    void transformGroupAndPersistence();
    void undoDrawingBlocksTextAndFiles();
    void orderingAndStyle();
    void drawingToolsAndCancellation();
    void legacyDocuments();
};
void EditorWorkflowTests::initTestCase()
{
    QApplication::setAttribute(Qt::AA_DontUseNativeDialogs);
    if(QGuiApplication::platformName()=="offscreen") {
        QVERIFY(QFontDatabase::addApplicationFont(qEnvironmentVariable("WINDIR")+"/Fonts/msyh.ttc")>=0);
        QApplication::setFont(QFont("Microsoft YaHei",9));
    }
}
void EditorWorkflowTests::wheelZoomAndPan()
{
    MainWindow window;showWindow(window);auto* view=window.canvas();
    view->addEntity(new MLine({-100,0},{-40,20}));window.editing().history.markSaved();
    const auto drawing=scene(view);const QPointF position(203.25,167.75);
    const auto anchor=view->ScreentoWorld(position);const auto initialScale=view->GetScale();
    QWheelEvent wheel(position,view->mapToGlobal(position.toPoint()),QPoint(),QPoint(0,120),Qt::NoButton,Qt::NoModifier,Qt::NoScrollPhase,false);
    QApplication::sendEvent(view,&wheel);
    QVERIFY(view->GetScale()>initialScale);QVERIFY(QLineF(anchor,view->ScreentoWorld(position)).length()<1e-9);
    QVERIFY(!view->isModified());QCOMPARE(scene(view),drawing);
    QWheelEvent back(position,view->mapToGlobal(position.toPoint()),QPoint(),QPoint(0,-120),Qt::NoButton,Qt::NoModifier,Qt::NoScrollPhase,false);
    QApplication::sendEvent(view,&back);QVERIFY(qAbs(view->GetScale()-initialScale)<1e-9);
    QWheelEvent smooth(position,view->mapToGlobal(position.toPoint()),QPoint(0,20),QPoint(),Qt::NoButton,Qt::NoModifier,Qt::ScrollUpdate,false);
    QApplication::sendEvent(view,&smooth);QVERIFY(view->GetScale()>initialScale);
    view->zoomAt(position,1e8);QCOMPARE(view->GetScale(),100.0);
    view->zoomAt(position,1e-12);QCOMPARE(view->GetScale(),.01);
    auto* combo=window.findChild<QComboBox*>("viewScaleCombo");QVERIFY(combo);
    combo->setCurrentText("150%");QMetaObject::invokeMethod(combo->lineEdit(),"editingFinished");QCOMPARE(view->GetScale(),1.5);
    combo->setCurrentText("bad");QMetaObject::invokeMethod(combo->lineEdit(),"editingFinished");QCOMPARE(view->GetScale(),1.5);QCOMPARE(combo->currentText(),QString("150%"));
    combo->setCurrentText("0%");QMetaObject::invokeMethod(combo->lineEdit(),"editingFinished");QCOMPARE(view->GetScale(),1.5);
    const QPoint press(300,250),destination(360,280);const auto oldPoint=view->ScreentoWorld(press);
    QTest::mousePress(view,Qt::MiddleButton,{},press);moveMouse(view,destination,Qt::MiddleButton);
    QTest::mouseRelease(view,Qt::MiddleButton,{},destination);
    QVERIFY(QLineF(oldPoint,view->ScreentoWorld(destination)).length()<1e-9);
    QVERIFY(!view->isModified());QCOMPARE(scene(view),drawing);
}
void EditorWorkflowTests::ordinaryDrag_data()
{
    QTest::addColumn<int>("type");
    for(int type:{etLine,etRectangle,etCircle,etArc,etEllipse,etPolygon,etText})
        QTest::newRow(qPrintable(QString::number(type)))<<type;
}
void EditorWorkflowTests::ordinaryDrag()
{
    QFETCH(int,type);MainWindow window;showWindow(window);auto* view=window.canvas();
    auto entity=primitive(type);QTransform transform;transform.translate(-100,-20);transform.rotate(37);transform.scale(1.25,1.25);
    entity->SetTransform(transform);auto* source=entity.release();view->addEntity(source);view->AddSelection(source);
    window.editing().history.markSaved();view->setScale(1.5);view->selectEntity();
    const auto before=payload(source);QRectF box;source->WorldBox(box);
    const QPoint start=view->WorldtoScreen(box.center()),destination=start+QPoint(75,-45);
    const auto delta=view->ScreentoWorld(destination)-view->ScreentoWorld(start);
    QTest::mousePress(view,Qt::LeftButton,{},start);moveMouse(view,destination);
    QVERIFY(view->hasPreview());QCOMPARE(payload(source),before);QVERIFY(!view->isModified());
    QTest::mouseRelease(view,Qt::LeftButton,{},destination);
    QVERIFY(!view->hasPreview());QVERIFY(view->isModified());QCOMPARE(source->GetType(),type);
    QRectF moved;source->WorldBox(moved);QVERIFY(QLineF(moved.center(),box.center()+delta).length()<1e-8);
    const auto after=payload(source);trigger(window,"undoAction");QCOMPARE(payload(view->GetEntityList().first()),before);
    QVERIFY(!view->isModified());trigger(window,"redoAction");QCOMPARE(payload(view->GetEntityList().first()),after);
    source=view->GetEntityList().first();const auto unchanged=payload(source);
    source->WorldBox(moved);const QPoint next=view->WorldtoScreen(moved.center());
    QTest::mousePress(view,Qt::LeftButton,{},next);moveMouse(view,next+QPoint(50,20));QVERIFY(view->hasPreview());
    QTest::keyClick(view,Qt::Key_Escape);QTest::mouseRelease(view,Qt::LeftButton,{},next+QPoint(50,20));
    QCOMPARE(payload(source),unchanged);view->setModified(false);
}
void EditorWorkflowTests::clipboardAndShortcuts()
{
    MainWindow window;showWindow(window);auto* view=window.canvas();
    QList<MEntity*> sources;
    for(int type:{etLine,etRectangle,etCircle,etArc,etEllipse,etPolygon,etText}) {
        auto entity=primitive(type);QTransform matrix;matrix.rotate(23);entity->SetTransform(matrix);
        QPen pen(QColor("#B02E40"),2.5,Qt::DashLine);QBrush brush(QColor("#E3F4F6"));entity->SetAttrib(&pen,&brush);
        auto* raw=entity.release();view->addEntity(raw);view->AddSelection(raw);sources.push_back(raw);
    }
    QCOMPARE(window.findChild<QAction*>("copyAction")->shortcut(),QKeySequence(QKeySequence::Copy));
    trigger(window,"copyAction");trigger(window,"pasteAction");QCOMPARE(view->GetEntityList().size(),14);
    for(int i=0;i<sources.size();++i) {
        std::unique_ptr<MEntity> expected(sources[i]->Copy());expected->TranslateWorld({20,-20});
        QCOMPARE(payload(view->GetEntityList()[i+7]),payload(expected.get()));
    }
    trigger(window,"undoAction");QCOMPARE(view->GetEntityList().size(),7);trigger(window,"redoAction");QCOMPARE(view->GetEntityList().size(),14);
    trigger(window,"duplicateAction");QCOMPARE(view->GetEntityList().size(),21);
    trigger(window,"cutAction");QCOMPARE(view->GetEntityList().size(),14);trigger(window,"undoAction");QCOMPARE(view->GetEntityList().size(),21);
    window.editing().history.markSaved();trigger(window,"newFileAction");QVERIFY(view->GetEntityList().isEmpty());
    trigger(window,"pasteAction");QCOMPARE(view->GetEntityList().size(),7);
    const auto selected=view->GetSelectedEntityList();QRectF before;selected.first()->WorldBox(before);
    QTest::keyClick(view,Qt::Key_Right);QRectF after;view->GetSelectedEntityList().first()->WorldBox(after);
    QVERIFY(QLineF(after.center(),before.center()+QPointF(1,0)).length()<1e-8);
    QTest::keyClick(view,Qt::Key_Up,Qt::ShiftModifier);view->GetSelectedEntityList().first()->WorldBox(after);
    QVERIFY(QLineF(after.center(),before.center()+QPointF(1,10)).length()<1e-8);
    trigger(window,"deleteAction");QVERIFY(view->GetEntityList().isEmpty());trigger(window,"undoAction");QCOMPARE(view->GetEntityList().size(),7);
    // Block clipboard brings its definitions into a new document and gives copies new instance IDs.
    view->ClearSelections();for(auto* entity:view->GetEntityList()) view->AddSelection(entity);
    trigger(window,"groupAction");auto* block=dynamic_cast<BlockEntity*>(view->GetEntityList().first());QVERIFY(block);
    const auto oldId=block->instance.instanceId;trigger(window,"copyAction");
    window.editing().history.markSaved();trigger(window,"newFileAction");trigger(window,"pasteAction");
    block=dynamic_cast<BlockEntity*>(view->GetEntityList().first());QVERIFY(block);QVERIFY(block->instance.instanceId!=oldId);
    QCOMPARE(window.blockLibrary().definitions.size(),1);
    QTemporaryDir directory;QVERIFY(DocumentStorage().save(directory.filePath("clipboard.qbl"),*view,window.blockLibrary()).ok());
    view->setModified(false);
}
void EditorWorkflowTests::alignAndDistribute_data()
{
    QTest::addColumn<QByteArray>("action");
    for(const char* action:{"alignLeftAction","alignHCenterAction","alignRightAction","alignTopAction","alignVCenterAction","alignBottomAction","distributeHorizontalAction","distributeVerticalAction"})
        QTest::newRow(action)<<QByteArray(action);
}
void EditorWorkflowTests::alignAndDistribute()
{
    QFETCH(QByteArray,action);MainWindow window;showWindow(window);auto* view=window.canvas();
    const QList<QRectF> rectangles={QRectF(0,0,10,20),QRectF(35,40,20,30),QRectF(100,100,30,15)};
    for(const auto& box:rectangles) {auto* entity=new MRectangle(box.topLeft(),box.bottomRight());view->addEntity(entity);view->AddSelection(entity);}
    const auto before=scene(view);trigger(window,action.constData());QVector<QRectF> boxes;
    for(auto* entity:view->GetEntityList()) {QRectF box;entity->WorldBox(box);boxes.push_back(box);}
    if(action=="alignLeftAction") for(const auto& box:boxes) QCOMPARE(box.left(),0.0);
    else if(action=="alignRightAction") for(const auto& box:boxes) QCOMPARE(box.right(),130.0);
    else if(action=="alignHCenterAction") for(const auto& box:boxes) QCOMPARE(box.center().x(),65.0);
    else if(action=="alignTopAction") for(const auto& box:boxes) QCOMPARE(box.bottom(),115.0);
    else if(action=="alignBottomAction") for(const auto& box:boxes) QCOMPARE(box.top(),0.0);
    else if(action=="alignVCenterAction") for(const auto& box:boxes) QCOMPARE(box.center().y(),57.5);
    else if(action=="distributeHorizontalAction") {
        QCOMPARE(boxes[1].left()-boxes[0].right(),boxes[2].left()-boxes[1].right());
        QCOMPARE(boxes[0],rectangles[0]);QCOMPARE(boxes[2],rectangles[2]);
    } else {
        QCOMPARE(boxes[1].top()-boxes[0].bottom(),boxes[2].top()-boxes[1].bottom());
        QCOMPARE(boxes[0],rectangles[0]);QCOMPARE(boxes[2],rectangles[2]);
    }
    const auto after=scene(view);trigger(window,"undoAction");QCOMPARE(scene(view),before);
    trigger(window,"redoAction");QCOMPARE(scene(view),after);view->setModified(false);
}
void EditorWorkflowTests::transformGroupAndPersistence()
{
    MainWindow window;showWindow(window);auto* view=window.canvas();
    for(int type:{etLine,etRectangle,etCircle,etArc,etEllipse,etPolygon,etText}) {
        auto entity=primitive(type);auto* raw=entity.release();view->addEntity(raw);view->AddSelection(raw);
    }
    const auto initial=scene(view);QVector<int> types;for(auto* entity:view->GetEntityList()) types.push_back(entity->GetType());
    trigger(window,"rotateRightAction");trigger(window,"flipHorizontalAction");trigger(window,"flipVerticalAction");
    respondToDialog([](QDialog* dialog) {
        if(dialog->objectName()=="selectionTransformDialog") {
            dialog->findChild<QDoubleSpinBox*>("selectionRotation")->setValue(37);
            dialog->findChild<QDoubleSpinBox*>("selectionScale")->setValue(1.5);dialog->accept();
        }
    },[&]{trigger(window,"selectionTransformAction");});
    const auto transformed=scene(view);QVERIFY(transformed!=initial);
    trigger(window,"groupAction");QCOMPARE(view->GetEntityList().size(),1);QCOMPARE(window.blockLibrary().definitions.size(),1);
    auto* group=dynamic_cast<BlockEntity*>(view->GetEntityList().first());QVERIFY(group);QCOMPARE(group->children.size(),size_t(7));
    trigger(window,"rotateLeftAction");window.editing().transform(0,2);
    group=dynamic_cast<BlockEntity*>(view->GetEntityList().first());const auto parent=group->worldTransform()*group->GetTransform();
    QVector<QTransform> expected;
    for(const auto& child:group->children) expected.push_back(child->GetTransform()*parent);
    trigger(window,"ungroupAction");QCOMPARE(view->GetEntityList().size(),7);
    for(int i=0;i<types.size();++i) {QCOMPARE(view->GetEntityList()[i]->GetType(),types[i]);QCOMPARE(view->GetEntityList()[i]->GetTransform(),expected[i]);}
    const auto ungrouped=scene(view);trigger(window,"undoAction");QCOMPARE(view->GetEntityList().size(),1);trigger(window,"redoAction");QCOMPARE(scene(view),ungrouped);
    QTemporaryDir directory;const auto path=directory.filePath("transforms.qbl");
    QVERIFY(DocumentStorage().save(path,*view,window.blockLibrary()).ok());QCADView loaded;BlockLibrary library;
    QVERIFY(DocumentStorage().load(path,loaded,library).ok());QCOMPARE(scene(&loaded),ungrouped);
    // Definition + insertion preserves transformed primitive types and exact snapshots.
    QUuid definition,id;
    QVERIFY(BlockDefinitionService().defineBlock(view->GetEntityList(),{"after-ungroup",{0,0}},window.blockLibrary(),definition).ok());
    QVERIFY(BlockInsertionService().insertAt(*view,window.blockLibrary(),{definition,{-180,-80},15,.8},id).ok());
    QDir().mkpath("artifacts");view->zoomRect(QRectF(-300,-200,600,450));
    QVERIFY(window.grab().save("artifacts/common-editing.png"));view->setModified(false);
}
void EditorWorkflowTests::undoDrawingBlocksTextAndFiles()
{
    MainWindow window;showWindow(window);auto* view=window.canvas();auto at=[view](QPointF p){return view->WorldtoScreen(p);};
    view->drawRectangle();QTest::mouseClick(view,Qt::LeftButton,{},at({-150,50}));QTest::mouseClick(view,Qt::LeftButton,{},at({-100,90}));
    trigger(window,"undoAction");QVERIFY(view->GetEntityList().isEmpty());trigger(window,"redoAction");QCOMPARE(view->GetEntityList().size(),1);
    auto* text=new MText;QVERIFY(TextPropertyService().apply(*text,{"before",QFont("Arial",12),Qt::blue,QRectF(-140,60,30,20)}).ok());
    view->addEntity(text);view->selectEntity();view->AddSelection(text);
    respondToDialog([](QDialog* active){if(auto* dialog=dynamic_cast<TextPropertiesDialog*>(active)) {
        dialog->findChild<QPlainTextEdit*>("textContent")->setPlainText("after");dialog->accept();}},
        [&]{trigger(window,"textPropertiesAction");});
    QCOMPARE(dynamic_cast<MText*>(view->GetEntityList().last())->GetText(),QString("after"));
    trigger(window,"undoAction");QCOMPARE(dynamic_cast<MText*>(view->GetEntityList().last())->GetText(),QString("before"));trigger(window,"redoAction");
    view->ClearSelections();for(auto* entity:view->GetEntityList()) view->AddSelection(entity);
    respondToDialog([](QDialog* active){if(auto* dialog=qobject_cast<QInputDialog*>(active)) {dialog->setTextValue("undo-block");dialog->accept();}},
        [&]{trigger(window,"defineBlockAction");});
    QTest::mouseClick(view,Qt::LeftButton,{},at({-150,50}));QCoreApplication::processEvents();
    QCOMPARE(window.blockLibrary().definitions.size(),1);trigger(window,"undoAction");QVERIFY(window.blockLibrary().definitions.isEmpty());trigger(window,"redoAction");
    QTimer dialogs;connect(&dialogs,&QTimer::timeout,[] {
        if(auto* input=qobject_cast<QInputDialog*>(QApplication::activeModalWidget())) input->accept();
        else if(auto* dialog=qobject_cast<QDialog*>(QApplication::activeModalWidget())) dialog->accept();
    });dialogs.start(20);trigger(window,"insertBlockAction");dialogs.stop();
    QTest::mouseClick(view,Qt::LeftButton,{},at({50,50}));QTest::keyClick(view,Qt::Key_Escape);
    QCOMPARE(view->GetEntityList().size(),3);trigger(window,"undoAction");QCOMPARE(view->GetEntityList().size(),2);trigger(window,"redoAction");
    QTemporaryDir directory;const auto file=directory.filePath("saved.qbl");
    QVERIFY(window.blockOperations().saveTo(file).ok());
    QVERIFY(QFileInfo::exists(file));QVERIFY(!view->isModified());const auto saved=scene(view);
    trigger(window,"undoAction");QVERIFY(view->isModified());trigger(window,"redoAction");QVERIFY(!view->isModified());QCOMPARE(scene(view),saved);
    QVERIFY(window.blockOperations().openFrom(file).ok());QCOMPARE(scene(view),saved);
    QVERIFY(!window.findChild<QAction*>("undoAction")->isEnabled());
    trigger(window,"newFileAction");QVERIFY(view->GetEntityList().isEmpty());QVERIFY(window.blockLibrary().definitions.isEmpty());QVERIFY(!view->isModified());
}
void EditorWorkflowTests::orderingAndStyle()
{
    MainWindow window;showWindow(window);auto* view=window.canvas();
    auto* a=new MRectangle({0,0},{50,40});auto* b=new MRectangle({10,10},{60,50});
    view->addEntity(a);view->addEntity(b);view->AddSelection(a);const auto before=scene(view);
    trigger(window,"bringFrontAction");QCOMPARE(view->GetEntityList().last(),static_cast<MEntity*>(a));
    trigger(window,"undoAction");QCOMPARE(scene(view),before);trigger(window,"redoAction");
    trigger(window,"sendBackAction");QCOMPARE(scene(view),before);
    QPen pen(Qt::red,3,Qt::DashLine);QBrush brush(Qt::yellow);window.editing().style(&pen,&brush);
    auto* styled=view->GetSelectedEntityList().first();QCOMPARE(styled->GetPen(),pen);QCOMPARE(styled->GetBrush(),brush);
    trigger(window,"undoAction");QCOMPARE(scene(view),before);trigger(window,"redoAction");
    trigger(window,"deleteAction");QCOMPARE(view->GetEntityList().size(),1);trigger(window,"undoAction");QCOMPARE(view->GetEntityList().size(),2);
    view->setModified(false);
}
void EditorWorkflowTests::legacyDocuments()
{
    QCADView view;BlockLibrary library;
    const auto example=QString::fromUtf8(QCAD_MORE_SOURCE_DIR)+"/examples/block-demo.qbl";QVERIFY(QFileInfo::exists(example));
    QVERIFY(DocumentStorage().load(example,view,library).ok());QVERIFY(!library.definitions.isEmpty());
    QTemporaryDir directory;const auto path=directory.filePath("upgraded.qbl");
    QVERIFY(DocumentStorage().save(path,view,library).ok());QFile file(path);QVERIFY(file.open(QIODevice::ReadOnly));
    QDataStream header(&file);quint32 magic,version;header>>magic>>version;QCOMPARE(version,quint32(3));file.close();
    QCADView loaded;BlockLibrary restored;QVERIFY(DocumentStorage().load(path,loaded,restored).ok());QCOMPARE(scene(&loaded),scene(&view));
    // A true old-style payload contains no affine prefix or trailing transform.
    MRectangle rectangle({0,0},{10,20});EntitySnapshot legacy;legacy.entityType=etRectangle;
    QDataStream out(&legacy.payload,QIODevice::WriteOnly);out.setVersion(QDataStream::Qt_5_12);rectangle.Serialize(out,true);
    std::unique_ptr<MEntity> decoded;QVERIFY(decodeEntity(legacy,decoded).ok());QVERIFY(decoded->GetTransform().isIdentity());
    // Reproduce the real v2 scene layout, including block-instance payloads without affine data.
    const auto v2Path=directory.filePath("legacy-v2.qbl");QFile v2File(v2Path);QVERIFY(v2File.open(QIODevice::WriteOnly));
    QDataStream v2(&v2File);v2.setVersion(QDataStream::Qt_5_12);
    v2<<quint32(0x51424c4b)<<quint32(2)<<quint32(library.definitions.size());
    for(const auto& definition:library.definitions) {
        v2<<definition.id<<definition.name<<definition.sourceBasePoint<<quint32(definition.entities.size());
        for(const auto& snapshot:definition.entities) v2<<qint32(snapshot.entityType)<<snapshot.payload;
    }
    v2<<quint32(view.GetEntityList().size());
    for(auto* entity:view.GetEntityList()) {
        QByteArray data;QDataStream stream(&data,QIODevice::WriteOnly);stream.setVersion(QDataStream::Qt_5_12);
        if(auto* block=dynamic_cast<BlockEntity*>(entity)) {
            block->MEntity::Serialize(stream,true);
            stream<<block->instance.instanceId<<block->instance.definitionId<<block->instance.position
                  <<block->instance.rotationDegrees<<block->instance.scale<<quint32(block->children.size());
            for(const auto& child:block->children) {
                QByteArray childData;QDataStream childStream(&childData,QIODevice::WriteOnly);childStream.setVersion(QDataStream::Qt_5_12);
                child->Serialize(childStream,true);stream<<qint32(child->GetType())<<childData;
            }
        } else entity->Serialize(stream,true);
        v2<<qint32(entity->GetType())<<data;
    }
    v2File.close();QVERIFY(DocumentStorage().load(v2Path,loaded,restored).ok());QCOMPARE(scene(&loaded),scene(&view));
}
void EditorWorkflowTests::drawingToolsAndCancellation()
{
    MainWindow window;showWindow(window);auto* view=window.canvas();
    auto click=[view](QPointF point){QTest::mouseClick(view,Qt::LeftButton,{},view->WorldtoScreen(point));};
    view->drawLine();click({-250,120});click({-190,140});
    view->drawRectangle();click({-150,60});click({-90,100}); // Both corner directions render correctly.
    view->drawCircle();click({-30,90});click({-10,90});
    view->drawArc();click({70,90});click({90,90});click({70,110});
    view->drawEllipse();click({130,60});click({200,100});
    view->drawPolygon();click({-250,-50});click({-190,-50});click({-220,0});
    QTest::mouseDClick(view,Qt::LeftButton,{},view->WorldtoScreen({-220,0}));
    view->drawText();click({-120,-50});
    QTimer answer;connect(&answer,&QTimer::timeout,[] {
        if(auto* dialog=qobject_cast<QInputDialog*>(QApplication::activeModalWidget())) {dialog->setTextValue(QStringLiteral("绘图测试"));dialog->accept();}
    });answer.start(20);click({-40,-10});answer.stop();
    QCOMPARE(view->GetEntityList().size(),7);
    const QVector<int> types={etLine,etRectangle,etCircle,etArc,etEllipse,etPolygon,etText};
    for(int i=0;i<types.size();++i) QCOMPARE(view->GetEntityList()[i]->GetType(),types[i]);
    const auto before=scene(view);window.editing().history.markSaved();
    view->drawCircle();click({40,-40});moveMouse(view,view->WorldtoScreen({60,-40}),Qt::NoButton);
    QVERIFY(view->hasPreview());QTest::keyClick(view,Qt::Key_Escape);QVERIFY(!view->hasPreview());
    QCOMPARE(scene(view),before);QVERIFY(!view->isModified());
    view->drawLine();click({40,-40});QTest::mouseClick(view,Qt::RightButton,{},view->WorldtoScreen({60,-40}));
    QCOMPARE(scene(view),before);
    view->selectEntity();view->ClearSelections();
    auto rendered=view->grab().toImage();
    // Check that the previously reversed ellipse corners actually produce a visible outline.
    const auto edge=view->WorldtoScreen({165,100});bool ink=false;
    for(int dx=-2;dx<=2;++dx) for(int dy=-2;dy<=2;++dy)
        if(rendered.pixelColor(edge+QPoint(dx,dy))!=QColor(Qt::white)) ink=true;
    QVERIFY(ink);
    QDir().mkpath("artifacts");QVERIFY(window.grab().save("artifacts/all-drawing-tools.png"));view->setModified(false);
}
QTEST_MAIN(EditorWorkflowTests)
#include "editor_workflow_tests.moc"

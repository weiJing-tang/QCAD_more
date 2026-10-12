#include <QtTest>
#include <QtWidgets>
#include "qcad/mainwindow.h"
#include "qcad/ENTITY.H"
#include "selection/SelectionService.h"
#include "blocks/BlockDefinitionService.h"
#include "blocks/BlockInsertionService.h"
#include "blocks/BlockEntity.h"
#include "text/TextPropertyService.h"
#include "text/TextPropertiesDialog.h"
#include "storage/DocumentStorage.h"
#include "storage/EntityCodec.h"
using namespace qcad_more;

class BlockWorkflowTests : public QObject {
    Q_OBJECT
private slots:
    void initTestCase();
    void documentRoundTrip();
    void failedLoadPreservesDocument();
    void selectionWithZoomAndShift();
    void blockSnapshotIsolation();
    void invalidFilesPreserveDocument_data();
    void invalidFilesPreserveDocument();
    void textPropertiesAndCancellation();
    void nestedTransformsAndTextRoundTrip();
    void dragBlocksAndPersist();
    void dragCancellationAndSelection();
    void windowWorkflow();
};

void BlockWorkflowTests::initTestCase()
{
#ifdef Q_OS_WIN
    // Windows offscreen QPA uses FreeType instead of the native font database.
    if (QGuiApplication::platformName() == QStringLiteral("offscreen")) {
        const auto path = qEnvironmentVariable("WINDIR") + QStringLiteral("/Fonts/msyh.ttc");
        QVERIFY(QFontDatabase::addApplicationFont(path) >= 0);
        QApplication::setFont(QFont(QStringLiteral("Microsoft YaHei"), 9));
    }
#endif
}

void BlockWorkflowTests::documentRoundTrip()
{
    QCADView view;
    BlockLibrary library;
    view.addEntity(new MLine({20, 20}, {100, 20}));
    view.addEntity(new MRectangle({20, 20}, {100, 80}));
    view.addEntity(new CCircle({50, 50}, 12));
    view.addEntity(new MEllipse({20, 20}, {80, 60}));
    view.addEntity(new CArc({50, 50}, {70, 50}, {50, 70}));
    view.addEntity(new MPolygon(QVector<QPointF>{{25, 25}, {40, 60}, {65, 25}}));
    auto* text = new MText;
    TextProperties properties;
    properties.content = QStringLiteral("图形块 · 中文 Text\n第二行");
    properties.font = QFont(QStringLiteral("Microsoft YaHei"), 16);
    properties.font.setBold(true); properties.font.setItalic(true); properties.font.setUnderline(true);
    properties.color = QColor("#235ab4"); properties.bounds = QRectF(20, 20, 100, 55);
    QVERIFY(TextPropertyService().apply(*text, properties).ok());
    view.addEntity(text);
    QPen pen(QColor("#b42d36"), 2.5, Qt::DashLine);
    QBrush brush(QColor("#dfedfa"));
    view.GetEntityList()[1]->SetAttrib(&pen, &brush);
    QVector<EntitySnapshot> originals;
    for (auto* entity : view.GetEntityList()) {
        EntitySnapshot snapshot; QVERIFY(encodeEntity(*entity, snapshot).ok()); originals.push_back(snapshot);
    }
    // The rectangle includes horizontal lines as well as area-bearing geometry.
    QVERIFY(SelectionService().selectInRectangle(view, QRectF(0, 0, 150, 100), SelectionMode::Replace).ok());
    QCOMPARE(view.GetSelectedEntityList().size(), 7);
    QUuid definitionId;
    QVERIFY(BlockDefinitionService().defineBlock(view.GetSelectedEntityList(),
        {QStringLiteral("组合块"), {20, 20}}, library, definitionId).ok());
    QCOMPARE(library.definitions.size(), 1);
    for (int i = 0; i < originals.size(); ++i) {
        EntitySnapshot after; QVERIFY(encodeEntity(*view.GetEntityList()[i], after).ok());
        QCOMPARE(after.payload, originals[i].payload); // Defining a block never edits originals.
    }
    QUuid unchanged = QUuid::createUuid(); const auto sentinel = unchanged;
    auto duplicate = BlockDefinitionService().defineBlock(view.GetSelectedEntityList(),
        {QStringLiteral("组合块"), {20, 20}}, library, unchanged);
    QCOMPARE(duplicate.code, ErrorCode::DuplicateBlockName); QCOMPARE(unchanged, sentinel);
    BlockInsertionService insertion;
    QVERIFY(insertion.previewAt(view, library, {definitionId, {180, 20}}).ok());
    QVERIFY(view.hasPreview()); QCOMPARE(view.GetEntityList().size(), 7);
    QVERIFY(insertion.cancelPreview(view).ok()); QVERIFY(!view.hasPreview());
    QUuid firstId, secondId;
    QVERIFY(insertion.insertAt(view, library, {definitionId, {180, 20}}, firstId).ok());
    QVERIFY(insertion.insertAt(view, library, {definitionId, {180, 150}}, secondId).ok());
    QVERIFY(firstId != secondId);
    auto* firstBlock = dynamic_cast<BlockEntity*>(view.GetEntityList()[7]); QVERIFY(firstBlock);
    auto* insertedRectangle = dynamic_cast<MRectangle*>(firstBlock->children[1].get()); QVERIFY(insertedRectangle);
    QCOMPARE(insertedRectangle->GetLeftTopPos(), QPointF(180,20));
    QCOMPARE(insertedRectangle->GetRightBottomPos(), QPointF(260,80));
    QCOMPARE(insertedRectangle->GetPen(), pen); QCOMPARE(insertedRectangle->GetBrush(), brush);
    firstBlock->Move({180,20}, {190,35}); // Whole-block movement updates persisted placement.
    QTemporaryDir directory; QVERIFY(directory.isValid());
    const auto path = directory.filePath(QStringLiteral("中文图形.qbl"));
    QVERIFY(DocumentStorage().save(path, view, library).ok());
    QCOMPARE(view.GetEntityList()[1]->GetPen(), pen); // Saving has no serialization side effects.
    QCADView loadedView; BlockLibrary loadedLibrary;
    QVERIFY(DocumentStorage().load(path, loadedView, loadedLibrary).ok());
    QCOMPARE(loadedView.GetEntityList().size(), 9);
    QCOMPARE(loadedLibrary.definitions.size(), 1); QCOMPARE(loadedLibrary.definitions[0].id, definitionId);
    for (int i = 0; i < originals.size(); ++i) {
        EntitySnapshot restoredSnapshot;
        QVERIFY(encodeEntity(*loadedView.GetEntityList()[i], restoredSnapshot).ok());
        QCOMPARE(restoredSnapshot.entityType, originals[i].entityType);
        QCOMPARE(restoredSnapshot.payload, originals[i].payload);
    }
    TextProperties restored;
    QVERIFY(TextPropertyService().read(*dynamic_cast<MText*>(loadedView.GetEntityList()[6]), restored).ok());
    QCOMPARE(restored.content, properties.content); QCOMPARE(restored.font, properties.font);
    QCOMPARE(restored.color, properties.color); QCOMPARE(restored.bounds, properties.bounds);
    auto* block = dynamic_cast<BlockEntity*>(loadedView.GetEntityList()[7]); QVERIFY(block);
    QCOMPARE(block->instance.instanceId, firstId); QCOMPARE(block->instance.position, QPointF(190,35));
    for (size_t i = 0; i < firstBlock->children.size(); ++i) {
        EntitySnapshot expected, actual;
        QVERIFY(encodeEntity(*firstBlock->children[i], expected).ok());
        QVERIFY(encodeEntity(*block->children[i], actual).ok());
        QCOMPARE(actual.payload, expected.payload);
    }
    auto* blockText = dynamic_cast<MText*>(block->children[6].get()); QVERIFY(blockText);
    QCOMPARE(blockText->GetText(), properties.content); QCOMPARE(blockText->GetTextColor(), properties.color);
    QUuid thirdId;
    QVERIFY(insertion.insertAt(loadedView, loadedLibrary, {definitionId, {400,0}}, thirdId).ok());
    QCOMPARE(loadedView.GetEntityList().size(), 10);
}

void BlockWorkflowTests::failedLoadPreservesDocument()
{
    QCADView view; BlockLibrary library;
    auto* line = new MLine({0,0},{10,10}); view.addEntity(line); view.AddSelection(line);
    QUuid id; QVERIFY(BlockDefinitionService().defineBlock({line}, {QStringLiteral("块"), {}}, library, id).ok());
    QTemporaryDir directory;
    const auto path = directory.filePath("drawing.qbl");
    QVERIFY(DocumentStorage().save(path, view, library).ok());
    QFile file(path); QVERIFY(file.open(QIODevice::ReadWrite)); QVERIFY(file.resize(file.size()-5)); file.close();
    QVERIFY(!DocumentStorage().load(path, view, library).ok());
    QCOMPARE(view.GetEntityList().front(), line); QCOMPARE(view.GetSelectedEntityList().front(), line);
    QCOMPARE(library.definitions.front().id, id);
    // An unsupported object must not overwrite an existing document.
    QVERIFY(file.open(QIODevice::ReadOnly)); const auto previous = file.readAll(); file.close();
    view.addEntity(new MEntity);
    QVERIFY(!DocumentStorage().save(path, view, library).ok());
    QVERIFY(file.open(QIODevice::ReadOnly)); QCOMPARE(file.readAll(), previous);
}

void BlockWorkflowTests::selectionWithZoomAndShift()
{
    MainWindow window; window.resize(1200, 800); window.show();
    QTest::qWait(50);
    auto* view = window.canvas();
    auto* horizontal = new MLine({-120, 20}, {-60, 20});
    auto* vertical = new MLine({20, -50}, {20, 10});
    auto* crossing = new MLine({-140, -40}, {0, -40});
    view->addEntity(horizontal); view->addEntity(vertical); view->addEntity(crossing);
    view->setScale(2); view->MoveOrg(37, -23);
    view->selectEntity();
    auto at = [view](QPointF p) { return view->WorldtoScreen(p); };
    // Reverse-direction drag includes a zero-height line but excludes a crossing line.
    QTest::mousePress(view, Qt::LeftButton, {}, at({-50, 30}));
    QTest::mouseMove(view, at({-130, -45}));
    QTest::mouseRelease(view, Qt::LeftButton, {}, at({-130, -45}));
    QCOMPARE(view->GetSelectedEntityList(), QList<MEntity*>{horizontal});
    QTest::mouseClick(view, Qt::LeftButton, Qt::ShiftModifier, at({20, -20}));
    QCOMPARE(view->GetSelectedEntityList().size(), 2);
    QTest::mouseClick(view, Qt::LeftButton, Qt::ShiftModifier, at({-90, 20}));
    QCOMPARE(view->GetSelectedEntityList(), QList<MEntity*>{vertical});
    // Pick at four screen pixels from the line, then verify outside the tolerance.
    QTest::mouseClick(view, Qt::LeftButton, {}, at({-90, 20}) + QPoint(0, 4));
    QCOMPARE(view->GetSelectedEntityList(), QList<MEntity*>{horizontal});
    QTest::mouseClick(view, Qt::LeftButton, {}, at({-90, 20}) + QPoint(0, 8));
    QVERIFY(view->GetSelectedEntityList().isEmpty());
    QTest::mousePress(view, Qt::LeftButton, {}, at({-130, 30}));
    QTest::mouseMove(view, at({30, -60}));
    QTest::keyClick(view, Qt::Key_Escape);
    QTest::mouseRelease(view, Qt::LeftButton, {}, at({30, -60}));
    QVERIFY(view->GetSelectedEntityList().isEmpty());
    view->setModified(false);
}

void BlockWorkflowTests::blockSnapshotIsolation()
{
    QCADView view; BlockLibrary library;
    auto* text = new MText;
    TextProperties original{QStringLiteral("定义时的文字"), QFont("Arial", 12), Qt::blue, QRectF(10, 20, 50, 30)};
    QVERIFY(TextPropertyService().apply(*text, original).ok()); view.addEntity(text);
    QUuid definition;
    QVERIFY(BlockDefinitionService().defineBlock({text, text}, {QStringLiteral("快照"), {10,20}}, library, definition).ok());
    QCOMPARE(library.definitions.front().entities.size(), 1);
    auto changed = original; changed.content = QStringLiteral("原件已修改");
    QVERIFY(TextPropertyService().apply(*text, changed).ok());
    QUuid instance;
    QVERIFY(BlockInsertionService().insertAt(view, library, {definition, {100,200}}, instance).ok());
    auto* block = dynamic_cast<BlockEntity*>(view.GetEntityList().last()); QVERIFY(block);
    auto* child = dynamic_cast<MText*>(block->children.front().get()); QVERIFY(child);
    QCOMPARE(child->GetText(), original.content);
    QCOMPARE(child->GetLeftTopPos(), QPointF(100,200));
    const auto before = library.definitions.size();
    QUuid nested;
    QVERIFY(BlockDefinitionService().defineBlock({block}, {QStringLiteral("嵌套"), {}}, library, nested).ok());
    QCOMPARE(library.definitions.size(), before + 1); QVERIFY(!nested.isNull());
    std::unique_ptr<MEntity> tree = std::make_unique<MLine>(QPointF(), QPointF(1,1));
    for (int level = 0; level < 32; ++level) {
        auto parent = std::make_unique<BlockEntity>();
        parent->instance = {QUuid::createUuid(), definition, {}};
        parent->children.push_back(std::move(tree)); tree = std::move(parent);
    }
    EntitySnapshot snapshot;
    QVERIFY(encodeEntity(*tree, snapshot).ok());
    std::unique_ptr<MEntity> restored;
    QVERIFY(decodeEntity(snapshot, restored).ok());
    auto tooDeep = std::make_unique<BlockEntity>();
    tooDeep->instance = {QUuid::createUuid(), definition, {}};
    tooDeep->children.push_back(std::move(tree));
    QVERIFY(!encodeEntity(*tooDeep, snapshot).ok());
}

void BlockWorkflowTests::invalidFilesPreserveDocument_data()
{
    QTest::addColumn<int>("corruption");
    QTest::newRow("wrong-magic") << 0;
    QTest::newRow("unsupported-version") << 1;
    QTest::newRow("excessive-record-count") << 2;
    QTest::newRow("trailing-data") << 3;
}

void BlockWorkflowTests::invalidFilesPreserveDocument()
{
    QFETCH(int, corruption);
    QCADView view; BlockLibrary library;
    auto* line = new MLine({1,2}, {3,4}); view.addEntity(line); view.AddSelection(line);
    QUuid id; QVERIFY(BlockDefinitionService().defineBlock({line}, {"existing", {}}, library, id).ok());
    QTemporaryDir directory; QVERIFY(directory.isValid());
    const auto path = directory.filePath("corrupt.qbl");
    QVERIFY(DocumentStorage().save(path, view, library).ok());
    QFile file(path); QVERIFY(file.open(QIODevice::ReadWrite));
    if (corruption == 3) { QVERIFY(file.seek(file.size())); QCOMPARE(file.write("extra"), qint64(5)); }
    else {
        QVERIFY(file.seek(corruption * 4));
        QDataStream stream(&file); stream.setVersion(QDataStream::Qt_5_12);
        stream << quint32(corruption == 0 ? 0 : corruption == 1 ? 999 : 100001);
    }
    file.close();
    QVERIFY(!DocumentStorage().load(path, view, library).ok());
    QCOMPARE(view.GetEntityList(), QList<MEntity*>{line});
    QCOMPARE(view.GetSelectedEntityList(), QList<MEntity*>{line});
    QCOMPARE(library.definitions.front().id, id); QVERIFY(view.isModified());
}

void BlockWorkflowTests::textPropertiesAndCancellation()
{
    MainWindow window; window.resize(1000, 700); window.show(); QTest::qWait(50);
    auto* view = window.canvas(); auto* text = new MText;
    TextProperties properties{QStringLiteral("原始中文\n第二行"), QFont("Arial", 12), QColor("#123456"), QRectF(-100, 0, 80, 40)};
    properties.font.setPointSizeF(17.5); properties.font.setBold(true);
    properties.font.setItalic(true); properties.font.setUnderline(true);
    QVERIFY(TextPropertyService().apply(*text, properties).ok()); view->addEntity(text); view->AddSelection(text);
    view->setModified(false);
    EntitySnapshot before; QVERIFY(encodeEntity(*text, before).ok());
    QTimer::singleShot(30, [] {
        if (auto* dialog = dynamic_cast<TextPropertiesDialog*>(QApplication::activeModalWidget())) {
            QDir().mkpath("artifacts");
            dialog->grab().save("artifacts/text-properties.png");
            dialog->findChild<QPlainTextEdit*>("textContent")->setPlainText(QStringLiteral("取消的修改"));
            dialog->findChild<QDoubleSpinBox*>("fontSize")->setValue(39.5);
            dialog->reject();
        }
    });
    window.findChild<QAction*>("textPropertiesAction")->trigger();
    EntitySnapshot after; QVERIFY(encodeEntity(*text, after).ok());
    QCOMPARE(after.payload, before.payload); QVERIFY(!view->isModified());
    auto invalid = properties; invalid.bounds.setWidth(0);
    QVERIFY(!TextPropertyService().apply(*text, invalid).ok());
    QVERIFY(encodeEntity(*text, after).ok()); QCOMPARE(after.payload, before.payload);
    QTimer::singleShot(30, [] {
        if (auto* dialog = dynamic_cast<TextPropertiesDialog*>(QApplication::activeModalWidget())) {
            dialog->findChild<QDoubleSpinBox*>("fontSize")->setValue(23.75);
            dialog->findChild<QDoubleSpinBox*>("textX")->setValue(-150);
            dialog->findChild<QDoubleSpinBox*>("textWidth")->setValue(125);
            dialog->accept();
        }
    });
    window.findChild<QAction*>("textPropertiesAction")->trigger();
    QCOMPARE(text->GetFont().pointSizeF(), 23.75); QCOMPARE(text->GetLeftTopPos().x(), -150.0);
    QCOMPARE(text->GetRightBottomPos().x(), -25.0); QCOMPARE(text->GetTextColor(), properties.color);
    QVERIFY(text->GetFont().bold()); QVERIFY(text->GetFont().italic()); QVERIFY(text->GetFont().underline());
    QVERIFY(view->isModified()); view->setModified(false);
}

void BlockWorkflowTests::nestedTransformsAndTextRoundTrip()
{
    MainWindow window; window.resize(1000,700); window.show(); QTest::qWait(50);
    auto* view = window.canvas(); BlockLibrary library;
    auto* line = new MLine({0,0}, {10,0}); view->addEntity(line);
    auto* text = new MText;
    QVERIFY(TextPropertyService().apply(*text, {QStringLiteral("模板"), QFont("Arial",12), Qt::blue, QRectF(0,0,10,5)}).ok());
    view->addEntity(text);
    QUuid baseId;
    QVERIFY(BlockDefinitionService().defineBlock({line,text}, {"base", {}}, library, baseId).ok());
    QUuid firstId;
    QVERIFY(BlockInsertionService().insertAt(*view, library, {baseId,{100,200},90,2}, firstId).ok());
    auto* first = dynamic_cast<BlockEntity*>(view->GetEntityList().last()); QVERIFY(first);
    const auto mapped = first->worldTransform().map(QPointF(110,200));
    QVERIFY(QLineF(mapped, QPointF(100,220)).length() < 1e-8);
    QRectF box; first->GetBox(box);
    QVERIFY(QLineF(box.topLeft(), QPointF(90,200)).length() < 1e-8);
    QVERIFY(QLineF(box.bottomRight(), QPointF(100,220)).length() < 1e-8);
    view->zoomRect(QRectF(80,180,50,50)); QTest::qWait(50);
    const auto rendered = view->grab().toImage();
    QDir().mkpath("artifacts"); rendered.save("artifacts/rotated-block.png");
    // Probe below the text glyphs so different native/offscreen font metrics
    // cannot hide the line with blue text at the rectangle's center.
    const auto pixel = view->WorldtoScreen({100,205});
    bool ink = false;
    for (int dx = -2; dx <= 2; ++dx)
        for (int dy = -2; dy <= 2; ++dy)
            if (rendered.pixelColor(pixel + QPoint(dx,dy)) == QColor(Qt::black)) ink = true;
    QVERIFY2(ink, "Rotated line must actually render at its transformed position.");
    QUuid outerId;
    QVERIFY(BlockDefinitionService().defineBlock({first}, {"outer", {100,200}}, library, outerId).ok());
    QUuid outerInstance;
    QVERIFY(BlockInsertionService().insertAt(*view, library, {outerId,{400,500},-90,.5}, outerInstance).ok());
    auto* outer = dynamic_cast<BlockEntity*>(view->GetEntityList().last()); QVERIFY(outer);
    outer->GetBox(box);
    QVERIFY(QLineF(box.topLeft(), QPointF(400,500)).length() < 1e-8);
    QVERIFY(QLineF(box.bottomRight(), QPointF(410,505)).length() < 1e-8);
    view->ClearSelections(); view->AddSelection(outer);
    QTimer dialogs;
    connect(&dialogs, &QTimer::timeout, [] {
        if (auto* input = qobject_cast<QInputDialog*>(QApplication::activeModalWidget())) {
            input->grab().save("artifacts/nested-text-selector.png"); input->accept();
        }
        else if (auto* properties = dynamic_cast<TextPropertiesDialog*>(QApplication::activeModalWidget())) {
            properties->findChild<QPlainTextEdit*>("textContent")->setPlainText(QStringLiteral("仅这个嵌套实例的文字"));
            properties->findChild<QDoubleSpinBox*>("fontSize")->setValue(19.25);
            properties->accept();
        }
    });
    dialogs.start(20);
    window.findChild<QAction*>("textPropertiesAction")->trigger(); dialogs.stop();
    QVector<MText*> texts; QStringList labels; outer->collectTexts(texts,labels);
    QCOMPARE(texts.size(), 1); QCOMPARE(texts.front()->GetText(), QStringLiteral("仅这个嵌套实例的文字"));
    QVector<MText*> unchanged; QStringList originalLabels; first->collectTexts(unchanged, originalLabels);
    QCOMPARE(unchanged.front()->GetText(), QStringLiteral("模板"));
    QTemporaryDir directory; const auto path = directory.filePath("nested.qbl");
    QVERIFY(DocumentStorage().save(path, *view, library).ok());
    QCADView loaded; BlockLibrary loadedLibrary;
    QVERIFY(DocumentStorage().load(path, loaded, loadedLibrary).ok());
    auto* restored = dynamic_cast<BlockEntity*>(loaded.GetEntityList().last()); QVERIFY(restored);
    EntitySnapshot expected, actual;
    QVERIFY(encodeEntity(*outer,expected).ok()); QVERIFY(encodeEntity(*restored,actual).ok());
    QCOMPARE(actual.payload,expected.payload);
    texts.clear(); labels.clear(); restored->collectTexts(texts, labels);
    QCOMPARE(texts.front()->GetText(), QStringLiteral("仅这个嵌套实例的文字"));
    QCOMPARE(texts.front()->GetFont().pointSizeF(), 19.25);
    view->ClearSelections(); view->AddSelection(outer);
    QTimer::singleShot(20, [] {
        if (auto* dialog = qobject_cast<QDialog*>(QApplication::activeModalWidget())) {
            dialog->findChild<QDoubleSpinBox*>("blockRotation")->setValue(45);
            dialog->findChild<QDoubleSpinBox*>("blockScale")->setValue(3);
            dialog->grab().save("artifacts/transform-settings.png");
            dialog->accept();
        }
    });
    window.findChild<QAction*>("blockTransformAction")->trigger();
    QCOMPARE(outer->instance.rotationDegrees, 45.0); QCOMPARE(outer->instance.scale, 3.0);
    QVERIFY(DocumentStorage().save(path, *view, library).ok());
    QVERIFY(DocumentStorage().load(path, loaded, loadedLibrary).ok());
    restored = dynamic_cast<BlockEntity*>(loaded.GetEntityList().last()); QVERIFY(restored);
    QCOMPARE(restored->instance.rotationDegrees, 45.0); QCOMPARE(restored->instance.scale, 3.0);
    outer->GetBox(box); view->zoomRect(box.adjusted(-10,-10,10,10));
    view->ClearSelections(); QTest::qWait(50);
    QVERIFY(window.grab().save("artifacts/nested-block-workflow.png"));
    const auto count = view->GetEntityList().size(); QUuid bad;
    QVERIFY(!BlockInsertionService().insertAt(*view, library, {baseId, {}, 0, 0}, bad).ok());
    QCOMPARE(view->GetEntityList().size(), count); QVERIFY(bad.isNull());
    QRectF beforeMove; restored->GetBox(beforeMove);
    restored->Move({400,500}, {450,550}); restored->GetBox(box);
    QVERIFY(QLineF(box.topLeft(), beforeMove.topLeft() + QPointF(50,50)).length() < 1e-8);
    // Older v1 files store references and placement without per-instance snapshots.
    const auto legacy = directory.filePath("legacy.qbl"); QFile file(legacy);
    QVERIFY(file.open(QIODevice::WriteOnly)); QDataStream stream(&file); stream.setVersion(QDataStream::Qt_5_12);
    stream << quint32(0x51424c4b) << quint32(1) << quint32(1);
    const auto& definition = library.definitions.front();
    stream << definition.id << definition.name << definition.sourceBasePoint << quint32(definition.entities.size());
    for (const auto& snapshot : definition.entities) stream << qint32(snapshot.entityType) << snapshot.payload;
    stream << quint32(1) << qint32(etBlock) << firstId << baseId << QPointF(50,60); file.close();
    QVERIFY(DocumentStorage().load(legacy, loaded, loadedLibrary).ok());
    restored = dynamic_cast<BlockEntity*>(loaded.GetEntityList().front()); QVERIFY(restored);
    QCOMPARE(restored->instance.position, QPointF(50,60)); QCOMPARE(restored->instance.scale,1.0);
    view->setModified(false);
}

void BlockWorkflowTests::dragBlocksAndPersist()
{
    MainWindow window; window.resize(1200,800); window.show(); QTest::qWait(50);
    auto* view = window.canvas(); auto& library = window.blockLibrary();
    auto* rect = new MRectangle({0,0},{60,40}); view->addEntity(rect);
    auto* text = new MText;
    QVERIFY(TextPropertyService().apply(*text,
        {QStringLiteral("拖动的文字"),QFont("Arial",10),Qt::blue,QRectF(5,10,45,20)}).ok());
    view->addEntity(text);
    QUuid base;
    QVERIFY(BlockDefinitionService().defineBlock({rect,text},{"drag-base",{}},library,base).ok());
    QUuid id;
    QVERIFY(BlockInsertionService().insertAt(*view,library,{base,{-200,60},30,1.5},id).ok());
    auto* child = dynamic_cast<BlockEntity*>(view->GetEntityList().last()); QVERIFY(child);
    QUuid nested;
    QVERIFY(BlockDefinitionService().defineBlock({child},{"drag-nested",{-200,60}},library,nested).ok());
    QVERIFY(BlockInsertionService().insertAt(*view,library,{nested,{-180,-70},-15,.75},id).ok());
    auto* first = dynamic_cast<BlockEntity*>(view->GetEntityList().last()); QVERIFY(first);
    QVERIFY(BlockInsertionService().insertAt(*view,library,{base,{150,-70}},id).ok());
    auto* second = dynamic_cast<BlockEntity*>(view->GetEntityList().last()); QVERIFY(second);
    view->setScale(1.5); view->MoveOrg(23,-17); view->selectEntity(); view->ClearSelections();
    view->setModified(false);
    const auto firstPosition = first->instance.position;
    auto at = [view](QPointF point){return view->WorldtoScreen(point);};
    QRectF bounds; first->GetBox(bounds);
    const QPoint press = at(bounds.center()), release = press + QPoint(90,45);
    const auto delta = view->ScreentoWorld(release) - view->ScreentoWorld(press);
    EntitySnapshot before;
    QVERIFY(encodeEntity(*first,before).ok());
    QTest::mousePress(view,Qt::LeftButton,{},press);
    QCOMPARE(view->GetSelectedEntityList(),QList<MEntity*>{first});
    QMouseEvent motion(QEvent::MouseMove,press+QPoint(60,30),Qt::NoButton,Qt::LeftButton,Qt::NoModifier);
    QApplication::sendEvent(view,&motion);
    QVERIFY(view->hasPreview()); QVERIFY(!view->isModified());
    EntitySnapshot during;
    QVERIFY(encodeEntity(*first,during).ok()); QCOMPARE(during.payload,before.payload);
    QCOMPARE(view->cursor().shape(),Qt::ClosedHandCursor);
    QTest::mouseRelease(view,Qt::LeftButton,{},release);
    QVERIFY(!view->hasPreview()); QVERIFY(view->isModified());
    QCOMPARE(first->instance.position,firstPosition+delta);
    QCOMPARE(first->instance.rotationDegrees,-15.0); QCOMPARE(first->instance.scale,.75);
    QVERIFY(view->cursor().shape()!=Qt::ClosedHandCursor);
    // Drag a selected group: both instances move by the same world-space delta.
    view->AddSelection(second);
    first->GetBox(bounds);
    const QPoint groupPress=at(bounds.center()), groupRelease=groupPress+QPoint(-30,-60);
    const auto groupDelta=view->ScreentoWorld(groupRelease)-view->ScreentoWorld(groupPress);
    const auto firstBefore=first->instance.position, secondBefore=second->instance.position;
    QTest::mousePress(view,Qt::LeftButton,{},groupPress);
    QMouseEvent groupMotion(QEvent::MouseMove,groupRelease,Qt::NoButton,Qt::LeftButton,Qt::NoModifier);
    QApplication::sendEvent(view,&groupMotion);
    QVERIFY(view->hasPreview());
    QTest::mouseRelease(view,Qt::LeftButton,{},groupRelease);
    QCOMPARE(first->instance.position,firstBefore+groupDelta);
    QCOMPARE(second->instance.position,secondBefore+groupDelta);
    QCOMPARE(view->GetSelectedEntityList().size(),2);
    QVector<MText*> texts; QStringList labels; first->collectTexts(texts,labels);
    QCOMPARE(texts.front()->GetText(),QStringLiteral("拖动的文字"));
    QTemporaryDir directory; QVERIFY(directory.isValid());
    const auto file=directory.filePath("drag.qbl");
    QVERIFY(DocumentStorage().save(file,*view,library).ok());
    QCADView loaded; BlockLibrary restoredLibrary;
    QVERIFY(DocumentStorage().load(file,loaded,restoredLibrary).ok());
    QCOMPARE(loaded.GetEntityList().size(),view->GetEntityList().size());
    for (int i=0;i<view->GetEntityList().size();++i) {
        EntitySnapshot expected,actual;
        QVERIFY(encodeEntity(*view->GetEntityList()[i],expected).ok());
        QVERIFY(encodeEntity(*loaded.GetEntityList()[i],actual).ok());
        QCOMPARE(actual.payload,expected.payload);
    }
    QDir().mkpath("artifacts");
    QVERIFY(window.grab().save("artifacts/dragged-blocks.png"));
    view->setModified(false);
}

void BlockWorkflowTests::dragCancellationAndSelection()
{
    MainWindow window; window.resize(1100,750); window.show(); QTest::qWait(50);
    auto* view=window.canvas(); auto& library=window.blockLibrary();
    auto* rect=new MRectangle({0,0},{50,40}); view->addEntity(rect);
    QUuid definition,instance;
    QVERIFY(BlockDefinitionService().defineBlock({rect},{"cancel-drag",{}},library,definition).ok());
    QVERIFY(BlockInsertionService().insertAt(*view,library,{definition,{-150,40}},instance).ok());
    auto* block=dynamic_cast<BlockEntity*>(view->GetEntityList().last()); QVERIFY(block);
    view->selectEntity(); view->AddSelection(block); view->setModified(false);
    EntitySnapshot original; QVERIFY(encodeEntity(*block,original).ok());
    const QPoint press=view->WorldtoScreen({-125,60}), destination=press+QPoint(60,40);
    auto beginDrag=[&] {
        QTest::mousePress(view,Qt::LeftButton,{},press);
        QMouseEvent motion(QEvent::MouseMove,destination,Qt::NoButton,Qt::LeftButton,Qt::NoModifier);
        QApplication::sendEvent(view,&motion);
    };
    for(int cancelMode=0;cancelMode<3;++cancelMode) {
        view->selectEntity(); beginDrag(); QVERIFY(view->hasPreview());
        if(cancelMode==0) QTest::keyClick(view,Qt::Key_Escape);
        else if(cancelMode==1) QTest::mouseClick(view,Qt::RightButton,{},destination);
        else view->drawLine();
        QVERIFY(!view->hasPreview()); QVERIFY(!view->isModified());
        QTest::mouseRelease(view,Qt::LeftButton,{},destination);
        EntitySnapshot unchanged; QVERIFY(encodeEntity(*block,unchanged).ok());
        QCOMPARE(unchanged.payload,original.payload);
        QVERIFY(view->cursor().shape()!=Qt::ClosedHandCursor);
    }
    view->selectEntity();
    // Click jitter is below the platform drag threshold and must not move the block.
    QTest::mousePress(view,Qt::LeftButton,{},press);
    QTest::mouseRelease(view,Qt::LeftButton,{},press+QPoint(1,1));
    QVERIFY(!view->hasPreview()); QVERIFY(!view->isModified());
    QCOMPARE(view->GetSelectedEntityList(),QList<MEntity*>{block});
    QTest::mouseClick(view,Qt::LeftButton,Qt::ShiftModifier,press);
    QVERIFY(view->GetSelectedEntityList().isEmpty());
    QTest::mouseClick(view,Qt::LeftButton,Qt::ShiftModifier,press);
    QCOMPARE(view->GetSelectedEntityList(),QList<MEntity*>{block});
    // Starting in empty space still performs rectangle selection of the block.
    const QPoint from=view->WorldtoScreen({-170,100}), to=view->WorldtoScreen({-80,20});
    view->ClearSelections();
    QTest::mousePress(view,Qt::LeftButton,{},from);
    QMouseEvent boxMove(QEvent::MouseMove,to,Qt::NoButton,Qt::LeftButton,Qt::NoModifier);
    QApplication::sendEvent(view,&boxMove);
    QVERIFY(!view->hasPreview());
    QTest::mouseRelease(view,Qt::LeftButton,{},to);
    QCOMPARE(view->GetSelectedEntityList(),QList<MEntity*>{block});
    view->setModified(true); beginDrag(); QTest::keyClick(view,Qt::Key_Escape);
    QVERIFY(view->isModified()); QTest::mouseRelease(view,Qt::LeftButton,{},destination);
    view->setModified(false);
}

void BlockWorkflowTests::windowWorkflow()
{
    MainWindow window; window.resize(1200,800); window.show();
    auto* view = window.canvas();
    QTest::qWait(100);
    auto at = [view](QPointF point) { return view->WorldtoScreen(point); };
    // Create real geometry using mouse commands, then switch commands safely.
    view->drawRectangle();
    QTest::mouseClick(view, Qt::LeftButton, {}, at({-400,180}));
    QTest::mouseClick(view, Qt::LeftButton, {}, at({-200,80}));
    view->drawCircle();
    QTest::mouseClick(view, Qt::LeftButton, {}, at({-360,130}));
    QTest::mouseClick(view, Qt::LeftButton, {}, at({-335,130}));
    view->drawText();
    QTest::mouseClick(view, Qt::LeftButton, {}, at({-320,150}));
    QTimer::singleShot(30, [] {
        if (auto* dialog = qobject_cast<QInputDialog*>(QApplication::activeModalWidget())) {
            dialog->setTextValue(QStringLiteral("设备 A")); dialog->accept();
        }
    });
    QTest::mouseClick(view, Qt::LeftButton, {}, at({-215,100}));
    QCOMPARE(view->GetEntityList().size(), 3);
    view->selectEntity();
    QTimer::singleShot(30, [] {
        if (auto* dialog = dynamic_cast<TextPropertiesDialog*>(QApplication::activeModalWidget())) {
            auto* fonts = dialog->findChild<QFontComboBox*>();
            QVERIFY(fonts != nullptr);
            QVERIFY(fonts->count() > 0);
            dialog->findChild<QPlainTextEdit*>("textContent")->setPlainText(QStringLiteral("设备 A / 文本已编辑"));
            dialog->findChild<QDoubleSpinBox*>("fontSize")->setValue(11);
            dialog->accept();
        }
    });
    QTest::mouseDClick(view, Qt::LeftButton, {}, at({-270,125}));
    QCOMPARE(dynamic_cast<MText*>(view->GetEntityList()[2])->GetText(), QStringLiteral("设备 A / 文本已编辑"));
    // Real drag selection, then define with a canvas base point.
    QTest::mousePress(view, Qt::LeftButton, {}, at({-415,195}));
    QTest::mouseMove(view, at({-185,65}));
    QTest::mouseRelease(view, Qt::LeftButton, {}, at({-185,65}));
    QCOMPARE(view->GetSelectedEntityList().size(), 3);
    QDir().mkpath("artifacts");
    QVERIFY(window.grab().save("artifacts/selection.png"));
    QTimer::singleShot(30, [] {
        if (auto* dialog = qobject_cast<QInputDialog*>(QApplication::activeModalWidget())) {
            dialog->setTextValue(QStringLiteral("设备组合")); dialog->accept();
        }
    });
    window.findChild<QAction*>("defineBlockAction")->trigger();
    QTest::mouseClick(view, Qt::LeftButton, {}, at({-400,80}));
    QCoreApplication::processEvents();
    QCOMPARE(window.blockLibrary().definitions.size(), 1);
    QVERIFY(window.grab().save("artifacts/block-defined.png"));
    QTimer::singleShot(30, [] {
        if (auto* dialog = qobject_cast<QInputDialog*>(QApplication::activeModalWidget())) dialog->accept();
    });
    QTimer transformDialog;
    connect(&transformDialog, &QTimer::timeout, [] {
        auto* dialog = qobject_cast<QDialog*>(QApplication::activeModalWidget());
        if (dialog && dialog->objectName() == "blockTransformDialog") dialog->accept();
    });
    transformDialog.start(20);
    window.findChild<QAction*>("insertBlockAction")->trigger();
    transformDialog.stop();
    // Deliver hover in Qt coordinates: native QTest cursor warps can be coalesced
    // after modal dialogs, whereas drag events above also have a pressed button.
    QMouseEvent hover(QEvent::MouseMove, at({-100,80}), Qt::NoButton, Qt::NoButton, Qt::NoModifier);
    QApplication::sendEvent(view, &hover);
    QTRY_VERIFY(view->hasPreview());
    QVERIFY(window.grab().save("artifacts/insert-preview.png"));
    QTest::mouseClick(view, Qt::LeftButton, {}, at({-100,80}));
    QTest::mouseClick(view, Qt::LeftButton, {}, at({150,-90}));
    QTest::keyClick(view, Qt::Key_Escape);
    QCOMPARE(view->GetEntityList().size(), 5); QVERIFY(!view->hasPreview());
    // Capture the real widget render and a reusable example document.
    QDir().mkpath("artifacts");
    QVERIFY(DocumentStorage().save("artifacts/block-demo.qbl", *view, window.blockLibrary()).ok());
    view->ClearSelections(); view->update(); QTest::qWait(100);
    QVERIFY(window.grab().save("artifacts/block-workflow.png"));
    view->setModified(false);
}
QTEST_MAIN(BlockWorkflowTests)
#include "block_workflow_tests.moc"

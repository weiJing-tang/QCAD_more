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
    TextProperties restored;
    QVERIFY(TextPropertyService().read(*dynamic_cast<MText*>(loadedView.GetEntityList()[6]), restored).ok());
    QCOMPARE(restored.content, properties.content); QCOMPARE(restored.font, properties.font);
    QCOMPARE(restored.color, properties.color); QCOMPARE(restored.bounds, properties.bounds);
    auto* block = dynamic_cast<BlockEntity*>(loadedView.GetEntityList()[7]); QVERIFY(block);
    QCOMPARE(block->instance.instanceId, firstId); QCOMPARE(block->instance.position, QPointF(190,35));
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
    QTest::mouseClick(view, Qt::LeftButton, {}, at({-270,125}));
    QCOMPARE(view->GetSelectedEntityList().size(), 1);
    auto* edit = window.findChild<QAction*>("textPropertiesAction"); QVERIFY(edit);
    QTimer::singleShot(30, [] {
        if (auto* dialog = dynamic_cast<TextPropertiesDialog*>(QApplication::activeModalWidget())) {
            dialog->findChild<QPlainTextEdit*>("textContent")->setPlainText(QStringLiteral("设备 A / 文本已编辑"));
            dialog->findChild<QDoubleSpinBox*>("fontSize")->setValue(11);
            dialog->accept();
        }
    });
    edit->trigger();
    QCOMPARE(dynamic_cast<MText*>(view->GetEntityList()[2])->GetText(), QStringLiteral("设备 A / 文本已编辑"));
    // Real drag selection, then define with a canvas base point.
    QTest::mousePress(view, Qt::LeftButton, {}, at({-415,195}));
    QTest::mouseMove(view, at({-185,65}));
    QTest::mouseRelease(view, Qt::LeftButton, {}, at({-185,65}));
    QCOMPARE(view->GetSelectedEntityList().size(), 3);
    QTimer::singleShot(30, [] {
        if (auto* dialog = qobject_cast<QInputDialog*>(QApplication::activeModalWidget())) {
            dialog->setTextValue(QStringLiteral("设备组合")); dialog->accept();
        }
    });
    window.findChild<QAction*>("defineBlockAction")->trigger();
    QTest::mouseClick(view, Qt::LeftButton, {}, at({-400,80}));
    QCoreApplication::processEvents();
    QCOMPARE(window.blockLibrary().definitions.size(), 1);
    QTimer::singleShot(30, [] {
        if (auto* dialog = qobject_cast<QInputDialog*>(QApplication::activeModalWidget())) dialog->accept();
    });
    window.findChild<QAction*>("insertBlockAction")->trigger();
    QTest::mouseMove(view, at({-100,80}));
    QVERIFY(view->hasPreview());
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

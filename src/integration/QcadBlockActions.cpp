#include "integration/QcadBlockActions.h"
#include "integration/PointCommand.h"
#include "blocks/BlockDefinitionService.h"
#include "blocks/BlockInsertionService.h"
#include "selection/SelectionService.h"
#include "text/TextPropertyService.h"
#include "text/TextPropertiesDialog.h"
#include "storage/DocumentStorage.h"
#include "qcad/ENTITY.H"
#include "qcad/mainwindow.h"
#include <QtWidgets>
namespace qcad_more {
namespace {
OperationResult cancelled() { return {ErrorCode::Cancelled, {}}; }
}
QcadBlockActions::QcadBlockActions(QCADView& view, BlockLibrary& library, QWidget& parent)
    : view_(view), library_(library), dialogParent_(parent) {}
OperationResult QcadBlockActions::defineBlock()
{
    view_.selectEntity();
    QList<MEntity*> selected;
    auto result = SelectionService().captureSelected(view_, selected);
    if (!result.ok()) return result;
    bool ok = false;
    const auto name = QInputDialog::getText(&dialogParent_, QStringLiteral("定义块"),
        QStringLiteral("块名称（下一步在画布选择基点）："), QLineEdit::Normal, {}, &ok).trimmed();
    if (!ok) return cancelled();
    if (name.isEmpty()) return {ErrorCode::InvalidInput, QStringLiteral("块名称不能为空。")};
    for (const auto& block : library_.definitions)
        if (block.name == name) return {ErrorCode::DuplicateBlockName, QStringLiteral("该块名称已存在。")};
    view_.setCommand(new PointCommand(&view_, [this, selected, name](const QPointF& base) {
        QUuid id;
        return BlockDefinitionService().defineBlock(selected, {name, base}, library_, id);
    }));
    Prompt(QStringLiteral("请在画布单击块基点；Esc / 右键取消。"));
    return {};
}
OperationResult QcadBlockActions::editSelectedText()
{
    view_.selectEntity();
    const auto selected = view_.GetSelectedEntityList();
    if (selected.size() != 1 || selected.front()->GetType() != etText)
        return {ErrorCode::InvalidInput, QStringLiteral("请只选择一个 Text 文本。")};
    auto* text = dynamic_cast<MText*>(selected.front());
    if (!text) return {ErrorCode::InvalidInput, QStringLiteral("所选对象不是文本。")};
    TextProperties properties;
    TextPropertyService service;
    auto result = service.read(*text, properties);
    if (!result.ok()) return result;
    TextPropertiesDialog dialog(properties, &dialogParent_);
    if (dialog.exec() != QDialog::Accepted) return cancelled();
    result = service.apply(*text, dialog.properties());
    if (result.ok()) { view_.setModified(true); view_.update(); }
    return result;
}
OperationResult QcadBlockActions::beginInsertBlock()
{
    view_.selectEntity();
    if (library_.definitions.isEmpty()) return {ErrorCode::BlockNotFound, QStringLiteral("请先定义一个块。")};
    QStringList names;
    for (const auto& block : library_.definitions) names.push_back(block.name);
    bool ok = false;
    const auto name = QInputDialog::getItem(&dialogParent_, QStringLiteral("插入块"), QStringLiteral("选择块："), names, 0, false, &ok);
    if (!ok) return cancelled();
    const auto id = library_.definitions[names.indexOf(name)].id;
    view_.setCommand(new PointCommand(&view_, [this, id](const QPointF& point) {
        QUuid instanceId;
        return BlockInsertionService().insertAt(view_, library_, {id, point}, instanceId);
    }, [this, id](const QPointF& point) {
        return BlockInsertionService().previewAt(view_, library_, {id, point});
    }, true));
    Prompt(QStringLiteral("移动鼠标预览，单击插入；Esc / 右键结束。"));
    return {};
}
void QcadBlockActions::updateTitle()
{
    dialogParent_.setWindowTitle(QStringLiteral("QCAD - %1[*]").arg(filePath_.isEmpty() ? QStringLiteral("未命名") : QFileInfo(filePath_).fileName()));
}
OperationResult QcadBlockActions::saveTo(const QString& path)
{
    auto result = DocumentStorage().save(path, view_, library_);
    if (result.ok()) { filePath_ = path; updateTitle(); view_.setModified(false); Prompt(QStringLiteral("文档已保存。")); }
    return result;
}
OperationResult QcadBlockActions::save()
{ return filePath_.isEmpty() ? saveAs() : saveTo(filePath_); }
OperationResult QcadBlockActions::saveAs()
{
    auto path = QFileDialog::getSaveFileName(&dialogParent_, QStringLiteral("保存 QCAD 块文档"), filePath_, QStringLiteral("QCAD 块文档 (*.qbl)"));
    if (path.isEmpty()) return cancelled();
    if (QFileInfo(path).suffix().isEmpty()) path += QStringLiteral(".qbl");
    return saveTo(path);
}
bool QcadBlockActions::confirmDiscard()
{
    if (!view_.isModified()) return true;
    const auto choice = QMessageBox::question(&dialogParent_, QStringLiteral("保存修改"), QStringLiteral("当前文档尚未保存，是否先保存？"),
        QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel, QMessageBox::Save);
    if (choice == QMessageBox::Discard) return true;
    if (choice != QMessageBox::Save) return false;
    auto result = save();
    if (!result.ok() && result.code != ErrorCode::Cancelled) QMessageBox::warning(&dialogParent_, QStringLiteral("保存失败"), result.message);
    return result.ok();
}
OperationResult QcadBlockActions::open()
{
    if (!confirmDiscard()) return cancelled();
    const auto path = QFileDialog::getOpenFileName(&dialogParent_, QStringLiteral("打开 QCAD 块文档"), {}, QStringLiteral("QCAD 块文档 (*.qbl)"));
    if (path.isEmpty()) return cancelled();
    auto result = DocumentStorage().load(path, view_, library_);
    if (result.ok()) { filePath_ = path; updateTitle(); Prompt(QStringLiteral("文档已打开，可继续插入已有块。")); }
    return result;
}
OperationResult QcadBlockActions::newDocument()
{
    if (!confirmDiscard()) return cancelled();
    view_.replaceEntities({}); library_.definitions.clear(); filePath_.clear();
    updateTitle(); view_.setModified(false);
    return {};
}
}

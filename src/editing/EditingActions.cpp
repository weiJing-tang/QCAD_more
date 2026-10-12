#include "editing/EditingActions.h"
#include "blocks/BlockEntity.h"
#include "blocks/BlockDefinitionService.h"
#include "storage/EntityCodec.h"
#include "qcad/mainwindow.h"
#include <QtWidgets>
#include <algorithm>
#include <functional>
namespace qcad_more {
namespace {
const char* Mime="application/x-qcad-more-entities";
constexpr quint32 ClipboardMagic=0x51434c33;
bool validCount(QDataStream& stream,quint32& count) { stream>>count; return stream.status()==QDataStream::Ok && count<=100000; }
void writeSnapshot(QDataStream& stream,const EntitySnapshot& value) { stream<<qint32(value.entityType)<<value.payload; }
EntitySnapshot readSnapshot(QDataStream& stream) { EntitySnapshot value; qint32 type=0; stream>>type>>value.payload; value.entityType=type; return value; }
}
EditingActions::EditingActions(QCADView& view,BlockLibrary& library,QWidget& parent)
    : history(view,library),view_(view),library_(library),parent_(parent)
{ view_.nudgeRequested=[this](QPointF delta){translate(delta);}; }
EditingActions::~EditingActions() { view_.nudgeRequested={}; }
QList<MEntity*> EditingActions::selection()
{ view_.selectEntity(); return view_.GetSelectedEntityList(); }
QRectF EditingActions::selectionBox() const
{
    QRectF total; bool first=true;
    for(auto* entity:view_.GetSelectedEntityList()) {
        QRectF box; entity->WorldBox(box);
        if(first) {total=box; first=false;}
        else total=QRectF(QPointF(qMin(total.left(),box.left()),qMin(total.top(),box.top())),
                         QPointF(qMax(total.right(),box.right()),qMax(total.bottom(),box.bottom())));
    }
    return total;
}
EditingActions::Clipboard EditingActions::selectedSnapshots() const
{
    Clipboard data; data.library=library_;
    for(auto* entity:view_.GetSelectedEntityList()) {
        EntitySnapshot snapshot;
        if(!encodeEntity(*entity,snapshot).ok()) { Prompt(QStringLiteral("所选图元无法复制。")); return {}; }
        data.entities.push_back(std::move(snapshot));
    }
    return data;
}
void EditingActions::copy()
{
    selection(); const auto data=selectedSnapshots(); if(data.entities.isEmpty()) return;
    QByteArray bytes; QDataStream out(&bytes,QIODevice::WriteOnly); out.setVersion(QDataStream::Qt_5_12);
    out<<ClipboardMagic<<quint32(data.library.definitions.size());
    for(const auto& definition:data.library.definitions) {
        out<<definition.id<<definition.name<<definition.sourceBasePoint<<quint32(definition.entities.size());
        for(const auto& snapshot:definition.entities) writeSnapshot(out,snapshot);
    }
    out<<quint32(data.entities.size()); for(const auto& snapshot:data.entities) writeSnapshot(out,snapshot);
    auto* mime=new QMimeData; mime->setData(Mime,bytes); QApplication::clipboard()->setMimeData(mime);
    lastClipboard_=bytes; pasteCount_=0; Prompt(QStringLiteral("已复制 %1 个图元。").arg(data.entities.size()));
}
void EditingActions::cut() { if(selection().isEmpty()) return; copy(); remove(); }
void EditingActions::paste()
{
    const auto* mime=QApplication::clipboard()->mimeData(); if(!mime || !mime->hasFormat(Mime)) return;
    const auto bytes=mime->data(Mime); if(bytes.size()>128*1024*1024) return;
    QDataStream in(bytes); in.setVersion(QDataStream::Qt_5_12);
    quint32 magic=0,count=0; in>>magic; if(magic!=ClipboardMagic || !validCount(in,count)) return;
    Clipboard data;
    QSet<QUuid> ids; QSet<QString> names;
    for(quint32 i=0;i<count;++i) {
        BlockDefinition definition; quint32 children;
        in>>definition.id>>definition.name>>definition.sourceBasePoint;
        if(!validCount(in,children) || children==0 || definition.id.isNull() || ids.contains(definition.id) ||
           definition.name.trimmed().isEmpty() || names.contains(definition.name) || !finitePoint(definition.sourceBasePoint)) return;
        ids.insert(definition.id); names.insert(definition.name);
        for(quint32 j=0;j<children;++j) {
            auto snapshot=readSnapshot(in); std::unique_ptr<MEntity> child;
            if(!decodeEntity(snapshot,child).ok()) return;
            definition.entities.push_back(std::move(snapshot));
        }
        data.library.definitions.push_back(std::move(definition));
    }
    if(!validCount(in,count)) return;
    for(quint32 i=0;i<count;++i) data.entities.push_back(readSnapshot(in));
    if(in.status()!=QDataStream::Ok || !in.atEnd()) { Prompt(QStringLiteral("剪贴板图形数据损坏。")); return; }
    if(lastClipboard_!=bytes) {lastClipboard_=bytes;pasteCount_=0;}
    const int offset=20*(++pasteCount_); paste(data,{double(offset),double(-offset)});
}
void EditingActions::paste(const Clipboard& data,QPointF offset)
{
    std::vector<std::unique_ptr<MEntity>> entities;
    for(const auto& snapshot:data.entities) {
        std::unique_ptr<MEntity> source;
        if(!decodeEntity(snapshot,source).ok()) {Prompt(QStringLiteral("剪贴板图元无效。"));return;}
        if(auto* block=dynamic_cast<BlockEntity*>(source.get()))
            if(!findBlock(library_,block->instance.definitionId) && !findBlock(data.library,block->instance.definitionId)) return;
        std::unique_ptr<MEntity> copy(source->Copy()); if(!copy) return;
        copy->TranslateWorld(offset); entities.push_back(std::move(copy));
    }
    if(entities.empty()) return;
    selection(); view_.beginEdit();
    QSet<QString> names; for(const auto& definition:library_.definitions) names.insert(definition.name);
    for(auto definition:data.library.definitions) {
        if(findBlock(library_,definition.id)) continue;
        const auto originalName=definition.name; int suffix=2;
        while(names.contains(definition.name)) definition.name=originalName+QStringLiteral(" (%1)").arg(suffix++);
        names.insert(definition.name); library_.definitions.push_back(std::move(definition));
    }
    view_.ClearSelections();
    for(auto& entity:entities) {auto* raw=entity.release();view_.addEntity(raw);view_.AddSelection(raw);}
    view_.setModified(true); view_.update(); view_.endEdit();
}
void EditingActions::duplicate() {selection();paste(selectedSnapshots(),{20,-20});}
void EditingActions::remove()
{
    const auto selected=selection(); if(selected.isEmpty()) return;
    view_.beginEdit();
    for(auto* entity:selected) {view_.removeEntity(entity);delete entity;}
    view_.endEdit(); view_.update();
}
void EditingActions::translate(QPointF delta)
{
    const auto selected=selection(); if(selected.isEmpty() || delta.isNull()) return;
    view_.beginEdit(); for(auto* entity:selected) entity->TranslateWorld(delta);
    view_.setModified(true);view_.update();view_.endEdit();
}
void EditingActions::transform(double degrees,double scale,bool flipH,bool flipV)
{
    const auto selected=selection();
    if(selected.isEmpty() || !std::isfinite(degrees) || !std::isfinite(scale) || scale<=0 || scale>10000) return;
    if(degrees==0 && scale==1 && !flipH && !flipV) return;
    const auto center=selectionBox().center(); QTransform operation;
    operation.translate(center.x(),center.y());operation.rotate(degrees);
    operation.scale(flipH?-scale:scale,flipV?-scale:scale);operation.translate(-center.x(),-center.y());
    for(auto* entity:selected) {
        const auto result=entity->GetTransform()*operation;
        QRectF raw;entity->GetBox(raw);const auto box=result.mapRect(raw);
        if(!result.isInvertible() || !finitePoint(box.topLeft()) || !finitePoint(box.bottomRight())) {
            Prompt(QStringLiteral("变换后的尺寸或位置超出有效范围。"));return;
        }
    }
    view_.beginEdit();
    for(auto* entity:selected) entity->SetTransform(entity->GetTransform()*operation);
    view_.setModified(true);view_.update();view_.endEdit();
}
void EditingActions::transformDialog()
{
    if(selection().isEmpty()) {Prompt(QStringLiteral("请先选择图形。"));return;}
    QDialog dialog(&parent_);dialog.setWindowTitle(QStringLiteral("旋转与缩放"));dialog.setObjectName("selectionTransformDialog");
    QFormLayout layout(&dialog);QDoubleSpinBox angle,scale;
    angle.setObjectName("selectionRotation");angle.setRange(-36000,36000);angle.setDecimals(3);
    scale.setObjectName("selectionScale");scale.setRange(.001,10000);scale.setDecimals(3);scale.setValue(1);
    layout.addRow(QStringLiteral("旋转增量（度，逆时针）"),&angle);layout.addRow(QStringLiteral("缩放倍率"),&scale);
    QDialogButtonBox buttons(QDialogButtonBox::Ok|QDialogButtonBox::Cancel);layout.addRow(&buttons);
    QObject::connect(&buttons,&QDialogButtonBox::accepted,&dialog,&QDialog::accept);
    QObject::connect(&buttons,&QDialogButtonBox::rejected,&dialog,&QDialog::reject);
    if(dialog.exec()==QDialog::Accepted) transform(angle.value(),scale.value());
}
void EditingActions::align(Align mode)
{
    const auto selected=selection();if(selected.size()<2) return;const auto total=selectionBox();view_.beginEdit();
    for(auto* entity:selected) {
        QRectF box;entity->WorldBox(box);QPointF delta;
        switch(mode) {
        case Align::Left:delta.setX(total.left()-box.left());break;
        case Align::HCenter:delta.setX(total.center().x()-box.center().x());break;
        case Align::Right:delta.setX(total.right()-box.right());break;
        case Align::Top:delta.setY(total.bottom()-box.bottom());break;
        case Align::VCenter:delta.setY(total.center().y()-box.center().y());break;
        case Align::Bottom:delta.setY(total.top()-box.top());break;
        }
        entity->TranslateWorld(delta);
    }
    view_.setModified(true);view_.update();view_.endEdit();
}
void EditingActions::distribute(bool horizontal)
{
    const auto selected=selection();if(selected.size()<3) return;
    struct Item {MEntity* entity;QRectF box;};QVector<Item> items;double sizes=0;
    for(auto* entity:selected) {QRectF box;entity->WorldBox(box);items.push_back({entity,box});sizes+=horizontal?box.width():box.height();}
    std::sort(items.begin(),items.end(),[horizontal](const Item& a,const Item& b){return horizontal?a.box.left()<b.box.left():a.box.top()<b.box.top();});
    const double start=horizontal?items.first().box.left():items.first().box.top();
    const double end=horizontal?items.last().box.right():items.last().box.bottom();
    const double gap=(end-start-sizes)/(items.size()-1);double current=start;view_.beginEdit();
    for(const auto& item:items) {
        const double delta=current-(horizontal?item.box.left():item.box.top());
        item.entity->TranslateWorld(horizontal?QPointF(delta,0):QPointF(0,delta));
        current+=(horizontal?item.box.width():item.box.height())+gap;
    }
    view_.setModified(true);view_.update();view_.endEdit();
}
void EditingActions::order(bool front)
{
    const auto selected=selection();if(selected.isEmpty()) return;QList<MEntity*> remaining,ordered;
    for(auto* entity:view_.GetEntityList()) (selected.contains(entity)?ordered:remaining).push_back(entity);
    view_.reorderEntities(front?remaining+ordered:ordered+remaining);
}
void EditingActions::group()
{
    const auto selected=selection();if(selected.size()<2) {Prompt(QStringLiteral("请至少选择两个图元。"));return;}
    QSet<QString> names;for(const auto& definition:library_.definitions) names.insert(definition.name);
    int number=1;QString name;do{name=QStringLiteral("组合%1").arg(number++);}while(names.contains(name));
    const auto base=selectionBox().center();view_.beginEdit();QUuid id;
    const auto result=BlockDefinitionService().defineBlock(selected,{name,base},library_,id);
    if(!result.ok()) {view_.endEdit();Prompt(result.message);return;}
    std::unique_ptr<BlockEntity> block;
    const auto made=makeBlockEntity(*findBlock(library_,id),{QUuid::createUuid(),id,base},block);
    if(!made.ok()) {library_.definitions.removeLast();view_.endEdit();Prompt(made.message);return;}
    for(auto* entity:selected) {view_.removeEntity(entity);delete entity;}
    auto* raw=block.release();view_.addEntity(raw);view_.AddSelection(raw);view_.update();view_.endEdit();
}
void EditingActions::ungroup()
{
    const auto selected=selection();QList<MEntity*> result;view_.beginEdit();
    for(auto* entity:selected) {
        auto* block=dynamic_cast<BlockEntity*>(entity);if(!block) {result.push_back(entity);continue;}
        const auto parentTransform=block->worldTransform()*block->GetTransform();
        for(const auto& child:block->children) {
            std::unique_ptr<MEntity> copy(child->Copy());
            copy->SetTransform(copy->GetTransform()*parentTransform);
            auto* raw=copy.release();view_.addEntity(raw);result.push_back(raw);
        }
        view_.removeEntity(block);delete block;
    }
    view_.ClearSelections();for(auto* entity:result) view_.AddSelection(entity);
    view_.update();view_.endEdit();
}
void EditingActions::style(const QPen* pen,const QBrush* brush,const QFont* font,const QColor* textColor)
{
    const auto selected=view_.GetSelectedEntityList();if(selected.isEmpty()) return;view_.beginEdit();
    std::function<void(MEntity*)> apply=[&](MEntity* entity) {
        if(auto* block=dynamic_cast<BlockEntity*>(entity)) for(auto& child:block->children) apply(child.get());
        if(pen) {auto value=*pen;entity->SetPen(value);}if(brush) {auto value=*brush;entity->SetBrush(value);}
        if(auto* text=dynamic_cast<MText*>(entity)) {if(font) text->SetFont(*font);if(textColor) text->SetTextColor(*textColor);}
    };
    for(auto* entity:selected) apply(entity);view_.setModified(true);view_.update();view_.endEdit();
}
void EditingActions::addMenus()
{
    auto* window=qobject_cast<QMainWindow*>(&parent_);if(!window) return;
    auto* edit=window->menuBar()->addMenu(QStringLiteral("编辑"));
    auto* undo=history.stack.createUndoAction(&parent_,QStringLiteral("撤销"));undo->setObjectName("undoAction");undo->setShortcut(QKeySequence::Undo);edit->addAction(undo);
    auto* redo=history.stack.createRedoAction(&parent_,QStringLiteral("重做"));redo->setObjectName("redoAction");redo->setShortcuts({QKeySequence::Redo,QKeySequence("Ctrl+Shift+Z")});edit->addAction(redo);
    auto add=[this](QMenu* menu,const QString& label,const char* name,std::function<void()> call,QKeySequence shortcut=QKeySequence()) {
        auto* action=menu->addAction(label);action->setObjectName(name);if(!shortcut.isEmpty()) action->setShortcut(shortcut);
        QObject::connect(action,&QAction::triggered,&parent_,[call]{call();});return action;
    };
    edit->addSeparator();
    add(edit,QStringLiteral("剪切"),"cutAction",[this]{cut();},QKeySequence::Cut);
    add(edit,QStringLiteral("复制"),"copyAction",[this]{copy();},QKeySequence::Copy);
    add(edit,QStringLiteral("粘贴"),"pasteAction",[this]{paste();},QKeySequence::Paste);
    add(edit,QStringLiteral("快速复制"),"duplicateAction",[this]{duplicate();},QKeySequence("Ctrl+D"));
    auto* arrange=window->menuBar()->addMenu(QStringLiteral("排列"));
    add(arrange,QStringLiteral("组合"),"groupAction",[this]{group();},QKeySequence("Ctrl+G"));
    add(arrange,QStringLiteral("取消组合"),"ungroupAction",[this]{ungroup();},QKeySequence("Ctrl+Shift+G"));
    arrange->addSeparator();
    add(arrange,QStringLiteral("旋转 / 缩放"),"selectionTransformAction",[this]{transformDialog();});
    add(arrange,QStringLiteral("向右旋转 90°"),"rotateRightAction",[this]{transform(-90,1);});
    add(arrange,QStringLiteral("向左旋转 90°"),"rotateLeftAction",[this]{transform(90,1);});
    add(arrange,QStringLiteral("水平翻转"),"flipHorizontalAction",[this]{transform(0,1,true,false);});
    add(arrange,QStringLiteral("垂直翻转"),"flipVerticalAction",[this]{transform(0,1,false,true);});
    auto* alignment=arrange->addMenu(QStringLiteral("对齐"));
    add(alignment,QStringLiteral("左对齐"),"alignLeftAction",[this]{align(Align::Left);});
    add(alignment,QStringLiteral("水平居中"),"alignHCenterAction",[this]{align(Align::HCenter);});
    add(alignment,QStringLiteral("右对齐"),"alignRightAction",[this]{align(Align::Right);});
    add(alignment,QStringLiteral("顶端对齐"),"alignTopAction",[this]{align(Align::Top);});
    add(alignment,QStringLiteral("垂直居中"),"alignVCenterAction",[this]{align(Align::VCenter);});
    add(alignment,QStringLiteral("底端对齐"),"alignBottomAction",[this]{align(Align::Bottom);});
    add(alignment,QStringLiteral("水平等距分布"),"distributeHorizontalAction",[this]{distribute(true);});
    add(alignment,QStringLiteral("垂直等距分布"),"distributeVerticalAction",[this]{distribute(false);});
    arrange->addSeparator();
    add(arrange,QStringLiteral("置于顶层"),"bringFrontAction",[this]{order(true);},QKeySequence("Ctrl+Shift+]"));
    add(arrange,QStringLiteral("置于底层"),"sendBackAction",[this]{order(false);},QKeySequence("Ctrl+Shift+["));
}
}

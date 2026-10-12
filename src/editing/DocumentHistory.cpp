#include "editing/DocumentHistory.h"
#include "storage/EntityCodec.h"
#include "qcad/mainwindow.h"
#include <QUndoCommand>
namespace qcad_more {
class DocumentHistory::Change final : public QUndoCommand {
public:
    Change(DocumentHistory& history, std::unique_ptr<State> before, std::unique_ptr<State> after)
        : history_(history), before_(std::move(before)), after_(std::move(after)) { setText(QStringLiteral("编辑图形")); }
    void undo() override { history_.restore(*before_); }
    void redo() override {
        if (firstRedo_) firstRedo_=false;
        else history_.restore(*after_);
    }
private:
    DocumentHistory& history_;
    std::unique_ptr<State> before_,after_;
    bool firstRedo_=true;
};
DocumentHistory::DocumentHistory(QCADView& view, BlockLibrary& library) : view_(view),library_(library)
{
    stack.setUndoLimit(100);
    view_.editStarted=[this]{begin();}; view_.editFinished=[this]{finish();};
    QObject::connect(&stack,&QUndoStack::indexChanged,[this]{view_.setModified(!stack.isClean());});
}
DocumentHistory::~DocumentHistory() { view_.editStarted={}; view_.editFinished={}; }
std::unique_ptr<DocumentHistory::State> DocumentHistory::capture() const
{
    auto state=std::make_unique<State>(); state->library=library_;state->modified=view_.isModified();
    QDataStream bytes(&state->fingerprint,QIODevice::WriteOnly); bytes.setVersion(QDataStream::Qt_5_12);
    bytes << quint32(library_.definitions.size());
    for(const auto& definition:library_.definitions) {
        bytes << definition.id << definition.name << definition.sourceBasePoint << quint32(definition.entities.size());
        for(const auto& snapshot:definition.entities) bytes << qint32(snapshot.entityType) << snapshot.payload;
    }
    const auto entities=view_.GetEntityList(); const auto selected=view_.GetSelectedEntityList();
    bytes << quint32(entities.size());
    for(int i=0;i<entities.size();++i) {
        EntitySnapshot snapshot;
        if(!encodeEntity(*entities[i],snapshot).ok()) return {};
        bytes << qint32(snapshot.entityType) << snapshot.payload;
        state->entities.push_back(std::move(snapshot));
        if(selected.contains(entities[i])) state->selection.push_back(i);
    }
    return state;
}
void DocumentHistory::begin()
{
    if(restoring_) return;
    if(depth_++==0) {
        before_=capture();
        if(!before_) Prompt(QStringLiteral("当前图元不支持撤销记录。"));
    }
}
void DocumentHistory::finish()
{
    if(restoring_ || depth_<=0 || --depth_!=0) return;
    auto after=capture();
    if(before_ && after && before_->fingerprint!=after->fingerprint)
        stack.push(new Change(*this,std::move(before_),std::move(after)));
    else if(before_ && after) view_.setModified(before_->modified);
    before_.reset();
}
void DocumentHistory::restore(const State& state)
{
    std::vector<std::unique_ptr<MEntity>> entities;
    QList<MEntity*> pointers;
    for(const auto& snapshot:state.entities) {
        std::unique_ptr<MEntity> entity;
        if(!decodeEntity(snapshot,entity).ok()) { Prompt(QStringLiteral("撤销数据无法恢复。")); return; }
        pointers.push_back(entity.get()); entities.push_back(std::move(entity));
    }
    restoring_=true;
    view_.replaceEntities(pointers); library_=state.library;
    for(auto& entity:entities) entity.release();
    for(int index:state.selection) view_.AddSelection(pointers[index]);
    view_.update(); restoring_=false;
}
void DocumentHistory::reset() { before_.reset(); depth_=0; stack.clear(); stack.setClean(); }
void DocumentHistory::markSaved() {stack.setClean();view_.setModified(false);}
}

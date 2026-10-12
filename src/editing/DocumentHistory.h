#pragma once
#include "blocks/BlockTypes.h"
#include <QUndoStack>
#include <memory>
class QCADView;
namespace qcad_more {
class DocumentHistory {
public:
    DocumentHistory(QCADView& view, BlockLibrary& library);
    ~DocumentHistory();
    void begin();
    void finish();
    void reset();
    void markSaved();
    QUndoStack stack;
private:
    struct State {
        QVector<EntitySnapshot> entities;
        BlockLibrary library;
        QVector<int> selection;
        QByteArray fingerprint;
        bool modified=false;
    };
    class Change;
    std::unique_ptr<State> capture() const;
    void restore(const State& state);
    QCADView& view_;
    BlockLibrary& library_;
    std::unique_ptr<State> before_;
    int depth_ = 0;
    bool restoring_ = false;
};
}

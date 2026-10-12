#pragma once
#include "blocks/BlockTypes.h"
#include "common/OperationResult.h"
#include <functional>
class QCADView;
class QWidget;
namespace qcad_more {
class QcadBlockActions {
public:
    std::function<void()> documentSaved,documentReset;
    QcadBlockActions(QCADView& view, BlockLibrary& library, QWidget& dialogParent);
    OperationResult defineBlock();
    OperationResult editSelectedText();
    OperationResult beginInsertBlock();
    OperationResult editSelectedBlockTransform();
    OperationResult save();
    OperationResult saveAs();
    OperationResult open();
    OperationResult openFrom(const QString& path);
    OperationResult saveTo(const QString& path);
    OperationResult newDocument();
    bool confirmDiscard();
    QString filePath() const { return filePath_; }
private:
    void updateTitle();
    QCADView& view_;
    BlockLibrary& library_;
    QWidget& dialogParent_;
    QString filePath_;
};
}

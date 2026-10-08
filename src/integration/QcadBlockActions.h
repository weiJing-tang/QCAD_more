#pragma once
#include "blocks/BlockTypes.h"
#include "common/OperationResult.h"
class QCADView;
class QWidget;
namespace qcad_more {
class QcadBlockActions {
public:
    QcadBlockActions(QCADView& view, BlockLibrary& library, QWidget& dialogParent);
    OperationResult defineBlock();
    OperationResult editSelectedText();
    OperationResult beginInsertBlock();
    OperationResult save();
    OperationResult saveAs();
    OperationResult open();
    OperationResult newDocument();
    bool confirmDiscard();
    QString filePath() const { return filePath_; }
private:
    OperationResult saveTo(const QString& path);
    void updateTitle();
    QCADView& view_;
    BlockLibrary& library_;
    QWidget& dialogParent_;
    QString filePath_;
};
}

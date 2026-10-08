#pragma once

#include "blocks/BlockTypes.h"
#include "common/OperationResult.h"

class QCADView;
class QWidget;

namespace qcad_more {

// MainWindow owns one action coordinator per document. Dependencies are borrowed
// and must outlive it. Connect QAction signals with lambdas; QObject is not needed.
class QcadBlockActions {
public:
    QcadBlockActions(QCADView& view, BlockLibrary& library, QWidget& dialogParent);

    OperationResult defineBlock();       // Selection + name/base-point interaction.
    OperationResult editSelectedText();  // Exactly one MText + property dialog.
    OperationResult beginInsertBlock();  // Choose definition, activate mouse command.
    OperationResult saveAs();            // Choose path, then call DocumentStorage.
    OperationResult open();              // Choose path, then load full document.

private:
    QCADView& view_;
    BlockLibrary& library_;
    QWidget& dialogParent_;
};

} // namespace qcad_more

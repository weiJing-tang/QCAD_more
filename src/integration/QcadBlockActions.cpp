#include "integration/QcadBlockActions.h"

namespace qcad_more {

QcadBlockActions::QcadBlockActions(QCADView& view, BlockLibrary& library,
                                 QWidget& dialogParent)
    : view_(view), library_(library), dialogParent_(dialogParent)
{
}

OperationResult QcadBlockActions::defineBlock()
{
    // TODO: U1 - capture selection, collect block name/base point, call B1.
    return OperationResult::notImplemented("QcadBlockActions::defineBlock");
}

OperationResult QcadBlockActions::editSelectedText()
{
    // TODO: U1 - read T1 properties, show dialog, apply only on confirmation.
    return OperationResult::notImplemented("QcadBlockActions::editSelectedText");
}

OperationResult QcadBlockActions::beginInsertBlock()
{
    // TODO: U1 - choose a block; MCommand routes world positions to I1.
    return OperationResult::notImplemented("QcadBlockActions::beginInsertBlock");
}

OperationResult QcadBlockActions::saveAs()
{
    // TODO: U1 - QFileDialog + P1. Update title/dirty state only after success.
    return OperationResult::notImplemented("QcadBlockActions::saveAs");
}

OperationResult QcadBlockActions::open()
{
    // TODO: U1 - QFileDialog + P1; handle unsaved changes before replacing drawing.
    return OperationResult::notImplemented("QcadBlockActions::open");
}

} // namespace qcad_more

#pragma once
#include "text/TextProperties.h"
#include <QDialog>
class QPlainTextEdit;
class QFontComboBox;
class QDoubleSpinBox;
class QCheckBox;
class QPushButton;
namespace qcad_more {
class TextPropertiesDialog final : public QDialog {
public:
    TextPropertiesDialog(const TextProperties& properties, QWidget* parent);
    TextProperties properties() const;
private:
    QPlainTextEdit* content_;
    QFontComboBox* family_;
    QDoubleSpinBox *size_, *x_, *y_, *width_, *height_;
    QCheckBox *bold_, *italic_, *underline_;
    QPushButton* colorButton_;
    QColor color_;
};
}

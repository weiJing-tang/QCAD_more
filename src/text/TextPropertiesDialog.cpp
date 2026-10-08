#include "text/TextPropertiesDialog.h"
#include <QtWidgets>
namespace qcad_more {
TextPropertiesDialog::TextPropertiesDialog(const TextProperties& p, QWidget* parent)
    : QDialog(parent), color_(p.color)
{
    setWindowTitle(QStringLiteral("Text 文本属性")); setObjectName("textPropertiesDialog"); resize(440, 470);
    auto* form = new QFormLayout(this);
    content_ = new QPlainTextEdit(p.content, this); content_->setObjectName("textContent");
    form->addRow(QStringLiteral("内容"), content_);
    family_ = new QFontComboBox(this); family_->setCurrentFont(p.font); form->addRow(QStringLiteral("字体"), family_);
    auto spin = [this](const char* name, double value, double minimum, double maximum) {
        auto* field = new QDoubleSpinBox(this); field->setObjectName(name); field->setDecimals(3);
        field->setRange(minimum, maximum); field->setValue(value); return field;
    };
    size_ = spin("fontSize", p.font.pointSizeF() > 0 ? p.font.pointSizeF() : 12, 1, 1000);
    form->addRow(QStringLiteral("字号（pt）"), size_);
    bold_ = new QCheckBox(QStringLiteral("粗体"), this); bold_->setChecked(p.font.bold());
    italic_ = new QCheckBox(QStringLiteral("斜体"), this); italic_->setChecked(p.font.italic());
    underline_ = new QCheckBox(QStringLiteral("下划线"), this); underline_->setChecked(p.font.underline());
    auto* styles = new QHBoxLayout; styles->addWidget(bold_); styles->addWidget(italic_); styles->addWidget(underline_);
    form->addRow(QStringLiteral("字形"), styles);
    colorButton_ = new QPushButton(color_.name(), this);
    connect(colorButton_, &QPushButton::clicked, this, [this] {
        auto color = QColorDialog::getColor(color_, this, QStringLiteral("文本颜色"));
        if (color.isValid()) { color_ = color; colorButton_->setText(color.name()); }
    });
    form->addRow(QStringLiteral("颜色"), colorButton_);
    x_ = spin("textX", p.bounds.x(), -1e9, 1e9); y_ = spin("textY", p.bounds.y(), -1e9, 1e9);
    width_ = spin("textWidth", p.bounds.width(), .001, 1e9); height_ = spin("textHeight", p.bounds.height(), .001, 1e9);
    form->addRow(QStringLiteral("X（世界坐标）"), x_); form->addRow(QStringLiteral("Y（较小值）"), y_);
    form->addRow(QStringLiteral("宽度"), width_); form->addRow(QStringLiteral("高度"), height_);
    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    buttons->button(QDialogButtonBox::Ok)->setText(QStringLiteral("确定"));
    buttons->button(QDialogButtonBox::Cancel)->setText(QStringLiteral("取消"));
    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    form->addRow(buttons);
}
TextProperties TextPropertiesDialog::properties() const
{
    QFont font = family_->currentFont(); font.setPointSizeF(size_->value());
    font.setBold(bold_->isChecked()); font.setItalic(italic_->isChecked()); font.setUnderline(underline_->isChecked());
    return {content_->toPlainText(), font, color_, QRectF(x_->value(), y_->value(), width_->value(), height_->value())};
}
}

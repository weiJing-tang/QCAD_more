#include "text/TextPropertyService.h"
#include "storage/EntityCodec.h"
namespace qcad_more {
OperationResult TextPropertyService::read(MText& text, TextProperties& output) const
{
    output = {text.GetText(), text.GetFont(), text.GetTextColor(),
              QRectF(text.GetLeftTopPos(), text.GetRightBottomPos()).normalized()};
    return {};
}
OperationResult TextPropertyService::apply(MText& text, const TextProperties& properties) const
{
    const auto bounds = properties.bounds.normalized();
    if (!properties.color.isValid() || !finitePoint(bounds.topLeft()) ||
        !finitePoint(bounds.bottomRight()) || bounds.width() <= 0 || bounds.height() <= 0 ||
        (properties.font.pointSizeF() <= 0 && properties.font.pixelSize() <= 0))
        return {ErrorCode::InvalidInput, QStringLiteral("请指定有效字体、颜色和大于零的文本框尺寸。")};
    text.SetText(properties.content);
    text.SetFont(properties.font);
    text.SetTextColor(properties.color);
    text.SetLeftTopPos(bounds.topLeft());
    text.SetRightBottomPos(bounds.bottomRight());
    return {};
}
}

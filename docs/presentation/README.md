# 展示用测试证据

本目录保留当前代码在 Windows 原生 Qt 测试中生成的截图及两种 QPA 模式的测试日志，供课堂 PPT 使用。用户自行制作 PPT，本仓库不包含自动生成的演示文稿。

## 功能确认

选择复合图形并定义块、Text 属性编辑、指定位置插入块和存储均已实现。扩展包括嵌套块、实例文字编辑、普通图元与块拖动、滚轮缩放，以及编辑/排列菜单的常用图形操作。

## 实测结果

2026-10-12 执行 `scripts/build.ps1 -Check`：Release 构建成功，CTest 4/4 通过。原块套件在两种模式下分别 15 passed，新编辑套件分别 24 passed，全部 0 failed、0 skipped。数量包含初始化/清理和参数化输入，不能理解为同等数量的独立功能。新编辑测试覆盖滚轮锚点、七类基元拖动、剪贴板、微移、对齐/分布、变换/组合拆分、Undo/Redo、图层样式、全部绘图工具和旧文件升级。

- [Windows 原生日志](test-results-windows.txt)
- [离屏日志](test-results-offscreen.txt)
- [编辑操作 Windows 日志](test-results-editor-windows.txt)
- [编辑操作离屏日志](test-results-editor-offscreen.txt)

离屏模式可能输出字体路径及布局提示；原生模式正常通过。测试控制的是实际 Qt 控件；鼠标悬停使用 Qt 事件投递，点击、双击和拖框由 QtTest 驱动。

## 截图与建议用途

| 图片 | 用途 |
| --- | --- |
| assets/block-workflow.png | 原始组合与两个块实例，完整效果 |
| assets/selection.png | 多图元框选结果 |
| assets/block-defined.png | 定义块后保留原图形 |
| assets/text-properties.png | 内容、字体、字号、字形、颜色和文本框属性 |
| assets/insert-preview.png | 插入临时预览，不增加正式图元 |
| assets/transform-settings.png | 已有实例的旋转和等比缩放 |
| assets/nested-text-selector.png | 按层级路径选择嵌套块中的文字 |
| assets/dragged-blocks.png | 嵌套块与普通块多选拖动后的结果 |
| assets/all-drawing-tools.png | 七类绘图工具实际鼠标创建结果 |
| assets/common-editing.png | 编辑/排列菜单、变换和组合拆分后继续定义与插入 |

算法与操作详见 [架构说明](../architecture/block-operations.md) 和 [使用指南](../usage.md)。代码验证位于 `tests/block_workflow_tests.cpp`。修改测试后可重新运行构建检查，从 `build/cmake/native-tests/artifacts/` 更新对应截图。

## 展示时需要说明的范围

嵌套为独立递归快照；实例编辑不回写定义或其他实例。支持 32 层和等比缩放，文字属性坐标使用变换前坐标。保存 qbl v3，继续读取 v1/v2；旧版不能读取 v3。剪贴板仅为 QCAD 图元，不直接交换 PowerPoint 对象；不兼容 DWG/DXF 或课程旧 cad。

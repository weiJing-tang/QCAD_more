# 展示用测试证据

本目录保留当前代码在 Windows 原生 Qt 测试中生成的截图及两种 QPA 模式的测试日志，供课堂 PPT 使用。用户自行制作 PPT，本仓库不包含自动生成的演示文稿。

## 功能确认

选择复合图形并定义块、Text 属性编辑、指定位置插入块和存储均已实现。扩展包括嵌套块、当前实例内部文字编辑、旋转、等比缩放和按住左键直接拖动块。

## 实测结果

2026-10-12 执行 `scripts/build.ps1 -Check`：Release 构建成功，CTest 2/2 通过。offscreen 和 Windows 原生模式各 15 passed、0 failed、0 skipped；每组包含初始化/清理、九个非参数化业务测试、文件损坏的四组参数化输入，并非 30 个不同业务测试。新增拖动测试验证嵌套与变换块、缩放位移、多选、松开提交、保存恢复、Esc/右键/工具切换取消、小幅点击抖动及 Shift/框选回归。

- [Windows 原生日志](test-results-windows.txt)
- [离屏日志](test-results-offscreen.txt)

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

算法与操作详见 [架构说明](../architecture/block-operations.md) 和 [使用指南](../usage.md)。代码验证位于 `tests/block_workflow_tests.cpp`。修改测试后可重新运行构建检查，从 `build/cmake/native-tests/artifacts/` 更新对应截图。

## 展示时需要说明的范围

嵌套为独立递归快照；当前实例编辑不会同步回写定义或其他实例。支持最多 32 层嵌套和等比缩放；块内文字坐标使用变换前坐标。保存为 qbl v2，继续读取 v1；旧版程序不能读取 v2。不兼容 DWG/DXF 或课程旧 cad 格式。

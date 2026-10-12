# 图形块操作：实现与接口

## 基线和分层

基于本地课程 `../QCAD/QCAD`，保留 MEntity、MText、QCADView 与原窗口/工具栏体系。已采用的源码位于 src/qcad，源文件 SHA-256 和编码记录在 docs/provenance/course-source.json。新工程能独立编译，不再读取相邻 QCAD 目录。课程 Visual Studio 配置依赖已不存在的机器路径，因此使用 CMake 构建这些实际需要的源码。

产品使用 C++14 / Qt 5；不引入新的 CAD 框架或业务运行时。

| 模块 | 入口 | 实现职责 |
| --- | --- | --- |
| selection | SelectionService、SelectCommand | 获取当前选择集、点选、拖框完全包含、Shift 追加/取消 |
| blocks | BlockDefinitionService | 命名、基点、图元独立快照、重复名称检查 |
| blocks | BlockEntity、BlockInsertionService | 世界坐标实例、子图元绘制/拾取/整体平移、临时预览及提交 |
| text | TextPropertyService、TextPropertiesDialog | 完整读取/编辑文字、字体、颜色和文本框 |
| storage | EntityCodec、DocumentStorage | 图元载荷、版本化文件、原子保存、完整校验后加载 |
| integration | QcadBlockActions、PointCommand、DrawEntityCommand | 菜单、对话框、基点选择、连续插入、绘图预览与取消 |
| editing | DocumentHistory、EditingActions | 文档撤销事务、剪贴板、排列、组合拆分、仿射变换和样式 |

## 数据与所有权

- BlockLibrary 由 MainWindow 持有，定义名称唯一，定义 UUID 稳定。定义创建后不通过实例修改。
- BlockDefinition 保存原始世界基点，EntitySnapshot 的几何数据已平移到以该基点为原点的局部坐标。
- 定义过程通过 EntityCodec 编解码获取独立副本，再转换局部坐标。整个过程中原图元不变；全部成功后才发布定义及返回 UUID。
- BlockEntity 继承 MEntity，保存 BlockInstance 和独占的子图元。子图元由定义快照生成，使用世界坐标供原 Draw 接口绘制。整体移动同时更新实例位置和子图元坐标。
- 普通图元与块实例归 QCADView 的图元容器所有。预览是 QCADView 单独持有的 unique_ptr，不进入正式图元列表；取消只释放预览。
- 选择结果中的 MEntity* 是借用指针。只在 GUI 线程、所属文档仍有效期间使用；选择命令和加载替换负责清理过期状态。
- MEntity 与 MCommand 已改为虚析构。切换命令先 Cancel，再销毁旧命令。块子图元由 unique_ptr 管理。
- 所有业务坐标为世界坐标；鼠标事件只在命令入口通过 ScreentoWorld 转换。

## 结果与交互

OperationResult 的 None 为成功，其余区分主动取消、参数错误、空选择、重名、不支持的图元、块不存在、IO 错误和格式错误。未完成的骨架入口已全部替换。

TextProperties 是完整属性集，不是部分更新；校验通过后一次性修改。独立 Text 及块内选定 Text 支持 Unicode、多行、本机 QFontDatabase 字体列表、0.1–10000 pt 可输入字号、字体样式、颜色和归一化文本框；取消对话框不应用。独立 Text 支持双击；块内 Text 通过选中块后点击 Text 属性或 Ctrl+E 选择层级路径。

QcadBlockActions 的 defineBlock/beginInsertBlock 返回成功表示交互已启动；真正定义或插入在 PointCommand 接收到画布位置时执行。提交失败显示消息，成功后刷新并标记文档已修改。

首次插入后命令保留，可连续提交。Esc/右键结束；切换工具也清除预览。定义成功后通过事件队列切回选择命令，避免在正在执行的命令回调中销毁自身。

选择状态下，左键命中图元或已选图元内部且未按 Shift 时进入拖动候选，可带动整个选择集。超过 QApplication::startDragDistance 后深复制为临时预览，隐藏原件绘制但不改写数据。世界位移为 `ScreentoWorld(current) - ScreentoWorld(press)`，松开时调用 TranslateWorld(delta)，作为一次撤销事务提交。取消只释放预览并恢复光标。空白与 Shift 仍用于框选/追加。

滚轮以当前鼠标世界点为锚：先记录 anchor，再将比例乘以 `1.15^(angleDelta/120)` 并限制到 0.01–100；重新计算浮点原点，使 anchor 仍映射到同一屏幕位置。触控板可使用 pixelDelta。中键拖动仅平移视图原点，比例输入与实际比例同步。视图变动不修改文档、不进入 Undo。

MEntity 的 m_transform 保留旋转、等比缩放和翻转。WorldBox 在原 GetBox 后映射变换；HitTest 将点逆变换并换算拾取半径。绘制通过 QCADView::screenTransform 共轭至屏幕空间，组合父块变换。TranslateWorld 在单位变换时沿用旧 Move，否则将世界平移附加到矩阵；避免旋转图形沿错误方向移动。所有基元类型保持不变。

DocumentHistory 在事务外层捕获 before/after（实体快照、块库、选择），不同才压入 QUndoStack；首次 redo 不重复执行已提交操作。Undo/Redo 全部解码后替换场景与块库，并恢复选择；dirty 以 stack 的 clean 保存位置为准。上限 100 次编辑，打开/新建清空历史。嵌套 begin/end 将批量操作合为一次记录。

剪贴板为有标识的 Qt MIME 图元数据，包含所需块库与快照。粘贴先解码、复制并重新生成实例 UUID，再合并定义与命名冲突，平移后一次性提交。组合用已有块定义/实例机制替换选择集；拆组将父变换附加到各子图元变换后拆出一层。对齐和分布按 WorldBox 计算世界平移，图层操作保留选择对象内部相对次序。

## qbl 文件格式 v3（兼容读取 v1/v2）

使用 QDataStream，固定流版本为 Qt_5_12，默认大端序。没有裸写实体内存或指针。

1. 文件头：quint32 magic `0x51424c4b`（QBLK）、quint32 格式版本 `3`。
2. 块库：quint32 定义数；每个定义为 QUuid、QString 名称、QPointF 原基点、quint32 子图元数；每个子图元是 qint32 类型和 QByteArray 载荷。
3. 场景：quint32 实体数；每个实体先写 qint32 类型，再写 QByteArray 载荷。块载荷为画笔/画刷、实例 UUID、定义 UUID、位置、角度、等比缩放、子图元数量及递归快照。子块作为自包含快照，无循环定义引用。v1 分支继续读取实例 UUID、定义 UUID、位置，并按定义重建。
4. 普通实体载荷先保存 QPen/QBrush，再由相应图元 Serialize 保存几何数据；Text 额外包含 QString 内容、QFont、QColor，保持复制/重开后的完整属性。
5. v3 图元载荷增加 QEN3 标识：标识 + 原 Serialize 内容 + QTransform。递归子块同样使用新载荷；旧载荷没有标识，解码时保持单位变换，因此真实 v1/v2 数据仍可读。Undo 历史、剪贴板和视图状态不写入文件。

支持直线、矩形、圆、圆弧、椭圆、多边形、Text 和嵌套块。递归编解码限制 32 层，单文件 128 MiB、各级记录数 100000；这是加载保护，不是性能承诺。块实例保存完整快照，以保留单实例文字修改。

保存先构建并验证完整数据，再由 QSaveFile 替换目标。加载先构建临时图元和块库，检查版本、类型、流状态、UUID 唯一性和引用完整性；全部通过后替换当前文档。失败保留当前图元、选择、块库和目标文件。

## 修复的接入障碍

- 原框选为 if(0)，现使用 SelectCommand 和统一选择服务。
- 原绘图命令混用预览与正式实体、部分取消/析构路径会误删图元；新 DrawEntityCommand 统一预览和提交生命周期，复用原图元类。
- MEntity::Serialize 原保存分支仍使用未初始化变量修改画笔；现直接对称序列化 QPen/QBrush。
- Text 的字体、颜色和 Unicode 复制/保存已补全；矩形/椭圆平移修复为纯平移，避免基点等于角点时变成缩放。
- 背景 QPainter 在旧图元各自启动 QPainter 前结束，避免画笔重入。
- 原 saveFile/saveAsFile 占位已替换，文件菜单、快捷键和标题修改状态已接入。
- 修复主窗口创建工具栏前解引用未初始化指针、重画连接不存在槽函数和 Delete 菜单未工作的实际问题。

## 验证和后续边界

block_workflow_tests 与 editor_workflow_tests 都在 offscreen 和 Windows 原生 QPA 运行。原套件验证块/Text/失败保护；新增套件验证滚轮与平移、七类普通图元拖动/Undo、剪贴板/微移、六种对齐和等距分布、仿射变换/组合拆分/保存、绘图/定义/插入/Text 的 Undo、图层样式、全部绘图工具取消，以及真实 v1 示例与旧格式 v2 数据。保存与打开使用菜单共用的带路径入口进行测试，文件选择弹窗由 Qt 提供。

块变换为 T(position)·R(angle)·S(scale)·T(-position)，子图元仍保留变换前坐标。绘制时将世界变换共轭到屏幕坐标，并与父块绘制变换组合；全部图元使用同一个绘制变换，因此矩形、椭圆和文字的方向均保持正确。GetBox 逐层变换轴对齐包围盒。移动同时平移子图元和变换基点。

Text 属性入口可递归列出选中块的内部文字，通过路径选择子图元，只修改当前实例快照。块定义不会同步改变。支持水平/垂直翻转，暂不支持非等比缩放、八点尺寸手柄、DXF/DWG 或原课程 .cad；编辑块定义本身需重新定义新名称。

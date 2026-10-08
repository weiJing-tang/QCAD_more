# 图形块操作：接口与接入设计

## 当前范围

本阶段完成 C++14 / Qt 5.12 功能骨架，包含五个业务模块和一个界面接入点。所有业务入口目前返回 `ErrorCode::NotImplemented`，不改变图元、输出参数、块库或磁盘文件。没有可运行的新增菜单，也没有宣称已完成块功能。

接口针对本机 `../QCAD/QCAD` 课程源码的 `QCADView`、`MEntity`、`MText`，不是商业 QCAD 的 RDocument 接口。`../QCAD/QCAD2` 是另一份副本，本阶段未混用。新库只依赖 Qt 的值类型，通过前置声明预留课程类接入，不复制整个旧工程。

## 文件与职责

| 需求 | 文件（位于 src/） | 主要接口 | 后续实现编号 |
| --- | --- | --- | --- |
| 选择复合图形 | selection/SelectionService.h/.cpp | captureSelected、selectInRectangle | S1 |
| 定义块 | blocks/BlockTypes.h、BlockDefinitionService.h/.cpp | defineBlock | B1 |
| 编辑 Text 属性 | text/TextProperties.h、TextPropertyService.h/.cpp | read、apply | T1 |
| 选择位置并插入块 | blocks/BlockInsertionService.h/.cpp | previewAt、insertAt、cancelPreview | I1 |
| 存储及重新打开 | storage/DocumentStorage.h/.cpp | save、load | P1 |
| 菜单和交互接入 | integration/QcadBlockActions.h/.cpp | defineBlock、editSelectedText、beginInsertBlock、saveAs、open | U1 |
| 共用操作结果 | common/OperationResult.h | ErrorCode、OperationResult::ok | 共用 |

各模块直接协作，不增加通用插件框架、数据库或多层服务包装。UI 负责收集参数、显示消息和成功后的重绘/修改标记；业务模块负责实际操作和结果。鼠标预览自身负责请求重绘。

## 数据与接口约定

- `BlockLibrary` 由文档持有。每个定义包含稳定 UUID、唯一名称、原始基点和图元快照；重命名不能改变 UUID。
- `EntitySnapshot` 持有自己的字节数据。几何坐标相对定义基点；定义时必须复制数据，不能移动原始实体。载荷应使用修复后的实体编解码器，由文件格式版本统一约束；目前未实现编码。
- `BlockInstance` 只保存实例 UUID、定义 UUID 和插入位置，供后续块实体使用。定义保存在文档块库，插入后的实体由原有图元容器管理；不另造一套独立的绘图场景。
- 第一版只约定平移插入，不提前引入旋转、缩放、嵌套块。遇到暂不支持的图元返回 `UnsupportedEntity`，不能静默丢失内容。
- 所有位置、文本框和选择框都是世界坐标。鼠标输入通过 `QCADView::ScreentoWorld` 转换一次。选择框采用完全包含规则，并支持替换/追加选择。
- 选择输出是借用的 `MEntity*`，只能在 GUI 线程中短期使用；删除实体、关闭或加载文档后不得继续使用。其他图元/视图引用同样不转移所有权。
- `TextProperties` 为完整属性集合：Unicode 内容、字体（含字号和字形）、颜色、文本框。先读取，确认对话框后整体应用；取消不修改。首版编辑单个选中的 MText；块定义内部文字编辑及实例属性覆盖暂不纳入。
- `OperationResult::ok()` 仅当 code 为 None 时为真。失败不得部分修改文档、块库或输出；UI 应显示 message。用户主动取消使用 Cancelled，不当作成功保存。

## 交互流程

1. 选择多个图元，输入块名并指定基点，调用 defineBlock；成功后在块库中列出新定义，原图元保留。
2. 选择一个 Text，read 读取属性，弹窗编辑，确认后 apply 并刷新视图。
3. 选择块定义，启动插入命令；移动鼠标调用 previewAt，左键调用 insertAt，Esc/右键调用 cancelPreview。预览不进入正式文档，每次成功插入生成独立实例 ID。
4. 保存文档时一起存储普通图元、完整块定义、实例引用和文本属性。重新打开时恢复完整内容，用于验证存储确实可用。

## 与课程工程接入

后续以 `../QCAD/QCAD` 为基线，将正式采用的课程源码和必要工程文件纳入本仓库并记录来源后再接菜单；本次没有改动该目录。现有 Visual Studio/Qt 工程可以添加这里的 .cpp/.h 文件并把 src 加入包含目录；现有工程继续负责 Widgets、moc 和窗口入口。也可从主工程 CMake 通过 add_subdirectory 引入，再链接 qcad_more；当前 CMake 仅用于新模块编译，不替代课程项目构建系统。

| 原文件/接口 | 已核实情况 | 接入前需要做的工作 |
| --- | --- | --- |
| QCADView.h 的 GetSelectedEntityList、AddSelection、GetEntityList | 已存在 | S1 直接复用，明确选择有效期 |
| MSelectCmd.cpp 框选分支 | `if (0)` 占位，每次点击先清空选择 | 接到 S1，补齐框选与追加选择交互 |
| ENTITY.H 的 MBlock | 有声明，在检查的 .cpp/.CPP 中未找到成员实现 | 完成可绘制、可拾取、可移动、可复制的块实体，接到 BlockInstance |
| CREATEBLOCK.CPP | 遗留 UINT、Position、AfxGetApp 等旧接口 | 不直接纳入 Qt 构建；沿当前 MCommand 风格实现插入命令 |
| MText | 有内容、位置、字体 setter；缺少字体 getter | 补 GetFont 与 QString setter，避免本地编码转换 |
| TEXT.cpp 的 Copy/复制构造/赋值与 Serialize | 字体和颜色复制不完整；序列化未保存文本颜色 | T1/P1 一并补齐，保证复制/重开后属性不丢失 |
| ENTITY.CPP 的 MEntity::Serialize | 保存分支后仍用未初始化读取变量重设画笔 | 必须先修复；不能直接拿此函数生成块快照 |
| ENTITY.H 的 MEntity 析构 | 非虚析构 | 明确实体销毁策略后再引入基类所有权指针，避免通过基类错误销毁派生对象 |
| mainwindow.cpp 的 saveFile/saveAsFile | 实际图元写入被注释、提前 return；另存为空 | U1 调用 P1 替换占位保存路径 |

## 存储协议计划

由 P1 实现单一文档文件，第一版使用 Qt 自带 QDataStream，不添加第三方格式库。文件头包含 magic、文档格式版本；显式固定 QDataStream 版本。依次保存块定义、普通图元和块实例，实体记录包含类型标记和载荷，文本记录覆盖所有属性。正式编码前确定并记录字段顺序。

使用 QSaveFile 在完整写入成功后替换目标文件。加载时先校验版本、类型、读取状态、定义 ID 与实例引用，再整体替换旧文档。格式不支持或文件不完整时保留当前内容。当前不承诺兼容课程示例 .cad 文件，也不声称支持 DXF/DWG。

## 实现顺序与必要验收

1. 接入课程基线，修复实际阻碍复制、资源销毁和序列化的问题；实现 S1。
2. 完成 T1 和 B1：多图元成块后原图形不变，文字属性保留。
3. 完成块实体和 I1：同一定义在两个位置插入，取消预览不遗留图元。
4. 完成 P1 和 U1：保存再打开后定义、实例位置和文本属性一致；保存失败不覆盖原文件。

每一步集中做一次与改动相关的构建和正常流程验证，出现实际问题再增加对应回归检查。当前阶段只检查结构、接口占位行为和可用环境下的构建，不编造未完成业务的测试结果。

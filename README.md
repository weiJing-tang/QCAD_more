# QCAD_more

计算机图形学课程 C++ 项目，目标是在课程 QCAD 中实现图形块定义与插入、Text 属性编辑和存储。

**当前状态：已搭建功能文件结构、数据类型和接口，业务实现及界面接入尚未完成。** 未实现入口统一返回 NotImplemented，不修改图形或写文件。

## 开发入口

开始开发前阅读 [AGENTS.md](AGENTS.md)。优先复用现有代码，保持实现简单，按风险集中验证；每次开发必须填写 [开发记录](docs/development/)。

完整接口约定、原工程接入点、已知问题和实现顺序见 [图形块操作设计](docs/architecture/block-operations.md)。

## 文件结构

```text
QCAD_more/
├── AGENTS.md
├── CMakeLists.txt              # 新功能静态库，不是完整 QCAD 应用
├── src/
│   ├── common/                # 操作结果与错误类型
│   ├── selection/             # 复合图形选择
│   ├── blocks/                # 块数据、定义、预览和插入
│   ├── text/                  # Text 属性读取与修改
│   ├── storage/               # 完整文档保存与读取
│   └── integration/           # 菜单和交互的接入接口
└── docs/
    ├── architecture/          # 接口契约与接入说明
    └── development/           # 每次开发记录及模板
```

## 构建

新增模块要求 C++14、CMake 3.16+ 和 Qt 5.12+ 的 Core/Gui 开发包。实际接入的课程应用还需要 Qt Widgets，并继续沿用其 Visual Studio/Qt 工程。源码依据为本机相邻的 `../QCAD/QCAD`，目前未复制进本仓库。

在安装了匹配编译器和 Qt 开发包的终端中执行；把路径替换成实际 Qt 安装目录：

```powershell
cmake -S . -B build -DCMAKE_PREFIX_PATH="D:/path/to/Qt/5.12.8/msvc2017_64"
cmake --build build --config Release
```

这里只生成 qcad_more 静态库，没有应用入口，也不会启动 QCAD。本机课程工程记录的 Qt 路径当前不存在，因此尚不能宣称完整编译通过；安装/定位开发包后再构建，无需下载新的 CAD 框架。

## 开发记录

使用 [模板](docs/development/TEMPLATE.md)，按 `YYYY-MM-DD-NN-简短主题.md` 命名。最新功能骨架记录见 [2026-10-08-02](docs/development/2026-10-08-02-图形块功能骨架.md)。

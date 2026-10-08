# 代码与工具来源

## 课程 QCAD

2026-10-08 从用户提供的本地 `../QCAD/QCAD` 引入需要的 21 份 C++ 源文件/头文件及 images 资源，放入 src/qcad。未引入 QCAD2、副本编译产物或旧 MFC 块命令。

[course-source.json](course-source.json) 记录导入前源码 SHA-256、文件名和原编码。源码统一转为 UTF-8；现文件包含本项目必要修复，因此其当前校验值与源清单不同。原有版权声明保留，例如 diagramitem.h 中的 Qt 示例 BSD 声明。课程目录没有可核实的统一许可证或 Git 上游提交，本仓库不将其重新标记为某个第三方许可证。

复用内容包括图元与几何操作、QPainter 绘制、窗口和工具栏、坐标转换及缩放命令。具体接入修复见架构文档。本机相邻源目录未被修改。

## Qt 与工具

- Qt 5.15.2 `win64_msvc2019_64`：通过 [Qt 官方下载源](https://download.qt.io/online/qtsdkrepository/windows_x86/desktop/qt5_5152/) 获取 qtbase 开发包，使用 Core/Gui/Widgets/Test；位于 build/Qt，不纳入 Git。项目动态链接 Qt。
- aqtinstall 3.3.0：[官方仓库](https://github.com/miurahr/aqtinstall)、[使用说明](https://aqtinstall.readthedocs.io/en/latest/getting_started.html)。已安装元数据标明 MIT License；只用于独立环境中下载 SDK。
- Visual Studio 2022 / MSVC 19.44 和 CMake：使用本机已有安装，不复制或卸载。
- 测试中使用本机 Windows 的微软雅黑字体文件进行离屏渲染，不把字体文件纳入项目或分发。

辅助 Python 3.11 环境单独创建在 build/qt-dev，与 lerobot 独立。早期借用 lerobot 的解释器建立过临时 venv；未向 lerobot 安装包，该临时目录已删除。

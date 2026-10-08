#include "mainwindow.h"
#include <QApplication>
#include <QFileInfo>
int main(int argc, char* argv[])
{
    QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QApplication app(argc, argv);
    app.setApplicationName("QCAD_more");
    MainWindow window;
    window.resize(1200, 800);
    window.show();
    return app.exec();
}

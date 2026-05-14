/// SmartMail Client - Qt6 前端应用入口
#include <QApplication>
#include "views/MainWindow.hpp"

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    app.setApplicationName("SmartMail Desktop");
    app.setApplicationVersion("1.0.0");

    // 占位 - 后续阶段实现 MainWindow
    // SmartMail::MainWindow window;
    // window.show();

    return app.exec();
}

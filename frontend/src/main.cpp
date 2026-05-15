/// SmartMail Client - Qt6 前端应用入口
#include <QApplication>
#include <QFile>
#include <QString>
#include "views/MainWindow.hpp"

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    app.setApplicationName("SmartMail Desktop");
    app.setApplicationVersion("1.0.0");

    // 加载 Midnight Paper 主题样式
    QFile styleFile(":/resources/style.qss");
    if (styleFile.open(QFile::ReadOnly | QFile::Text)) {
        QString style = styleFile.readAll();
        app.setStyleSheet(style);
        styleFile.close();
    }

    SmartMail::MainWindow window;
    window.show();

    return app.exec();
}

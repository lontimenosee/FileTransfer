#include "mainwindow.h"

#include <QApplication>
#include <QNetworkProxy>
#include <QNetworkProxyFactory>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    QNetworkProxyFactory::setUseSystemConfiguration(false);
    QNetworkProxy::setApplicationProxy(QNetworkProxy::NoProxy);
    MainWindow window;
    window.show();
    return app.exec();
}

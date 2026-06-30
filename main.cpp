#include "mainwindow.h"

#include <QApplication>
#include <QCommandLineParser>
#include <QSplashScreen>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    //QSplashScreen *splash = new QSplashScreen;
    //splash->setPixmap(QPixmap(":/Images/Images/loading_background.png"));
    //splash->show();

    MainWindow w(argc, argv);
    w.show();
    return a.exec();
}

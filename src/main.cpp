#include "mainwindow.h"
#include <QApplication>
#include <cstdio>

int main(int argc, char *argv[])
{
    freopen("stdout.log", "w", stdout);
    freopen("stderr.log", "w", stderr);

    QApplication a(argc, argv);

    MainWindow w;

    w.show();
    return a.exec();
}

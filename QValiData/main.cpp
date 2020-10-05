#include "mainwindow.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    a.setOrganizationName("jchen-cs");
    a.setOrganizationDomain("jchen-cs");
    MainWindow w;
    w.show();

    return a.exec();
}

#include "maincontrolwindow.h"
#include <QApplication>
#include <QSettings>
#include <QDebug>

int main(int argc, char *argv[]) {
    QApplication a(argc, argv);

    QGuiApplication::setQuitOnLastWindowClosed(false);
    QCoreApplication::setOrganizationName("mosher.mine.nu");

    QSettings::setDefaultFormat(QSettings::IniFormat);

    MainControlWindow w;
    w.show();

    return QApplication::exec();
}

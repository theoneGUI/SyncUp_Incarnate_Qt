#include "GUI.h"
#include <QtWidgets/QApplication>

#ifndef QT_NO_SYSTEMTRAYICON

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    if (!QSystemTrayIcon::isSystemTrayAvailable()) {
        QMessageBox::critical(nullptr, QObject::tr("SyncUp"),
            QObject::tr("I couldn't detect any system tray "
                "on this system."));
        return 1;
    }
    QApplication::setQuitOnLastWindowClosed(false);

    GUI w;
    w.show();
    return a.exec();
}

#else
int main(int argc, char* argv[])
{
    QApplication a(argc, argv);
    GUI w;
    w.show();
    return a.exec();
}
#endif
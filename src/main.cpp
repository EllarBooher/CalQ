#include "mainwindow.h"

#include <QApplication>

// NOLINTNEXTLINE
int main(int argc, char *argv[])
{
    QApplication const app(argc, argv);
    MainWindow window;
    window.show();
    return QApplication::exec();
}

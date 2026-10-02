#include "mainwindow.h"
#include <visa.h>

#include <QApplication>

int main( int argc, char *argv[] )
{
    QApplication a( argc, argv );
    MainWindow w;
    w.show();
    return QApplication::exec();
}

#include "mainwindow.h"
#include <visa.h>

#include <QApplication>

int main( int argc, char *argv[] )
{
    ViSession rm, znb;
    viOpenDefaultRM( &rm );
    QApplication a( argc, argv );
    MainWindow w;
    w.show();
    viClose( znb );
    viClose( rm );
    return QApplication::exec();
}

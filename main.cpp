#include "mainwindow.h"
#include <visa.h>
#include <QMessageBox>
#include <QApplication>
#include <windows.h>

int main( int argc, char *argv[] )
{
    try
    {
        QApplication a( argc, argv );
        MainWindow w;
        w.show();
        return QApplication::exec();
    }
    catch ( const VisaError &e )
    {
        auto errorMsg = QString( L"Ошибка VISA в функции %1\nСтатус: %2\nСообщение: %3" )
                            .arg( QString::fromStdString( e.context() ) )
                            .arg( e.status() )
                            .arg( e.what() )
                            .toStdString();

        MessageBoxA(
            NULL,
            errorMsg.data(),
            "Ошибка",
            MB_OK | MB_ICONERROR );
    }
    catch ( const std::exception &e )
    {
        MessageBoxA(
            NULL,
            e.what(),
            "Ошибка",
            MB_OK | MB_ICONERROR );
        return -1;
    }
    catch ( ... )
    {
        QMessageBox::critical( nullptr, "Неизвестная ошибка",
                               "Произошло необработанное исключение неизвестного типа." );
        return -1;
    }
}

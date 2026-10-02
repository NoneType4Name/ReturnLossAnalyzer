#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include <QTimer>
#include <QSerialPortInfo>

MainWindow::MainWindow( QWidget *parent ) :
    QMainWindow( parent ), ui( new Ui::MainWindow ), comPortsTimer( new QTimer( this ) )
{
    ui->setupUi( this );
    QObject::connect( comPortsTimer, &QTimer::timeout, this, [ & ]()
                      { auto selected{ui->comPortsComboBox->currentText()};
                      ui->comPortsComboBox->clear();
                    for(auto port:QSerialPortInfo::availablePorts())
                    {
                        ui->comPortsComboBox->addItem(port.portName(), port.systemLocation());
                    } } );

    comPortsTimer->start( 100 );
}

MainWindow::~MainWindow()
{
    delete ui;
    delete comPortsTimer;
}

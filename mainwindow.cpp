#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include <QTimer>
#include <QtConcurrent>
#include <qtconcurrentrun.h>
#include "Visa.hpp"
#include "visaResourceManager.hpp"

MainWindow::MainWindow( QWidget *parent ) :
    QMainWindow( parent ), ui( new Ui::MainWindow )
{
    ui->setupUi( this );
    connect( &reloadWatcher, &QFutureWatcher<QVector<QString>>::finished, this, &MainWindow::on_reloadPushButton_finished );
}

MainWindow::~MainWindow()
{
    delete inst;
    inst = 0;
    delete ui;
}

void MainWindow::on_connectButton_clicked()
{
    if ( inst )
    {
        delete inst;
        inst = 0;
        ui->connectButton->setText( "connect" );
        ui->testButton->setEnabled( 0 );
    }
    else
    {
        auto st = VisaResourceManager::instance().openSession( ui->comPortsComboBox->currentText(), 500 );
        if ( st == VI_NULL )
        {
            ui->errorLabel->setText( VisaResourceManager::instance().lastError() );
        }
        inst = new VisaInstrument( st );
        ui->connectButton->setText( "disconnect" );
        ui->testButton->setEnabled( 1 );
    }
}

void MainWindow::on_reloadPushButton_clicked()
{
    ui->reloadPushButton->setEnabled( 0 );
    QFuture<QVector<QString>> future = QtConcurrent::run( []()
                                                          { return VisaResourceManager::instance().listResources(); } );
    reloadWatcher.setFuture( future );
}

void MainWindow::on_reloadPushButton_finished()
{
    auto selected { ui->comPortsComboBox->currentText() };
    ui->comPortsComboBox->clear();
    for ( auto rsc : reloadWatcher.future().result() )
    {
        ui->comPortsComboBox->addItem( rsc );
        if ( rsc == selected )
        {
            ui->comPortsComboBox->setCurrentText( selected );
        }
    }
    ui->reloadPushButton->setEnabled( 1 );
    ui->errorLabel->setText( VisaResourceManager::instance().lastError() );
}

void MainWindow::on_testButton_clicked()
{
    // todo
}

void MainWindow::on_lockPushButton_clicked()
{
    // todo
}

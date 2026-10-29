#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include <QTimer>
#include <QtConcurrent>
#include <cmath>
#include <QMessageBox>
#include <qcombobox.h>
#include <qmessagebox.h>
#include <qtconcurrentrun.h>
#include "Visa.hpp"
#include "visaResourceManager.hpp"

MainWindow::MainWindow( QWidget *parent ) :
    QMainWindow( parent ), ui( new Ui::MainWindow )
{
    ui->setupUi( this );
    connect( &reloadWatcher, &QFutureWatcher<QVector<QString>>::finished, this, &MainWindow::onReloadPushButtonFinished );
    connect( &testReturnWatcher, &QFutureWatcher<QVector<float>>::finished, this, &MainWindow::onTestButtonReturn );
}

MainWindow::~MainWindow()
{
    QThreadPool::globalInstance()->waitForDone();
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
        ui->nameLabel->clear();
        ui->connectButton->setText( "connect" );
        ui->testButton->setEnabled( 0 );
        ui->reloadPushButton->setEnabled( 1 );
    }
    else
    {
        auto st = VisaResourceManager::instance().openSession( ui->comPortsComboBox->currentText(), 2000 );
        if ( st == VI_NULL )
        {
            QMessageBox::critical( this, "Error", VisaResourceManager::instance().lastError() );
        }
        inst = new VisaInstrument( st );
        ui->connectButton->setText( "disconnect" );
        ui->nameLabel->setText( QString::fromStdString( inst->query( "*IDN?\n" ) ) );
        ui->testButton->setEnabled( 1 );
        ui->reloadPushButton->setEnabled( 0 );
    }
}

void MainWindow::on_reloadPushButton_clicked()
{
    ui->reloadPushButton->setEnabled( 0 );
    ui->connectButton->setEnabled( 0 );
    QFuture<QVector<QString>> future = QtConcurrent::run( []()
                                                          { return VisaResourceManager::instance().listResources(); } );
    reloadWatcher.setFuture( future );
}

void MainWindow::onReloadPushButtonFinished()
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
    ui->connectButton->setEnabled( 1 );
    QMessageBox::critical( this, "Error", VisaResourceManager::instance().lastError() );
}

void MainWindow::onTestButtonReturn()
{
    auto ret { testReturnWatcher.future().result() };
    if ( ret.empty() )
    {
        QMessageBox::information( this, "Measure result", "Не подходит заданным условиям." );
    }
    else
    {
        QString bands;
        for ( auto band : ret )
        {
            bands += QString::number( band / ( std::pow( 10, ui->startUnitComboBox->currentIndex() * 3 ) ) ) + ' ' + ui->startUnitComboBox->currentText() + ", ";
        }
        if ( bands.size() )
            bands.resize( bands.size() - 2 );
        QString ans { QString( "Подходящих каналов: %1, на частотах: \n%2" ).arg( ret.size() ).arg( bands ) };
        QMessageBox::information( this, "Measure result", ans );
    }
    ui->testButton->setEnabled( 1 );
    ui->startHzDoubleSpinBox->setEnabled( 1 );
    ui->startUnitComboBox->setEnabled( 1 );
    ui->endHzDoubleSpinBox->setEnabled( 1 );
    ui->endUnitComboBox->setEnabled( 1 );
    ui->pointsCountSpinBox->setEnabled( 1 );
    ui->bandWidthComboBox->setEnabled( 1 );
    ui->bandWidthSpinBox->setEnabled( 1 );
    ui->connectButton->setEnabled( 1 );
    ui->comPortsComboBox->setEnabled( 1 );
}

void MainWindow::on_testButton_clicked()
{
    ui->testButton->setEnabled( 0 );
    ui->startHzDoubleSpinBox->setEnabled( 0 );
    ui->startUnitComboBox->setEnabled( 0 );
    ui->endHzDoubleSpinBox->setEnabled( 0 );
    ui->endUnitComboBox->setEnabled( 0 );
    ui->pointsCountSpinBox->setEnabled( 0 );
    ui->bandWidthComboBox->setEnabled( 0 );
    ui->bandWidthSpinBox->setEnabled( 0 );
    ui->connectButton->setEnabled( 0 );
    ui->comPortsComboBox->setEnabled( 0 );
    QFuture<QVector<float>> future = QtConcurrent::run( []()
                                                        { return QVector<float> { 1000000, 20000000 }; } );
    testReturnWatcher.setFuture( future );
}

void MainWindow::on_lockPushButton_clicked()
{
    // todo
}

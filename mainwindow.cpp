#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include <cmath>
#include <QTimer>
#include <QComboBox>
#include <QMessageBox>
#include <QSizePolicy>
#include <QtConcurrentRun>
#include <qcontainerfwd.h>
#include <vector>
#include "visa.hpp"
#include "visaResourceManager.hpp"

MainWindow::MainWindow( QWidget *parent ) :
    QMainWindow( parent ), ui( new Ui::MainWindow )
{
    ui->setupUi( this );
    connect( &reloadWatcher, &QFutureWatcher<QVector<QString>>::finished, this, &MainWindow::onReloadPushButtonFinished );
    connect( &testReturnWatcher, &QFutureWatcher<std::pair<std::vector<uint32_t>, std::vector<double>>>::finished, this, &MainWindow::onTestButtonReturn );

    plot = new QCustomPlot { this };
    ui->workingLayout->addWidget( plot );
    plot->setSizePolicy( QSizePolicy { QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Expanding } );
    plot->addGraph();
    plot->graph( 0 )->setPen( QPen( Qt::red, 2 ) );
    plot->graph( 0 )->setBrush( QBrush( QColor( 255, 0, 0, 30 ) ) );
    plot->graph( 0 )->setLineStyle( QCPGraph::lsNone );
    plot->graph( 0 )->setScatterStyle( QCPScatterStyle( QCPScatterStyle::ssCircle, 6 ) );
    plot->xAxis->setRange( -1, 1 );
    plot->yAxis->setRange( -1, 1 );
    plot->xAxis->setLabel( "frequency" );
    plot->yAxis->setLabel( "|S11|" );
    on_reloadPushButton_clicked();
}

MainWindow::~MainWindow()
{
    QThreadPool::globalInstance()->waitForDone();
    delete inst;
    inst = 0;
    delete plot;
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
        ui->comPortsComboBox->setEnabled( 1 );
    }
    else
    {
        auto st = VisaResourceManager::instance().openSession( ui->comPortsComboBox->currentText(), 5000 );
        if ( st == VI_NULL )
        {
            QMessageBox::critical( this, "Error", VisaResourceManager::instance().lastError() );
        }
        inst = new VisaInstrument( st );
        ui->connectButton->setText( "disconnect" );
        ui->nameLabel->setText( QString::fromStdString( inst->query( "*IDN?\n" ) ) );
        ui->testButton->setEnabled( 1 );
        ui->reloadPushButton->setEnabled( 0 );
        ui->comPortsComboBox->setEnabled( 0 );
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
    auto err { VisaResourceManager::instance().lastError() };
    if ( err.size() )
        QMessageBox::critical( this, "Error", err );
}

void MainWindow::onTestButtonReturn()
{
    auto ret { testReturnWatcher.future().result() };
    if ( ret.first.empty() )
    {
        QMessageBox::information( this, "Measure result", "Не подходит заданным условиям." );
    }
    else
    {
        QString bands;
        // for ( auto band : ret )
        // {
        //     bands += QString::number( band / ( std::pow( 10, ui->startUnitComboBox->currentIndex() * 3 ) ) ) + ' ' + ui->startUnitComboBox->currentText() + ", ";
        // }
        if ( bands.size() )
            bands.resize( bands.size() - 2 );
        QString ans { QString( "Подходящих каналов: %1, на частотах: \n%2" ).arg( ret.first.size() ).arg( bands ) };
        QMessageBox::information( this, "Measure result", ans );
    }

    plot->graph( 0 )->data()->clear();
    for ( size_t i { 0 }; i < ret.first.size(); ++i )
    {
        plot->graph( 0 )->addData( ret.first[ i ], ret.second[ i ] );
    }
    plot->graph( 0 )->rescaleAxes();
    plot->replot();
    ui->testButton->setEnabled( 1 );
    ui->startHzDoubleSpinBox->setEnabled( 1 );
    ui->startUnitComboBox->setEnabled( 1 );
    ui->endHzDoubleSpinBox->setEnabled( 1 );
    ui->endUnitComboBox->setEnabled( 1 );
    ui->pointsCountSpinBox->setEnabled( 1 );
    ui->bandWidthComboBox->setEnabled( 1 );
    ui->bandWidthDoubleSpinBox->setEnabled( 1 );
    ui->connectButton->setEnabled( 1 );
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
    ui->bandWidthDoubleSpinBox->setEnabled( 0 );
    ui->connectButton->setEnabled( 0 );
    QFuture<std::pair<std::vector<uint32_t>, std::vector<double>>> future = QtConcurrent::run( [ & ]()
                                                                                               {
                                                            inst->write("INIT1:CONT OFF");
                                                            inst->write("CALC1:PAR:DEL:ALL");
                                                            inst->write("CALC1:PAR:DEF 'Trc1', S11");
                                                            inst->write("CALC1:PAR:SEL 'Trc1'");
                                                            inst->write("CALC1:FORM MLOG");
                                                            inst->write("FORM:DATA REAL,64");
                                                            inst->write(QString("FREQ:STAR %1").arg(ui->startHzDoubleSpinBox->value()*std::pow(10,3*ui->startUnitComboBox->currentIndex()),0, 'f',2).toStdString());
                                                            inst->write(QString("FREQ:STOP %1").arg(ui->endHzDoubleSpinBox->value()*std::pow(10,3*ui->endUnitComboBox->currentIndex()),0, 'f',2).toStdString());
                                                            inst->write(QString("SWE:POIN %1").arg(ui->pointsCountSpinBox->value()).toStdString());
                                                            inst->write("SENS1:BAND 10000");
                                                            inst->write("INIT:IMM");
                                                            inst->query("*OPC?");
                                                            inst->write("CALC1:DATA:STIM?");
                                                            auto d{inst->rawDataRead()};
                                                            std::vector<uint32_t> x {d.begin(), d.end()}; 
                                                            inst->query("*OPC?");
                                                            inst->write("CALC1:DATA? FDAT");
                                                            d = inst->rawDataRead();
                                                            std::vector<double> y {d.begin(), d.end()}; 
                                                             return std::make_pair( x, y ); } );
    testReturnWatcher.setFuture( future );
}

void MainWindow::on_lockPushButton_clicked()
{
    // todo
}

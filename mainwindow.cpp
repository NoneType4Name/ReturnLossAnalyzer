#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include <cmath>
#include <QTimer>
#include <QComboBox>
#include <QMessageBox>
#include <QSizePolicy>
#include <QtConcurrentRun>
#include <qcontainerfwd.h>
#include <qnamespace.h>
#include <utility>
#include <vector>
#include "deps/qcustomplot-source/qcustomplot.h"
#include "visa.hpp"
#include "visaResourceManager.hpp"

MainWindow::MainWindow( QWidget *parent ) :
    QMainWindow( parent ), ui( new Ui::MainWindow )
{
    ui->setupUi( this );
    connect( &reloadWatcher, &QFutureWatcher<QVector<QString>>::finished, this, &MainWindow::onReloadPushButtonFinished );
    connect( &testReturnWatcher, &QFutureWatcher<std::pair<std::vector<ChannelBounds>, std::pair<std::vector<double>, std::vector<double>>>>::finished, this, &MainWindow::onTestButtonReturn );

    ui->plot->addGraph();
    ui->plot->addGraph();
    ui->plot->graph( 0 )->setPen( QPen( Qt::red, 2 ) );
    ui->plot->graph( 0 )->setBrush( QBrush( QColor( 255, 0, 0, 30 ) ) );
    ui->plot->graph( 0 )->setLineStyle( QCPGraph::lsLine );
    ui->plot->graph( 0 )->setScatterStyle( QCPScatterStyle( QCPScatterStyle::ssDot ) );
    ui->plot->graph( 1 )->setPen( QPen( Qt::blue, 2 ) );
    ui->plot->graph( 1 )->setLineStyle( QCPGraph::lsNone );
    ui->plot->graph( 1 )->setScatterStyle( QCPScatterStyle( QCPScatterStyle::ssCircle, 6 ) );
    ui->plot->xAxis->setRange( -1, 1 );
    ui->plot->yAxis->setRange( -1, 1 );
    ui->plot->xAxis->setLabel( "frequency, Hz" );
    ui->plot->yAxis->setLabel( "S11, dB" );
    ui->plot->setInteractions( QCP::iRangeDrag | QCP::iRangeZoom );

    ui->markersTableWidget->horizontalHeader()->setSectionResizeMode( QHeaderView::Stretch );
    ui->markersTableWidget->viewport()->setMouseTracking( 1 );

    on_reloadPushButton_clicked();
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
    channels = std::move( ret.first );
    freq     = std::move( ret.second.first );
    dbMag    = std::move( ret.second.second );

    if ( channels.empty() )
    {
        QMessageBox::information( this, "Measure result", "No channels with this configurations" );
    }
    else
    {
    }

    ui->plot->graph( 0 )->data()->clear();
    ui->plot->graph( 1 )->data()->clear();
    for ( size_t i { 0 }; i < freq.size(); ++i )
    {
        ui->plot->graph( 0 )->addData( freq[ i ], dbMag[ i ] );
    }
    for ( size_t i { 0 }; i < channels.size(); ++i )
    {
        ui->plot->graph( 1 )->addData( freq[ channels[ i ].peak_idx ], dbMag[ channels[ i ].peak_idx ] );
    }
    ui->plot->rescaleAxes();
    this->addBw();
    ui->plot->replot();
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

__asm__(
    ".section .rdata,\"dr\"\n"
    ".global _binary_freq_bin_start\n"
    ".global _binary_freq_bin_end\n"
    ".global _binary_mag_db_bin_start\n"
    ".global _binary_mag_db_bin_end\n"

    "_binary_freq_bin_start:\n"
    "   .incbin \"D:/main/coding/u/vna/dataset/freq.bin\"\n"
    "_binary_freq_bin_end:\n"

    "_binary_mag_db_bin_start:\n"
    "   .incbin \"D:/main/coding/u/vna/dataset/mag_db.bin\"\n"
    "_binary_mag_db_bin_end:\n" );

extern "C"
{
    extern const double _binary_freq_bin_start[];
    extern const double _binary_freq_bin_end[];
    extern const double _binary_mag_db_bin_start[];
    extern const double _binary_mag_db_bin_end[];
}

inline auto addEnding( double hz )
{
    return ( hz >= 1e9 ) ? QString( "%1 GHz" ).arg( hz / 1e9, 0, 'f', 2 ) : ( hz >= 1e6 ) ? QString( "%1 MHz" ).arg( hz / 1e6, 0, 'f', 2 )
                                                                        : ( hz >= 1e3 )   ? QString( "%1 kHz" ).arg( hz / 1e3, 0, 'f', 2 )
                                                                                          : QString( "%1 Hz" ).arg( hz, 0, 'f', 2 );
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
    QFuture<std::pair<std::vector<ChannelBounds>, std::pair<std::vector<double>, std::vector<double>>>> future = QtConcurrent::run( [ & ]()
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

                                                            // auto d{inst->rawDataRead()};
                                                            inst->rawRead();
                                                            // std::vector<uint32_t> x {d.begin(), d.end()}; 
                                                            std::vector<double> x {_binary_freq_bin_start, _binary_freq_bin_end}; 
                                                            inst->query("*OPC?");
                                                            inst->write("CALC1:DATA? FDAT");
                                                            
                                                            inst->rawRead();
                                                            // d = inst->rawDataRead();
                                                            // std::vector<double> y {d.begin(), d.end()}; 
                                                            std::vector<double> y {_binary_mag_db_bin_start, _binary_mag_db_bin_end};
                                                            auto freq {xt::xarray<double>::from_shape({x.size()})};
                                                            std::copy(x.begin(), x.end(), freq.begin());

                                                            auto dbMag {xt::xarray<double>::from_shape({y.size()})};
                                                            std::copy(y.begin(), y.end(), dbMag.begin());
                                                            
                                                            auto peaks{find_minimum_peaks(dbMag, ui->limitDoubleSpinBox->value())};
                                                            auto channels{find_channel_bounds(freq,dbMag,peaks, ui->tresholdDoubleSpinBox->value())};
                                                            channels = filter_channels_by_bandwidth(freq, channels, ui->bandWidthDoubleSpinBox->value() * std::pow(10,3*ui->bandWidthComboBox->currentIndex()));
                                       return std::make_pair( channels, std::make_pair(x, y) ); } );
    testReturnWatcher.setFuture( future );
}

void MainWindow::addBw()
{
    auto rowCount { ui->markersTableWidget->rowCount() };
    for ( auto i { 0 }; i < rowCount; ++i )
    {
        ui->markersTableWidget->removeRow( 0 );
    }

    for ( auto item : _plotItems )
    {
        ui->plot->removeItem( item );
    }
    _plotItems.clear();
    QPen dashPen;
    dashPen.setColor( Qt::gray );
    dashPen.setWidth( 1 );
    dashPen.setStyle( Qt::DashLine );
    _plotItems.reserve( channels.size() * 3 );
    size_t i { 0 };
    for ( auto bound : channels )
    {
        QCPItemStraightLine *vLine = static_cast<QCPItemStraightLine *>( _plotItems.emplace_back( new QCPItemStraightLine( ui->plot ) ) );
        vLine->setPen( dashPen );

        vLine->point1->setCoords( freq[ bound.min_idx ], 0.0 );
        vLine->point2->setCoords( freq[ bound.min_idx ], 1.0 );

        QCPItemStraightLine *vLine2 = static_cast<QCPItemStraightLine *>( _plotItems.emplace_back( new QCPItemStraightLine( ui->plot ) ) );
        vLine2->setPen( dashPen );
        vLine2->point1->setCoords( freq[ bound.max_idx ], 0.0 );
        vLine2->point2->setCoords( freq[ bound.max_idx ], 1.0 );

        QCPItemStraightLine *vLine3 = static_cast<QCPItemStraightLine *>( _plotItems.emplace_back( new QCPItemStraightLine( ui->plot ) ) );
        vLine3->setPen( dashPen );

        vLine3->point1->setCoords( 0, ui->limitDoubleSpinBox->value() );
        vLine3->point2->setCoords( 1, ui->limitDoubleSpinBox->value() );

        QColor shadingColor( 0, 255, 0, 30 );
        QCPItemRect *rect = static_cast<QCPItemRect *>( _plotItems.emplace_back( new QCPItemRect( ui->plot ) ) );
        rect->setPen( Qt::NoPen );
        rect->setBrush( QBrush( shadingColor ) );

        rect->topLeft->setTypeX( QCPItemPosition::ptPlotCoords );
        rect->topLeft->setTypeY( QCPItemPosition::ptAxisRectRatio );
        rect->topLeft->setAxes( ui->plot->xAxis, ui->plot->yAxis );

        rect->bottomRight->setTypeX( QCPItemPosition::ptPlotCoords );
        rect->bottomRight->setTypeY( QCPItemPosition::ptAxisRectRatio );
        rect->bottomRight->setAxes( ui->plot->xAxis, ui->plot->yAxis );

        rect->topLeft->setCoords( freq[ bound.min_idx ], 0 );
        rect->bottomRight->setCoords( freq[ bound.max_idx ], 1 );

        QCPItemText *marker = static_cast<QCPItemText *>( _plotItems.emplace_back( new QCPItemText( ui->plot ) ) );
        marker->setPositionAlignment( Qt::AlignBottom | Qt::AlignHCenter );
        marker->position->setType( QCPItemPosition::ptPlotCoords );
        marker->position->setCoords( freq[ bound.peak_idx ], dbMag[ bound.peak_idx ] );
        marker->setText( QString( "M%1" ).arg( ++i ) );
        marker->setFont( QFont( marker->font().family(), 14 ) );

        ui->markersTableWidget->insertRow( ui->markersTableWidget->rowCount() );
        auto item = new QTableWidgetItem( QString( "M%1" ).arg( i ) );
        ui->markersTableWidget->setItem( ui->markersTableWidget->rowCount() - 1, 0, item );

        item = new QTableWidgetItem( addEnding( freq[ bound.peak_idx ] ) );
        item->setToolTip( QString::number( freq[ bound.peak_idx ], 'f', 0 ) + " Hz" );
        ui->markersTableWidget->setItem( ui->markersTableWidget->rowCount() - 1, 1, item );

        item = new QTableWidgetItem( QString::number( dbMag[ bound.peak_idx ] ) );
        ui->markersTableWidget->setItem( ui->markersTableWidget->rowCount() - 1, 2, item );

        item = new QTableWidgetItem( addEnding( freq[ bound.min_idx ] ) );
        item->setToolTip( QString::number( freq[ bound.min_idx ], 'f', 0 ) + " Hz" );
        ui->markersTableWidget->setItem( ui->markersTableWidget->rowCount() - 1, 3, item );

        item = new QTableWidgetItem( addEnding( freq[ bound.max_idx ] ) );
        item->setToolTip( QString::number( freq[ bound.max_idx ], 'f', 0 ) + " Hz" );
        ui->markersTableWidget->setItem( ui->markersTableWidget->rowCount() - 1, 4, item );

        item = new QTableWidgetItem( addEnding( freq[ bound.peak_idx ] ) );
        item->setToolTip( QString::number( freq[ bound.peak_idx ], 'f', 0 ) + " Hz" );
        ui->markersTableWidget->setItem( ui->markersTableWidget->rowCount() - 1, 5, item );

        if ( freq[ bound.max_idx ] == freq[ bound.min_idx ] )
        {
            ui->markersTableWidget->setItem( ui->markersTableWidget->rowCount() - 1, 6, new QTableWidgetItem( QString::number( dbMag[ bound.min_idx ] ) ) );
        }
        else
        {
            double y1 = dbMag[ bound.min_idx ];
            double y2 = dbMag[ bound.max_idx ];
            double x1 = freq[ bound.min_idx ];
            double x2 = freq[ bound.max_idx ];
            ui->markersTableWidget->setItem( ui->markersTableWidget->rowCount() - 1, 6, new QTableWidgetItem( QString::number( y1 + ( y2 - y1 ) * ( x2 - x1 ) / ( x2 - x1 ) ) ) );
        }

        item = new QTableWidgetItem( addEnding( ( freq[ bound.max_idx ] - freq[ bound.min_idx ] ) ) );
        item->setToolTip( QString::number( ( freq[ bound.max_idx ] - freq[ bound.min_idx ] ), 'f', 0 ) + " Hz" );
        ui->markersTableWidget->setItem( ui->markersTableWidget->rowCount() - 1, 7, item );
    }
}

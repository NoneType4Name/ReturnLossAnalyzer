#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "visa.hpp"
#include <QMainWindow>
#include <QFutureWatcher>
#include <vector>
#include <qcustomplot.h>
#include "BandwidthSearch.hxx"

QT_BEGIN_NAMESPACE
namespace Ui
{
    class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

  public:
    explicit MainWindow( QWidget *parent = nullptr );
    ~MainWindow() override;

  private slots:
    void on_connectButton_clicked();

    void on_reloadPushButton_clicked();
    void onReloadPushButtonFinished();

    void on_testButton_clicked();
    void onTestButtonReturn();

    void on_lockPushButton_clicked();

  private:
    void addBw();
    Ui::MainWindow *ui;
    QFutureWatcher<QVector<QString>> reloadWatcher;
    QFutureWatcher<std::pair<std::vector<ChannelBounds>, std::pair<std::vector<double>, std::vector<double>>>> testReturnWatcher;
    VisaInstrument *inst { 0 };
    std::vector<QCPAbstractItem *> _plotItems;
    std::vector<ChannelBounds> channels;
    std::vector<double> freq;
    std::vector<double> dbMag;
};
#endif // MAINWINDOW_H

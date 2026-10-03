#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "Visa.hpp"
#include <QMainWindow>
#include <QFutureWatcher>
#include <memory>

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
    void on_reloadPushButton_finished();

    void on_testButton_clicked();

    void on_lockPushButton_clicked();

  private:
    Ui::MainWindow *ui;
    QFutureWatcher<QVector<QString>> reloadWatcher;
    VisaInstrument *inst;
};
#endif // MAINWINDOW_H

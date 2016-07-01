
#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QtCore/QtGlobal>
#include <QMainWindow>
#include <QtSerialPort/QSerialPort>
#include <QCloseEvent>
#include "fbcupg.h"
#include "assistantobject.h"
#include "helpdialog.h"

QT_BEGIN_NAMESPACE

class QLabel;
class QProgressBar;
class PassworDialog;

namespace Ui {
class MainWindow;
}

QT_END_NAMESPACE

class Console;
class SettingsDialog;
class AdvanceDialog;
class UpgradeSetting;
class upgradeInfoDialog;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();
private slots:
    void openSerialPort();
    void closeSerialPort();
    void about();
    void writeData(const QByteArray &data);
    void saveLog();
    void prepareUpgrade();
    void handleWorkerMessage(int message, QString data);
    void getUpgradeInfo(QList<FbcUpgHandler::sectionInfo> list);
protected:
    void closeEvent(QCloseEvent *event);
private:
    void initActionsConnections();
    void updateActionStatus();
    bool loadUpgradefile();
private:
    Ui::MainWindow *ui;
    Console *console;
    SettingsDialog *settingsDialog;
    PassworDialog *passwdDialog;
    UpgradeSetting *upgradeSetting;
    FbcUpgHandler *fbcUpgHandler;
    HelpDialog *helpDialog;
    QThread upgradeThread;
    QThread assistantThread;
    QThread sendFactoryStopTherad;
    QProgressBar *process;
    assistantObject *assistant;
    QTimer *sendFactoryStopTimer;
signals:
    void startUpgrade();
    void dispatchMessageToWorkerUpg(int message, QString upgradeInfo);
    void dispatchMessageToWorkerAssistant(int message, QString upgradeInfo);
};

#endif // MAINWINDOW_H

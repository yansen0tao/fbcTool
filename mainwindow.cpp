
#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "console.h"
#include "settingsdialog.h"
#include "passwordialog.h"
#include "upgradesetting.h"
#include "upgradeinfo.h"

#include <QMessageBox>
#include <QLabel>
#include <QtSerialPort/QSerialPort>
#include <QFile>
#include <QFileDialog>
#include <QLineEdit>
#include <QProgressBar>
#include <QMetaType>
#include <QTimer>
#include <QDebug>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    qDebug() << "MainWindow" << QThread::currentThread();

    ui->setupUi(this);

    ui->actionConnect->setEnabled(true);
    ui->actionDisconnect->setEnabled(false);
    ui->actionConfigure->setEnabled(true);
    ui->actionUpgrade->setEnabled(false);
    //TODO second construction
    settingsDialog = new SettingsDialog(this);
    passwdDialog = new PassworDialog(this);
    upgradeSetting = new UpgradeSetting(this);
    helpDialog = new HelpDialog(this);

    console = new Console;
    console->setEnabled(false);
    setCentralWidget(console);

    process = new QProgressBar(this);
    if (process != NULL)
    {
        process->setFixedWidth(ui->statusBar->width());
        process->setMaximum(100);
        process->setMinimum(0);

        ui->statusBar->addWidget(process);
    }

    initActionsConnections();
    connect(console, &Console::getData, this, &MainWindow::writeData);
    fbcUpgHandler = new FbcUpgHandler();

    if (fbcUpgHandler!=NULL)
    {
        fbcUpgHandler->moveToThread(&upgradeThread);
        connect(this, &MainWindow::startUpgrade, fbcUpgHandler, &FbcUpgHandler::doUpgrade);
        connect(&upgradeThread, &QThread::finished, fbcUpgHandler, &QObject::deleteLater);
        upgradeThread.start();

        connect(this, &MainWindow::dispatchMessageToWorkerUpg, fbcUpgHandler, &FbcUpgHandler::handleUiMessage);
        connect(fbcUpgHandler, &FbcUpgHandler::dispatchMessageToUi, this, &MainWindow::handleWorkerMessage);

        qRegisterMetaType<QList<FbcUpgHandler::sectionInfo>>("QList<sectionInfo>");
        connect(fbcUpgHandler, &FbcUpgHandler::sendUpgradeInfoToUI, this, &MainWindow::getUpgradeInfo);
    }

    if (fbcUpgHandler!=NULL)
    {
        assistant = new assistantObject();
        if (assistant)
        {
            assistant->moveToThread(&assistantThread);
            connect(this, &MainWindow::dispatchMessageToWorkerAssistant,
                    assistant, &assistantObject::handleUiMessage);
            connect(assistant, &assistantObject::dispatchMessageToUi,
                    this, &MainWindow::handleWorkerMessage);
            connect(&assistantThread, &QThread::finished,
                    assistant, &QObject::deleteLater);

            assistantThread.start();
        }
    }

    if (fbcUpgHandler!=NULL)
    {
        sendFactoryStopTimer = new QTimer();
        sendFactoryStopTimer->setInterval(5);
        sendFactoryStopTimer->moveToThread(&sendFactoryStopTherad);
        connect(sendFactoryStopTimer, SIGNAL(timeout()), fbcUpgHandler, SLOT(prepareUpgrade()));
        connect(&sendFactoryStopTherad, SIGNAL(started()), sendFactoryStopTimer,SLOT(start()));
    }

    updateActionStatus();
}

MainWindow::~MainWindow()
{
    if (settingsDialog)
    {
        delete settingsDialog;
        settingsDialog = NULL;
    }

    if (console)
    {
        delete console;
        console = NULL;
    }

    if (passwdDialog)
    {
        delete passwdDialog;
        passwdDialog = NULL;
    }

    if (upgradeSetting)
    {
        delete upgradeSetting;
        upgradeSetting = NULL;
    }

//    if (assistant)
//    {
//        delete assistant;
//        assistant = NULL;
//    }

    if (ui)
    {
        delete ui;
        ui = NULL;
    }

//    if (upgradeThread.isRunning())
//        upgradeThread.quit();

    //TODO
//    if (fbcUpgHandler)
//    {
//        delete fbcUpgHandler;
//        fbcUpgHandler = NULL;
//    }
}

void MainWindow::getUpgradeInfo(QList<FbcUpgHandler::sectionInfo> list)
{
    if (!list.isEmpty())
    {
        upgradeInfoDialog upgradeInfo;
        qRegisterMetaType<QList<FbcUpgHandler::sectionInfo>>("QList<FbcUpgHandler::sectionInfo>");
        connect(&upgradeInfo, &upgradeInfoDialog::sendUpgradeInfoToUPG, fbcUpgHandler, &FbcUpgHandler::getUpgradeInfoFromUI);
        upgradeInfo.update(list);
        upgradeInfo.exec();
    }
}

void MainWindow::handleWorkerMessage(int message, QString data)
{
    int device = message&0xf;
    int instruction = message&0xf0;
    int state = message&0xf00;

    if (FbcUpgHandler::Console == device)
    {
        if (FbcUpgHandler::Upgrade == instruction)
        {
            QString settings = "\n本次升级配置如下\n";

            if (!FbcUpgHandler::isFactoryModeActive)
                settings += tr("\n升级模式%1\n").arg(upgradeSetting->getUpgradeModeString());
            settings += tr("\n等待Fbc升级时间%1秒\n").arg(UpgradeSetting::waitRebootSecs);
            settings += tr("\n读取数据Fbc响应时间%1毫秒\n").arg(UpgradeSetting::waitReponseMsecs);
            console->putData(settings);
        }

        console->putData(data);

        if (FbcUpgHandler::WaitPrepare == instruction)
        {
            if (FbcUpgHandler::Success == state)
            {
                FbcUpgHandler::isUpgradePrepared = false;
                sendFactoryStopTherad.exit();

                QString filename = QFileDialog::getOpenFileName(this, tr("打开升级文件"),
                                                                0, tr("升级文件类型(*.bin)"),
                                                                0, QFileDialog::DontUseNativeDialog);
                if(!filename.isEmpty())
                    emit dispatchMessageToWorkerUpg(FbcUpgHandler::File|FbcUpgHandler::Open, filename);
            }
        }
        else if (FbcUpgHandler::Upgrade == instruction)
        {
            process->setValue(0);
            emit startUpgrade();
        }
    }
    else if (FbcUpgHandler::ProgressBar == device)
    {
        qDebug() << "value:" << data.toInt();
        if (process!=NULL && !data.isEmpty())
            process->setValue(data.toInt());
    }
    else if (FbcUpgHandler::PopUp == device)
    {
        if (FbcUpgHandler::Success == state)
            QMessageBox::information(this, tr(" "), data, QMessageBox::Ok);
        else if (FbcUpgHandler::Failure == state)
            QMessageBox::warning(this, tr(" "), data, QMessageBox::Ok);
        else
            Q_ASSERT(false);
    }
    else if (FbcUpgHandler::Invalid == device)
    {
        if (FbcUpgHandler::Exit == instruction)
        {
            if (FbcUpgHandler::Success == instruction)
            {
                upgradeThread.exit();
                close();
            }
        }
    }

    if (FbcUpgHandler::Console != device)
        updateActionStatus();
}

void MainWindow::openSerialPort()
{
    fbcUpgHandler->configureSerialPort(settingsDialog->settings());

    emit dispatchMessageToWorkerUpg(FbcUpgHandler::Serial|FbcUpgHandler::Open, "");
}

void MainWindow::closeSerialPort()
{
    dispatchMessageToWorkerUpg(FbcUpgHandler::Serial|FbcUpgHandler::Close, "");
}

void MainWindow::about()
{
    helpDialog->show();
}

void MainWindow::writeData(const QByteArray &data)
{
    emit dispatchMessageToWorkerUpg(FbcUpgHandler::Console|FbcUpgHandler::Write, data);
}

void MainWindow::initActionsConnections()
{
    connect(ui->actionConnect, &QAction::triggered, this, &MainWindow::openSerialPort);
    connect(ui->actionDisconnect, &QAction::triggered, this, &MainWindow::closeSerialPort);
    connect(ui->actionConfigure, &QAction::triggered, settingsDialog, &MainWindow::show);
    connect(ui->actionClear, &QAction::triggered, console, &Console::clear);
    connect(ui->actionAbout, &QAction::triggered, this, &MainWindow::about);
    connect(ui->actionUpgrade, &QAction::triggered, this, &MainWindow::prepareUpgrade);
    connect(ui->actionLock, &QAction::triggered, passwdDialog, &MainWindow::show);
    connect(ui->actionUpgradeStting, &QAction::triggered, upgradeSetting, &MainWindow::show);
    connect(ui->actionSave, &QAction::triggered, this, &MainWindow::saveLog);
}

void MainWindow::saveLog()
{
    QString content = console->toPlainText();

    if (!content.isEmpty())
        emit dispatchMessageToWorkerAssistant(FbcUpgHandler::Console|FbcUpgHandler::Save, content);
    else
        QMessageBox::warning(this, tr("保存日志文件"), tr("亲好像没有数据可以保存哦！"));
}

void MainWindow::prepareUpgrade()
{
    //Avoid upgrade is running now
    if (fbcUpgHandler->isUpgradeRunning)
        return;

    FbcUpgHandler::isUpgradePrepared = true;
    sendFactoryStopTherad.start();

    console->putData("\n升级程序正在启动中，请耐心等待，如果看到这条消息很久，升级都没有启动的话，请断掉电源，"
                     "等待几秒钟，然后插上电源！\n");
}

void MainWindow::updateActionStatus()
{
    bool isConnected = fbcUpgHandler->isSerialPortConnected;
    bool isRunning = fbcUpgHandler->isUpgradeRunning;

    console->setEnabled(isConnected);

    ui->actionConnect->setEnabled(!isConnected);
    ui->actionConfigure->setEnabled(!isConnected);
    ui->actionDisconnect->setEnabled(isConnected && !isRunning);

    ui->actionUpgrade->setEnabled(isConnected && !isRunning);
    ui->actionUpgradeStting->setEnabled(isConnected && !isRunning);

    ui->actionLock->setEnabled(isConnected && !isRunning);
    ui->actionClear->setEnabled(isConnected && !isRunning);

    //TODO
    SettingsDialog::Settings rSetting = settingsDialog->settings();
    console->setLocalEchoEnabled(rSetting.localEchoEnabled);
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    if (fbcUpgHandler && fbcUpgHandler->isSerialPortConnected)
    {
        if (fbcUpgHandler->isUpgradeRunning)
        {
            QMessageBox msgBox;
            msgBox.setIcon(QMessageBox::Warning);
            msgBox.setText(tr("升级进程正在运行"));
            msgBox.setInformativeText(tr("请耐心等待升级结束。"));
            msgBox.setStandardButtons(QMessageBox::Yes);
            msgBox.setDefaultButton(QMessageBox::Yes);
            int ret = msgBox.exec();

            switch (ret)
            {
                case QMessageBox::Yes:
                    event->ignore();
                break;
            }
        }
    }
}

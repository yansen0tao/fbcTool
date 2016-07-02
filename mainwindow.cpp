
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

#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "console.h"
#include "settingsdialog.h"
#include "passwordialog.h"
#include "upgradesetting.h"
#include "upgradeinfo.h"

MainWindow* MainWindow::NewInstance()
{
    MainWindow* ret = new MainWindow();

    if (!ret || !ret->construct())
    {
        delete ret;
        ret = NULL;
    }

    return ret;
}

bool MainWindow::initUi()
{
    bool ret = true;

    ui = new Ui::MainWindow();

    if (ui)
    {
        ui->setupUi(this);
        updateActionStatus();
    }
    else
    {
        ret = false;
    }

    if (ret)
    {
        settingsDialog = new SettingsDialog(this);

        if (!settingsDialog)
            ret = false;
    }

    if (ret)
    {
        passwdDialog = new PassworDialog(this);

        if (!passwdDialog)
            ret = false;
    }

    if (ret)
    {
        upgradeSetting = new UpgradeSetting(this);

        if (!upgradeSetting)
            ret = false;
    }

    if (ret)
    {
        helpDialog = new HelpDialog(this);

        if (!helpDialog)
            ret = false;
    }

    if (ret)
    {
        console = new Console;
        if (console)
        {
            console->setEnabled(true);
            setCentralWidget(console);

            connect(console, &Console::getData, this, &MainWindow::writeData);
        }
        else
        {
            ret = false;
        }
    }

    if (ret)
    {
        process = new QProgressBar(this);
        if (process)
        {
            process->setFixedWidth(ui->statusBar->width());
            process->setMaximum(100);
            process->setMinimum(0);

            ui->statusBar->addWidget(process);
        }
        else
        {
            ret = false;
        }
    }

    if (ret)
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
        else
        {
            ret = false;
        }
    }

    if (ret)
        initActionsConnections();

    return ret;
}

bool MainWindow::construct()
{
    bool ret = initUi();

    if (ret)
    {
        fbcUpgHandler = FbcUpgHandler::NewInstance();
        if (fbcUpgHandler)
        {
            qRegisterMetaType<QList<FbcUpgHandler::sectionInfo>>("QList<sectionInfo>");
            connect(fbcUpgHandler, &FbcUpgHandler::sendUpgradeInfoToUI, this, &MainWindow::getUpgradeInfo);
            connect(this, &MainWindow::startUpgrade, fbcUpgHandler, &FbcUpgHandler::doUpgrade);
            connect(this, &MainWindow::dispatchMessageToWorkerUpg, fbcUpgHandler, &FbcUpgHandler::handleUiMessage);
            connect(fbcUpgHandler, &FbcUpgHandler::dispatchMessageToUi, this, &MainWindow::handleWorkerMessage);

            fbcUpgHandler->moveToThread(&upgradeThread);
            connect(&upgradeThread, &QThread::finished, fbcUpgHandler, &QObject::deleteLater);
            upgradeThread.start();
        }
        else
        {
            ret = false;
        }
    }

    if (ret)
    {
        sendFactoryStopTimer = new QTimer();
        if (sendFactoryStopTimer)
        {
            sendFactoryStopTimer->setInterval(5);
            sendFactoryStopTimer->moveToThread(&sendFactoryStopTherad);
            connect(sendFactoryStopTimer, SIGNAL(timeout()), fbcUpgHandler, SLOT(prepareUpgrade()));
            connect(&sendFactoryStopTherad, SIGNAL(finished()), sendFactoryStopTimer,SLOT(stop()));
            connect(&sendFactoryStopTherad, SIGNAL(started()), sendFactoryStopTimer,SLOT(start()));
        }
        else
        {
            ret = false;
        }
    }

    return ret;
}

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent)
{
    qDebug() << "MainWindow" << QThread::currentThread();
}

MainWindow::~MainWindow()
{
    if (ui) delete ui;

    if (settingsDialog) delete settingsDialog;

    if (console) delete console;

    if (passwdDialog) delete passwdDialog;

    if (upgradeSetting) delete upgradeSetting;

//    if (assistant)
//    {
//        delete assistant;
//        assistant = NULL;
//    }

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
            QString settings = "\n本次升级配置如下：\n";

            if (fbcUpgHandler->getFactoryMode())
            {
                settings += tr("\n注意:当前升级模式为工厂模式，将不使用您在升级配置中的普通升级方式，"
                               "但保留您在升级配置中选择的时间配置\n");
            }

            settings += tr("\n您在升级配置中的参数如下\n");
            settings += tr("\n升级模式%1\n").arg(upgradeSetting->getUpgradeModeString());
            settings += tr("\n等待Fbc升级时间%1秒\n").arg(UpgradeSetting::waitRebootSecs);
            settings += tr("\n读取数据Fbc响应时间%1毫秒\n").arg(UpgradeSetting::waitReponseMsecs);

            console->putData(settings);

            process->setValue(0);
            emit startUpgrade();
        }
        else if (FbcUpgHandler::Prepare == instruction)
        {
            if (FbcUpgHandler::Success == state)
            {
                updatePrepareThread(false);
                QString filename = QFileDialog::getOpenFileName(this, tr("打开升级文件"),
                                                                0, tr("升级文件类型(*.bin)"),
                                                                0, QFileDialog::DontUseNativeDialog);
                if(!filename.isEmpty())
                    emit dispatchMessageToWorkerUpg(FbcUpgHandler::File|FbcUpgHandler::Open, filename);
            }
            else
            {
                updatePrepareThread(false);
                console->putData(tr("\n检测到Fbc状态错误，请断电重启或者在控制台输入reboot重启，稍等片刻后再重新升级\n"));
                console->putData(tr("\n注意：发生该错误很可能是升级配置中的时间设置太短导致的，请重启后检查。\n"));
            }
        }

        console->putData(data);
    }
    else if (FbcUpgHandler::ProgressBar == device)
    {
        if (!data.isEmpty())
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

    if (FbcUpgHandler::Console != device)
        updateActionStatus(fbcUpgHandler->getConnected(), fbcUpgHandler->getRunning());
}

void MainWindow::openSerialPort()
{
    fbcUpgHandler->configureSerialPort(settingsDialog->settings());
    emit dispatchMessageToWorkerUpg(FbcUpgHandler::Serial|FbcUpgHandler::Open, 0);
}

void MainWindow::closeSerialPort()
{
    dispatchMessageToWorkerUpg(FbcUpgHandler::Serial|FbcUpgHandler::Close, 0);
}

void MainWindow::about()
{
    helpDialog->show();
}

void MainWindow::writeData(const QByteArray &data)
{
    emit dispatchMessageToWorkerUpg(FbcUpgHandler::Serial|FbcUpgHandler::Write, data);
}

void MainWindow::initActionsConnections()
{
    connect(ui->actionConnect, &QAction::triggered, this, &MainWindow::openSerialPort);
    connect(ui->actionDisconnect, &QAction::triggered, this, &MainWindow::closeSerialPort);
    connect(ui->actionConfigure, &QAction::triggered, settingsDialog, &MainWindow::show);
    connect(ui->actionClear, &QAction::triggered, this, &MainWindow::clear);
    connect(ui->actionAbout, &QAction::triggered, this, &MainWindow::about);
    connect(ui->actionUpgrade, &QAction::triggered, this, &MainWindow::startPrepare);
    connect(ui->actionLock, &QAction::triggered, passwdDialog, &MainWindow::show);
    connect(ui->actionUpgradeStting, &QAction::triggered, upgradeSetting, &MainWindow::show);
    connect(ui->actionSave, &QAction::triggered, this, &MainWindow::saveLog);
}

void MainWindow::clear()
{
    emit dispatchMessageToWorkerUpg(FbcUpgHandler::Serial|FbcUpgHandler::Clean, 0);
    console->clear();
}

void MainWindow::saveLog()
{
    QString content = console->toPlainText();

    if (!content.isEmpty())
        emit dispatchMessageToWorkerAssistant(FbcUpgHandler::Console|FbcUpgHandler::Save, content);
    else
        QMessageBox::warning(this, tr("保存日志文件"), tr("亲好像没有数据可以保存哦！"));
}

void MainWindow::updatePrepareThread(bool active)
{
    if (active)
        sendFactoryStopTherad.start();
    else
        sendFactoryStopTherad.exit();

    fbcUpgHandler->setDectedSignal(active);
}

void MainWindow::startPrepare()
{
    updatePrepareThread(true);

    console->putData("\n升级程序正在启动中，请耐心等待，如果看到这条消息很久，升级都没有启动的话，请断掉电源，"
                     "等待几秒钟，然后插上电源！\n");
}

void MainWindow::updateActionStatus(bool isConnected, bool isRunning)
{
    ui->actionConnect->setEnabled(!isConnected);
    ui->actionConfigure->setEnabled(!isConnected);
    ui->actionDisconnect->setEnabled(isConnected && !isRunning);
    ui->actionUpgrade->setEnabled(isConnected && !isRunning);
    ui->actionUpgradeStting->setEnabled(isConnected && !isRunning);
    ui->actionLock->setEnabled(isConnected && !isRunning);
    ui->actionClear->setEnabled(isConnected && !isRunning);
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    QMessageBox msgBox;
    int ret = 0;

    if (fbcUpgHandler->getConnected())
    {
        if (fbcUpgHandler->getRunning())
        {
            msgBox.setIcon(QMessageBox::Warning);
            msgBox.setText(tr("升级进程正在运行中"));
            msgBox.setInformativeText(tr("请耐心等待升级结束,谢谢。"));
            msgBox.setStandardButtons(QMessageBox::Yes);
            msgBox.setDefaultButton(QMessageBox::Yes);
            ret = msgBox.exec();

            switch (ret)
            {
                case QMessageBox::Yes:
                    event->ignore();
                break;
            }
        }
    }
}

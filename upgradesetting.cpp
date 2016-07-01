
#include <QDebug>
#include <QStandardItemModel>
#include "upgradesetting.h"
#include "ui_upgradesetting.h"
#include "fbcupg.h"
#include "passwordialog.h"

FbcUpgHandler::UpgradegMode UpgradeSetting::upgradegMode = FbcUpgHandler::NormalLiteMode;
int UpgradeSetting::waitRebootSecs = 2;
int UpgradeSetting::waitReponseMsecs = 1200;

UpgradeSetting::UpgradeSetting(QWidget *parent) :
    QDialog(parent, Qt::Dialog|Qt::WindowCloseButtonHint),
    ui(new Ui::UpgradeSetting)
{
    ui->setupUi(this);

    ui->upgradeMode->addItem("精简版本            普通模式");
    ui->upgradeMode->addItem("高级版本            普通模式");

    ui->lineEditDelayTime->setText(tr("%1").arg(waitRebootSecs));
    ui->lineEditAckDelay->setText(tr("%1").arg(waitReponseMsecs));
    ui->upgradeMode->setCurrentIndex(FbcUpgHandler::NormalLiteMode);
}

UpgradeSetting::~UpgradeSetting()
{
    delete ui;
}

QString UpgradeSetting::getUpgradeModeString()
{
    return ui->upgradeMode->currentText();
}

FbcUpgHandler::UpgradegMode UpgradeSetting::getCurrentUpgradeMode()
{
    return upgradegMode;
}

void UpgradeSetting::closeEvent(QCloseEvent *event)
{


    ui->lineEditDelayTime->setText(tr("%1").arg(waitRebootSecs));
    ui->lineEditAckDelay->setText(tr("%1").arg(waitReponseMsecs));
    ui->upgradeMode->setCurrentIndex(upgradegMode);

    qDebug() << "closeEvent::upgradeMode:" << upgradegMode;
}

void UpgradeSetting::on_OkButton_clicked()
{
    switch (ui->upgradeMode->currentIndex())
    {
    case 0:
        upgradegMode = FbcUpgHandler::NormalLiteMode;
        break;
    case 1:
        upgradegMode = FbcUpgHandler::NormalFullMode;
        break;
    default:
        upgradegMode = FbcUpgHandler::NormalLiteMode;
        break;
    }

    waitRebootSecs = ui->lineEditDelayTime->text().toInt();
    waitReponseMsecs = ui->lineEditAckDelay->text().toInt();

    hide();

    qDebug() << "on_OkButton_clicked::upgradeMode:" << upgradegMode;
}

void UpgradeSetting::on_upgradeMode_activated(int index)
{
    switch (index)
    {
    case FbcUpgHandler::NormalLiteMode:
        ui->lineEditDelayTime->setText("2");
        break;
    case FbcUpgHandler::NormalFullMode:
        ui->lineEditDelayTime->setText("20");
        break;
    }

    ui->lineEditDelayTime->repaint();
}


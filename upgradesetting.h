#ifndef UPGRADESETTING_H
#define UPGRADESETTING_H

#include <QDialog>
#include "fbcupg.h"

class QRadioButton;

namespace Ui {
class UpgradeSetting;
}

class UpgradeSetting : public QDialog
{
    Q_OBJECT

public:
    explicit UpgradeSetting(QWidget *parent = 0);
    ~UpgradeSetting();
    static int waitRebootSecs;
    static int waitReponseMsecs;
    QString getUpgradeModeString();
    static FbcUpgHandler::UpgradegMode getCurrentUpgradeMode();
private:
    Ui::UpgradeSetting *ui;
    static FbcUpgHandler::UpgradegMode upgradegMode;
protected:
    void closeEvent(QCloseEvent *event);
private slots:
    void on_OkButton_clicked();
    void on_upgradeMode_activated(int index);
};

#endif // UPGRADESETTING_H

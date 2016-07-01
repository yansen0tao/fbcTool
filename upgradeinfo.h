#ifndef UPGRADEINFO_H
#define UPGRADEINFO_H

#include <QDialog>
#include <QList>
#include "fbcupg.h"

class QCheckBox;
class QVBoxLayout;
class QLabel;
class QPushButton;

namespace Ui {
 class upgradeInfoDialog;
}

class upgradeInfoDialog : public QDialog
{
    Q_OBJECT
private:
    QList<QCheckBox*> mCheckBoxs;
    QVBoxLayout *globalLayout;
    QLabel *title;
    QPushButton *apply;
    QList<FbcUpgHandler::sectionInfo> sectionInfoList;
public:
    explicit upgradeInfoDialog(QWidget *parent = 0);
    ~upgradeInfoDialog();
    void update(QList<FbcUpgHandler::sectionInfo> list);\
    FbcUpgHandler *fbcUpgHandler;
signals:
    void sendUpgradeInfoToUPG(QList<FbcUpgHandler::sectionInfo> list);
public slots:
    void onApplyClicked(bool checked);
};

#endif // UPGRADEINFO_H

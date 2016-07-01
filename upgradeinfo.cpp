
#include <QLabel>
#include <QDialog>
#include <QWidget>
#include <QCheckBox>
#include <QVBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QDebug>
#include <QMessageBox>
#include "upgradeinfo.h"

upgradeInfoDialog::upgradeInfoDialog(QWidget *parent):QDialog(parent)
{
    setWindowTitle(tr("升级信息"));
}

upgradeInfoDialog::~upgradeInfoDialog()
{
    for (int i = 0; i < mCheckBoxs.length(); i++)
    {
        delete mCheckBoxs.at(i);
    }

    delete globalLayout;
    delete title;

    qDebug() << "Destory";
}

void upgradeInfoDialog::update(QList<FbcUpgHandler::sectionInfo> list)
{   
    QFont titleFont;
    QPalette titlePalette;
    QCheckBox *temp = NULL;

    sectionInfoList = list;

    globalLayout = new QVBoxLayout(this);
    title = new QLabel();

    titleFont.setPointSize(12);
    titlePalette.setColor(QPalette::WindowText, Qt::red);
    title->setFont(titleFont);
    title->setPalette(titlePalette);
    title->setText(tr("请选择升级分区"));

    globalLayout->addWidget(title);

    for (int i = 0; i < sectionInfoList.length(); i++)
    {
        int start = static_cast<FbcUpgHandler::sectionInfo>(sectionInfoList.at(i)).start;
        int length = static_cast<FbcUpgHandler::sectionInfo>(sectionInfoList.at(i)).length;

        temp = new QCheckBox();
        temp->setText(tr("分区:%1 起始地址:0x%2 大小:0x%3")
                      .arg(i+1).arg(start, -8, 16).arg(length, -8, 16));
        temp->setChecked(true);
        globalLayout->addWidget(temp);
        mCheckBoxs.append(temp);
    }

    apply = new QPushButton();
    apply->setText("确定");
    globalLayout->addWidget(apply);

    connect(apply, &QPushButton::clicked, this, onApplyClicked);

    setFixedSize(sizeHint());
}

void upgradeInfoDialog::onApplyClicked(bool checked)
{
    int checkedNum = 0;

    for (int i = 0; i < sectionInfoList.length(); i++)
    {
        if (!mCheckBoxs.at(i)->isChecked())
        {
            qDebug() << tr("i=%1").arg(mCheckBoxs.at(i)->isChecked());
            sectionInfoList.replace(i, 0);
        }
        else
        {
            checkedNum++;
        }
    }

    if(0 == checkedNum)
    {
        QMessageBox::warning(this, tr(""), tr("至少选择一项！"));
    }
    else
    {
        sendUpgradeInfoToUPG(sectionInfoList);
        close();
    }
}

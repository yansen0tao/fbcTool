
#include <QMessageBox>
#include "ui_passwordialog.h"

#include "passwordialog.h"
#include "upgradesetting.h"

bool PassworDialog::isPasswdCorrect = false;

PassworDialog::PassworDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::PassworDialog)
{
    ui->setupUi(this);
}

PassworDialog::~PassworDialog()
{
    delete ui;
}

void PassworDialog::on_OkButton_clicked()
{
    if (ui->lineEditPasswd->getPassword() == "8888")
    {
        isPasswdCorrect = true;
        hide();
    }
    else
    {
        isPasswdCorrect = false;
        QMessageBox::warning(this, tr(""), tr("密码错误"), QMessageBox::Ok);
    }
}

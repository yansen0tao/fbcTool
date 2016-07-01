#ifndef PASSWORDIALOG_H
#define PASSWORDIALOG_H

#include <QDialog>
#include "fbcupg.h"

namespace Ui {
class PassworDialog;
}

class PassworDialog : public QDialog
{
    Q_OBJECT
public:
    explicit PassworDialog(QWidget *parent = 0);
    ~PassworDialog();
    static bool isPasswdCorrect;
private slots:
    void on_OkButton_clicked();
private:
    Ui::PassworDialog *ui;
};

#endif // PASSWORDIALOG_H

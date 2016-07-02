#ifndef QPASSWORDLINEEDIT_H
#define QPASSWORDLINEEDIT_H

#include <QLineEdit>
class QPasswordLineEdit : public QLineEdit
{
    Q_OBJECT
public:
    QPasswordLineEdit(QWidget *parent,int timeout = 300);
    ~QPasswordLineEdit();

    QString getPassword();
    void setTimeout(int msec);
private slots:
    void slotCursorPositionChanged(int,int);
    void slotTextEdited(const QString&);
    void slotDisplayMaskPassword();
private:
    QString  getMaskPassword();

private:
    int	mTimeout;
    QString	mLineEditText;
    int	mLastCharCount;
};

#endif // QPASSWORDLINEEDIT_H

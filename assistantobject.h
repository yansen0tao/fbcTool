#ifndef ASSISTANTOBJECT_H
#define ASSISTANTOBJECT_H

#include <QObject>

class assistantObject : public QObject
{
    Q_OBJECT
public:
    assistantObject();
    ~assistantObject();
signals:
    void dispatchMessageToUi(int message, QString data);
public slots:
    void handleUiMessage(int message, QString data);
};

#endif // ASSISTANTOBJECT_H

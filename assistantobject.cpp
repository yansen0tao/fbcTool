
#include <QDateTime>
#include <QFile>
#include <QString>
#include <QDir>
#include <QDebug>
#include "assistantobject.h"
#include "fbcupg.h"

assistantObject::assistantObject()
{

}

assistantObject::~assistantObject()
{

}

void assistantObject::handleUiMessage(int message, QString data)
{
    QDateTime time = QDateTime::currentDateTime();
    QString currentPath;
    currentPath = QDir::currentPath();
    QString logFileName = currentPath + "/fbc" + time.toString("yy-MM-dd-hh-mm-ss") + ".txt";
    QFile file(logFileName);

    qDebug() << logFileName;

    if(!file.open(QFile::WriteOnly|QFile::Text))
    {
        dispatchMessageToUi(FbcUpgHandler::PopUp|FbcUpgHandler::Failure,
                            tr("Sorry, 打开失败。这可能是由于没有足够的权限造成的！"));
    }
    else
    {
        if (file.write(data.toUtf8().data())!=-1)
            dispatchMessageToUi(FbcUpgHandler::PopUp|FbcUpgHandler::Success,
                                tr(logFileName.toLatin1()) + tr(" 保存成功！"));
        else
            dispatchMessageToUi(FbcUpgHandler::PopUp|FbcUpgHandler::Failure,
                                tr("log保存失败！"));

        file.close();
    }
}

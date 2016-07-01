
#include <QSerialPort>
#include <QDebug>
#include <QThread>
#include <QTime>
#include <QFile>
#include <QDir>
#include <QMetaType>
#include <QEventLoop>
#include <QTimer>
#include "define.h"
#include "fbcupg.h"
#include "settingsdialog.h"
#include "upgradesetting.h"

bool FbcUpgHandler::isUpgradePrepared = false;
bool FbcUpgHandler::isFactoryModeActive = false;

FbcUpgHandler::FbcUpgHandler()
{
    isUpgradeRunning = false;
    isSerialPortConnected = false;
    waitSecs = 0;
    currentFileSize = 0;
    totalUpgradeLength = 0;

    upgradeFileBuff = NULL;
    serial = NULL;
}

FbcUpgHandler::~FbcUpgHandler()
{
    resetUpgrade();
    resetSerialPort();
}

void FbcUpgHandler::resetSerialPort()
{
    closeSerialPort();
    resetSerialPortState();
}

void FbcUpgHandler::resetSerialPortState()
{
    if (pSettings)
    {
        delete pSettings;
        pSettings = NULL;
    }

    isSerialPortConnected = false;
}

void FbcUpgHandler::resetUpgrade()
{
    isUpgradeRunning = false;
    waitSecs = 0;
    currentFileSize = 0;

    totalUpgradeLength = 0;

    if (upgradeFileBuff)
    {
        delete upgradeFileBuff;
        upgradeFileBuff = NULL;
    }
}

FbcUpgHandler::partition_info_t* FbcUpgHandler::getPartitionInfoUpgFile(int section, int partition)
{
    unsigned int offset = 0;

    if (partition == PARTITION_FIRST_BOOT)
        offset = FIRST_BOOT_INFO_OFFSET;
    else if ((section < 2) && (partition < PARTITION_NUM)) {
        offset = section ? SECTION_1_INFO_OFFSET : SECTION_0_INFO_OFFSET;
        offset += PARTITION_INFO_SIZE * (partition - 1);
    }
    else
        return 0;

    return (partition_info_t *)(upgradeFileBuff + offset);
}

bool FbcUpgHandler::parseUpgradeInfo(const int section, const int partition, int &start, int &length)
{
    bool ret = false;
    partition_info_t *info = NULL;

    //TODO
    if (upgradeFileBuff && (SECTION_0 == section))
    {
        info = getPartitionInfoUpgFile(section, partition);

        switch (partition)
        {
        case PARTITION_FIRST_BOOT:
        case PARTITION_SECOND_BOOT:
        case PARTITION_SUSPEND:
        case PARTITION_UPDATE:
            start = info->code_offset;
            length = info->code_size + info->data_size;
            ret = true;
            break;
        case PARTITION_MAIN:
            start = info->code_offset;
            length = info->code_size + info->data_size + info->spi_code_size +
                     info->readonly_size + info->audio_param_size + info->sys_param_size;
            ret = true;
            break;
        case PARTITION_PQ:
        case PARTITION_USER:
        case PARTITION_FACTORY:
            start = info->data_offset;
            length = info->data_size;
            ret = true;
            break;
        default:
            break;
        }
    }

    return ret;
}

void FbcUpgHandler::handleError(QSerialPort::SerialPortError error)
{
    if (error == QSerialPort::ResourceError) {
        emit dispatchMessageToUi(PopUp|Failure, serial->errorString());
        resetSerialPortState();
        //closeSerialPort();//TODO
        killSianal = true;
    }
}

void FbcUpgHandler::configureSerialPort(SettingsDialog::Settings settings)
{
    pSettings = new SettingsDialog::Settings;

    if (pSettings != NULL)
    {
        pSettings->name = settings.name;
        pSettings->baudRate = settings.baudRate;
        pSettings->stringBaudRate = settings.stringBaudRate;
        pSettings->dataBits = settings.dataBits;
        pSettings->stringDataBits = settings.stringDataBits;
        pSettings->parity = settings.parity;
        pSettings->stringParity = settings.stringParity;
        pSettings->stopBits = settings.stopBits;
        pSettings->stopBits = settings.stopBits;
        pSettings->stringStopBits = settings.stringStopBits;
        pSettings->flowControl = settings.flowControl;
        pSettings->stringFlowControl = settings.stringFlowControl;
        pSettings->localEchoEnabled = settings.localEchoEnabled;
    }
}

void FbcUpgHandler::readData()
{
    if (!serial || !serial->bytesAvailable())
        return;

    QString data = serial->readAll();

    //TODO
    if (!isUpgradeRunning && isUpgradePrepared)
    {
        if (data.contains("pre-boot#"))
        {
            //UpgradeSetting::upgradegMode = FbcUpgHandler::FactoryLiteMode;
            FbcUpgHandler::isFactoryModeActive = true;
            emit dispatchMessageToUi(Console, "\n当前升级模式为工厂模式，不使用升级配置中"
                                     "的普通升级方式，但保留您在升级配置中选择的时间配置\n");
        }

        if (data.contains("fbc-main#")||
            data.contains("fbc-boot#")||
            data.contains("pre-boot#"))
        {
            emit dispatchMessageToUi(Console|WaitPrepare|Success, "");
        }
    }

    emit dispatchMessageToUi(Console|Write, data);
}

void FbcUpgHandler::getUpgradeInfoFromUI(QList<FbcUpgHandler::sectionInfo> sectionInfoList)
{
    QString upgradeInfo = "\n分区信息\n";

    totalUpgradeLength = 0;

    list.clear();
    list = sectionInfoList;

    for (int i = 0; i < list.length(); i++)
    {
        int start = static_cast<sectionInfo>(list.at(i)).start;
        int length = static_cast<sectionInfo>(list.at(i)).length;

        upgradeInfo += tr("\n分区 %1 : 起始地址(%2), 大小(%3)\n")
                .arg(i, 0, 16)
                .arg(start, 0, 16)
                .arg(length, 0, 16);

        totalUpgradeLength += length;
    }

    emit dispatchMessageToUi(Console|Upgrade, upgradeInfo);
}

void FbcUpgHandler::handleUiMessage(int message, QString data)
{
    bool ret = false;
    int device = message&0xf;
    int instruction = message&0xf0;
//    int state = message&0xf00;

    if (Serial == device)
    {
        if (Open == instruction)
        {
            //TODO
            ret = openSerialPort(*pSettings);
            if (ret)
            {
                connect(serial, &QSerialPort::readyRead, this, &FbcUpgHandler::readData);
                connect(serial, static_cast<void (QSerialPort::*)(QSerialPort::SerialPortError)>(&QSerialPort::error),
                        this, &FbcUpgHandler::handleError);
            }
        }
        else if (Close == instruction)//TODO
        {
            resetUpgrade();
            resetSerialPort();
            emit dispatchMessageToUi(Invalid, "");
        }

        if (Clean == instruction)
        {
            serial->clear();
        }
    }
    else if(File == device)
    {
        if (Open == instruction)
        {
            QString fileName = data;
            QString responseData;

            bool ret = loadUpgradefile(fileName, responseData);

            if (ret)
            {
                initBlocksInfo(list);
                emit sendUpgradeInfoToUI(list);
            }
            else
            {
                dispatchMessageToUi(PopUp|Failure, responseData);
            }
        }
    }
    else if(Console == device)
    {
        if (Write == instruction)
        {
            if (false == isUpgradeRunning)
                serial->write(data.toLatin1());
            else
                emit dispatchMessageToUi(Console|Write, tr("正在升级，不支持输入！"));
        }
    }
    else if (Invalid == device)
    {
        ;//TODO
    }
    else
    {
        Q_ASSERT(false);
    }
}

bool FbcUpgHandler::openSerialPort(SettingsDialog::Settings settings)
{
    qDebug() << "FbcUpgHandler::openSerialPort()" << QThread::currentThread();

    serial = new QSerialPort(this);

    if (serial)
    {
        serial->setPortName(settings.name);
        serial->setBaudRate(settings.baudRate);
        serial->setDataBits(settings.dataBits);
        serial->setParity(settings.parity);
        serial->setStopBits(settings.stopBits);
        serial->setFlowControl(settings.flowControl);

        if (serial->open(QIODevice::ReadWrite))
        {
            isSerialPortConnected = true;
            dispatchMessageToUi(PopUp|Success, tr("打开串口:%1成功(波特率:%2)").
                                arg(settings.name).arg(settings.baudRate));
            return true;
        }
        else
        {
            isSerialPortConnected = false;
            dispatchMessageToUi(PopUp|Failure, serial->errorString());
            return false;
        }
    }
    else
    {
        dispatchMessageToUi(PopUp|Failure, tr("内存不足"));
        return false;
    }
}

void FbcUpgHandler::closeSerialPort()
{
    if (serial)
    {
        if (serial->isOpen())
            serial->close();

        delete serial;
        serial = NULL;
    }
}

unsigned int FbcUpgHandler::crc32(unsigned int crc, unsigned char *ptr, unsigned int buf_len)
{
    unsigned int crcu32 = crc;
    static const unsigned int s_crc32[16] ={0, 0x1db71064, 0x3b6e20c8, 0x26d930ac, 0x76dc4190, 0x6b6b51f4,
                                            0x4db26158, 0x5005713c,0xedb88320, 0xf00f9344, 0xd6d6a3e8, 0xcb61b38c,
                                            0x9b64c2b0, 0x86d3d2d4, 0xa00ae278, 0xbdbdf21c };
    if (!ptr)
        return 0;

    crcu32 = ~crcu32;

    while(buf_len--)
    {
        unsigned char b = *ptr++;
        crcu32 = (crcu32 >> 4) ^ s_crc32[(crcu32 & 0xF) ^ (b & 0xF)];
        crcu32 = (crcu32 >> 4) ^ s_crc32[(crcu32 & 0xF) ^ (b >> 4)];
    }

    return ~crcu32;
}

bool FbcUpgHandler::loadUpgradefile(const QString &filename, QString &responseData)
{
    QFile file(filename);

    if (upgradeFileBuff)
    {
        delete upgradeFileBuff;
        upgradeFileBuff = NULL;
    }

    if(!file.open(QFile::ReadOnly))
    {
        responseData = tr("对不起, 打开失败。这可能是由于没有足够的权限造成的！");
        return false;
    }
    else
    {
        if(file.size() != MBYTE(2) && file.size() != MBYTE(4))
        {
            file.close();
            responseData = tr("对不起, 只支持%1或者%2字节大小文件！").arg(MBYTE(2)).arg(MBYTE(4));
            return false;
        }

        currentFileSize = file.size();

        upgradeFileBuff = new unsigned char[file.size()];

        if (upgradeFileBuff != NULL)
        {
            memset (upgradeFileBuff, 0, file.size());

            if (-1 == file.read((char*)upgradeFileBuff, file.size()))
            {
                file.close();
                responseData = tr("对不起, 读取文件失败");
                return false;
            }
        }
        else
        {
            file.close();
            responseData = tr("对不起, 内存不足");
            return false;
        }

        file.close();
    }

    return true;
}

bool FbcUpgHandler::parseFbcResponse(QByteArray &responseData)
{
    QByteArray okResponse;
    QByteArray failureResponse;

    okResponse.append(0x5a);
    okResponse.append(0x5a);
    okResponse.append(0x5a);
    okResponse.append(0x5a);

    failureResponse.append(0xa5);
    failureResponse.append(0xa5);
    failureResponse.append(0xa5);
    failureResponse.append(0xa5);

    if (responseData.contains(okResponse))
    {
        emit dispatchMessageToUi(Console|Write, tr("\n成功写入%1字节数据。\n ").arg(KBYTE(64)));
        return true;
    }
    else if (responseData.contains(failureResponse))
    {
        emit dispatchMessageToUi(Console|Write, tr("\n写入失败，重新写入... \n ").arg(KBYTE(64)));
        return false;
    }
    else
    {
        emit dispatchMessageToUi(Console|Write, tr("\n没有获得Fbc响应，重新连接！ \n "));
        return false;
    }
}

void FbcUpgHandler::prepareUpgrade()
{
    if (serial)
    {
        serial->putChar(0x20);
        serial->putChar('\r');
    }
}

void FbcUpgHandler::syncHandleSerialPort(QByteArray &rData, int delayMsec)
{
    QByteArray temp;
    QTime time;

    serial->clearError();
    serial->clear();
    serial->write(rData.data(), rData.length());
    //serial->flush();

    rData.clear();

    time.start();

    while (serial->waitForReadyRead(delayMsec))
    {
        temp = serial->readAll();
        rData += temp;

        emit dispatchMessageToUi(Console|Write, temp);
    }

    emit dispatchMessageToUi(Console|Write, tr("\n读取数据等待%1秒\n").arg(time.elapsed()/1000.0));
}

void FbcUpgHandler::initBlocksInfo(QList<sectionInfo> &list)
{
    int currentPartition = 0;
    int start = 0, length = 0;

    list.clear();

    if (isFactoryModeActive)
    //if (FactoryLiteMode == UpgradeSetting::upgradegMode)
    {
        list.append(sectionInfo(KEY_OFFSET, KEY_SIZE));
        list.append(sectionInfo(FIRST_BOOT_INFO_OFFSET, FIRST_BOOT_INFO_SIZE));
    }

    list.append(sectionInfo(SECTION_0_INFO_OFFSET, SECTION_INFO_SIZE));

    for (currentPartition = PARTITION_SECOND_BOOT; currentPartition < PARTITION_NUM; currentPartition++)
    {
        parseUpgradeInfo(SECTION_0, currentPartition, start, length);
        list.append(sectionInfo(start, length));
    }
}

void FbcUpgHandler::doUpgrade()
{
    qDebug() << "FbcUpgHandler::doUpgrade" << QThread::currentThreadId();
    unsigned int crc = 0, count = 0;
    int currentWriteLength = 0, allPartitionWriteLength = 0;
    int currentPartition = 0, reSendTimes = 0;
    int start = 0, length = 0, rate = 0;
    QByteArray cmdStr;
    char *pBuff = NULL;

    emit dispatchMessageToUi(Console, tr("\n请耐心等待%1s秒，等待接收升级文件，"
                                         "请不要拔掉电源或者串口。。。\n")
                             .arg(UpgradeSetting::waitRebootSecs));

    cmdStr.clear();
    isUpgradeRunning = true;
    killSianal = false;
    emit dispatchMessageToUi(Invalid, tr(""));
    disconnect(serial, &QSerialPort::readyRead, this, &FbcUpgHandler::readData);

    QTime time;
    time.start();

    if (NormalLiteMode == UpgradeSetting::getCurrentUpgradeMode())
    {
        cmdStr = "reboot -r upgrade_lite\r";
    }
    else if (NormalFullMode == UpgradeSetting::getCurrentUpgradeMode())
    {
        cmdStr = "reboot -r upgrade\r";
    }
    //else if (FactoryLiteMode == UpgradeSetting::upgradegMode)
    //if fbc crash or etc mormal mode invalid
    if (FbcUpgHandler::isFactoryModeActive)
    {
        cmdStr = "reboot -r upgrade\r";
        FbcUpgHandler::isFactoryModeActive = false;
    }

    syncHandleSerialPort(cmdStr, UpgradeSetting::waitReponseMsecs);

    QThread::sleep(UpgradeSetting::waitRebootSecs);
    serial->clear();

    for (currentPartition = 0; !killSianal && (currentPartition < list.length()); currentPartition++)
    {
        currentWriteLength = 0;

        start = static_cast<sectionInfo>(list.at(currentPartition)).start;
        length = static_cast<sectionInfo>(list.at(currentPartition)).length;

        if (start == 0x49030)
        {
            start = 0x49000;
            length += 0x30;
        }

        while(!killSianal && (currentWriteLength < length))
        {
            count = (UNIT_LENGTH <= (length-currentWriteLength)?
                     UNIT_LENGTH : length-currentWriteLength);
            cmdStr = QString("upgrade 0x%1 0x%2\r").arg(start, 0, 16)
                                                   .arg(count, 0, 16).toLatin1();

            syncHandleSerialPort(cmdStr, UpgradeSetting::waitReponseMsecs);

            pBuff = (char*)(upgradeFileBuff + start);
            cmdStr.clear();
            cmdStr.append(pBuff, count);

            syncHandleSerialPort(cmdStr, UpgradeSetting::waitReponseMsecs);

            crc = crc32(0, (unsigned char*)pBuff, count);
            char crcbuf[4] = {0};
            crcbuf[0] = crc & 0xFF;
            crcbuf[1] = (crc >> 8) & 0xFF;
            crcbuf[2] = (crc >> 16) & 0xFF;
            crcbuf[3] = (crc >> 24) & 0xFF;

            cmdStr.clear();
            cmdStr.append(crcbuf, 4);

            syncHandleSerialPort(cmdStr, UpgradeSetting::waitReponseMsecs);

            if (parseFbcResponse(cmdStr))
            {
                reSendTimes = 0;
                currentWriteLength += count;
                allPartitionWriteLength += count;
                start += count;
                rate = (++allPartitionWriteLength)*100/totalUpgradeLength;
            }
            else
            {
                reSendTimes++;

                if (reSendTimes > 3)
                {
                    killSianal = true;
                    break;
                }
            }

            emit dispatchMessageToUi(ProgressBar|Upgrade, tr("%1").arg(rate));
        }
    }

    cmdStr = "reboot\r";
    syncHandleSerialPort(cmdStr, UpgradeSetting::waitReponseMsecs);

    emit dispatchMessageToUi(ProgressBar|Upgrade, tr("%1").arg(0));

    resetUpgrade();

    if (!killSianal)
    {
        connect(serial, &QSerialPort::readyRead, this, &FbcUpgHandler::readData);
        dispatchMessageToUi(PopUp|Success, tr("升级成功,耗时%1秒\n ").arg(time.elapsed()/1000.0));
    }
    else
    {
        killSianal = false;
        resetSerialPort();
        dispatchMessageToUi(PopUp|Failure, tr("升级失败,耗时%1秒\n ").arg(time.elapsed()/1000.0));
    }

    return;
}


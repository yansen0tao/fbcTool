#ifndef FBCUPG_H
#define FBCUPG_H

#include <QSerialPort>
#include <QThread>
#include "settingsdialog.h"

class QString;
class QSerialPort;

class FbcUpgHandler : public QObject
{
    Q_OBJECT
public:
    enum Device
    {
        Serial = 0x1,
        File = 0x2,
        Console = 0x3,
        PopUp = 0x4,
        ProgressBar = 0x5,
        Invalid = 0x06,
    };

    enum Instruction
    {
        Open = 0x10,
        Close = 0x20,
        Read = 0x30,
        Write = 0x40,
        Clean = 0x50,
        Save = 0x60,
        Exit = 0x70,
        Upgrade = 0x80,
        WaitPrepare = 0x90,
    };

    enum State
    {
        Success = 0x100,
        Failure = 0x200,
    };

    enum UpgradegMode
    {
        NormalLiteMode= 0,
        NormalFullMode,
        //FactoryLiteMode,
    };

    enum
    {
        SECTION_0,
        SECTION_1,
    };

    enum
    {
        PARTITION_FIRST_BOOT = 0,
        PARTITION_SECOND_BOOT,
        PARTITION_SUSPEND,
        PARTITION_UPDATE,
        PARTITION_MAIN,
        PARTITION_PQ,
        PARTITION_USER,
        PARTITION_FACTORY,
        PARTITION_NUM,
    };

    struct partition_info_t
    {
        public:
            unsigned code_offset;
            unsigned code_size;
            unsigned data_offset;
            unsigned data_size;
            unsigned bss_offset;
            unsigned bss_size;
            unsigned readonly_offset;
            unsigned readonly_size;
            unsigned char signature[256];
            unsigned spi_code_offset;
            unsigned spi_code_size;
            unsigned audio_param_offset;
            unsigned audio_param_size;
            unsigned sys_param_offset;
            unsigned sys_param_size;
            unsigned crc;
            unsigned char sha[32];
    } ;

    class sectionInfo
    {
    public:
        int start;
        int length;

        sectionInfo(int start = 0, int length = 0)
        {
            this->start = start;
            this->length = length;
        }
    };
public:
    FbcUpgHandler();
    ~FbcUpgHandler();
    bool isUpgradeRunning;
    bool isSerialPortConnected;
    qint32 waitSecs;
    void configureSerialPort(SettingsDialog::Settings settings);
    static bool isUpgradePrepared;
    static bool isFactoryModeActive;
private:
    const int LAYOUT_VERSION_OFFSET = 0x40000;
    const int LAYOUT_VERSION_SIZE = 0x1000;
    const int KEY_OFFSET = 0x0;
    const int KEY_SIZE = 0x41000;
    const int PARTITION_INFO_SIZE = 0x200;
    const int FIRST_BOOT_INFO_OFFSET = 0x41000;
    const int FIRST_BOOT_INFO_SIZE = 0x1000;
    const int SECTION_INFO_SIZE = 0x1000;
    const int SECTION_0_INFO_OFFSET = 0x42000;
    const int SECTION_1_INFO_OFFSET = 0x43000;
    const int FIRST_BOOT_OFFSET = 0x44000;
    const int FIRST_BOOT_SIZE = 0x5000;
    const int SECTION_SIZE = 0xAD000;
    const int SECTION_0_OFFSET = 0x49000;
    const int SECTION_1_OFFSET = 0xF6000;
private:
    unsigned char *upgradeFileBuff;
    QSerialPort *serial;
    SettingsDialog::Settings *pSettings;
    qint64 currentFileSize;
    QList<sectionInfo> list;
    int totalUpgradeLength;
    bool killSianal;
private:
    unsigned int crc32(unsigned int crc, unsigned char *ptr, unsigned int buf_len);
    bool loadUpgradefile(const QString &filename, QString &responseData);
    bool openSerialPort(SettingsDialog::Settings Settings);
    void closeSerialPort();
    void syncHandleSerialPort(QByteArray &rData, int delayMsec);
    bool parseFbcResponse(QByteArray &responseData);
    void resetSerialPortState();
    void resetSerialPort();
    void resetUpgrade();
    void initBlocksInfo(QList<sectionInfo> &list);
    bool parseUpgradeInfo(const int section, const int partition, int &start, int &length);
    FbcUpgHandler::partition_info_t* getPartitionInfoUpgFile(int section, int partition);
signals:
    void dispatchMessageToUi(int message, QString data);
    void sendUpgradeInfoToUI(QList<sectionInfo> list);
public slots:
    void getUpgradeInfoFromUI(QList<FbcUpgHandler::sectionInfo> list);
    void handleUiMessage(int message, QString data);
    void handleError(QSerialPort::SerialPortError error);
    void readData();
    void doUpgrade();
    void prepareUpgrade();
};

#endif

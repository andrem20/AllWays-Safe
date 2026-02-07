#ifndef TRAFFICCONTROLSYSTEM_MFRC522_HPP
#define TRAFFICCONTROLSYSTEM_MFRC522_HPP

#include <atomic>
#include <functional>
#include <memory>

#include "SPI_DeviceDriver.hpp"
#include "../../CppWrapper/CppWrapper.hpp"

/* MACROS */
#define SPI_PATH    "/dev/spidev0.0"
#define SPI_MODE    SPI_MODE_0
#define SPI_BITS    8
#define SPI_SPEED   1000000U

// MFRC522 Register Definitions
#define CommandReg          0x01
#define ComIEnReg           0x02
#define DivIEnReg           0x03
#define ComIrqReg           0x04
#define DivIrqReg           0x05
#define ErrorReg            0x06
#define Status1Reg          0x07
#define Status2Reg          0x08
#define FIFODataReg         0x09
#define FIFOLevelReg        0x0A
#define WaterLevelReg       0x0B
#define ControlReg          0x0C
#define BitFramingReg       0x0D
#define CollReg             0x0E
#define ModeReg             0x11
#define TxModeReg           0x12
#define RxModeReg           0x13
#define TxControlReg        0x14
#define TxASKReg            0x15
#define TxSelReg            0x16
#define RxSelReg            0x17
#define RxThresholdReg      0x18
#define DemodReg            0x19
#define MfTxReg             0x1C
#define MfRxReg             0x1D
#define SerialSpeedReg      0x1F
#define CRCResultRegH       0x21
#define CRCResultRegL       0x22
#define ModWidthReg         0x24
#define RFCfgReg            0x26
#define GsNReg              0x27
#define CWGsPReg            0x28
#define ModGsPReg           0x29
#define TModeReg            0x2A
#define TPrescalerReg       0x2B
#define TReloadRegH         0x2C
#define TReloadRegL         0x2D
#define TCounterValRegH     0x2E
#define TCounterValRegL     0x2F
#define TestSel1Reg         0x31
#define TestSel2Reg         0x32
#define TestPinEnReg        0x33
#define TestPinValueReg     0x34
#define TestBusReg          0x35
#define AutoTestReg         0x36
#define VersionReg          0x37
#define AnalogTestReg       0x38
#define TestDAC1Reg         0x39
#define TestDAC2Reg         0x3A
#define TestADCReg          0x3B

// MFRC522 Commands
#define PCD_IDLE            0x00
#define PCD_AUTHENT         0x0E
#define PCD_RECEIVE         0x08
#define PCD_TRANSMIT        0x04
#define PCD_TRANSCEIVE      0x0C
#define PCD_SOFT_RESET      0x0F
#define PCD_CALCCRC         0x03

/* Proximity Integrated Circuit Card Commands */
#define PICC_REQIDL         0x26
#define PICC_REQALL         0x52
#define PICC_ANTICOLL       0x93
#define PICC_SELECTTAG      0x93
#define PICC_AUTHENT1A      0x60
#define PICC_AUTHENT1B      0x61
#define PICC_READ           0x30
#define PICC_WRITE          0xA0
#define PICC_DECREMENT      0xC0
#define PICC_INCREMENT      0xC1
#define PICC_RESTORE        0xC2
#define PICC_TRANSFER       0xB0
#define PICC_HALT           0x50

/* Status Codes */
#define OK               0
#define NOTAGERR         1
#define ERR              2

class MFRC522
{
private:
    /*--- General Attributes -----------------------------------------------------------------------------------------*/
    std::unique_ptr<SPI_DeviceDriver> spi_driver;

    /*--- Methods ----------------------------------------------------------------------------------------------------*/
    int mfrc522_write_register (int reg, int value);
    void mfrc522_set_bit_mask(int reg, int mask);
    void mfrc522_calculate_crc(uint8_t *data, int len, uint8_t *result) ;
    void mfrc522_timer_config ();
    void mrfrc522_TX_config();
    void mfrc522_antenna_on ();
    void mfrc522_reset ();
    void mfrc522_start ();
    int mfrc522_to_card(int command, uint8_t *send_data,
                    int send_len, uint8_t *back_data, int *back_len);

    /*--- Threading & Synchronization Resources ----------------------------------------------------------------------*/
    CppWrapper::Thread mfrc522Thread;
    static void* t_readRFID(void* arg);
    CppWrapper::Mutex mfrc522Mutex;

    using MFRC522callback = std::function<void(uint32_t uuid)>;
    MFRC522callback mfrc522Callback;

    CppWrapper::Timer mfrc522Timer;

    std::atomic<bool>& _shutdown_requested;

public:
    /*--- Constructors/Destructor ------------------------------------------------------------------------------------*/
    MFRC522(MFRC522callback cardCallback, std::atomic<bool>& shutdownRequested);
    MFRC522( MFRC522callback cardCallback, std::atomic<bool>& shutdownRequested, const char* devpath, int mode,
            int bits, int speed);
    ~MFRC522();
    /*--- Methods ----------------------------------------------------------------------------------------------------*/
    void start();
    void stop();

    int mfrc522_read_register (int reg);
    void mfrc522_init ();
    int mfrc522_request(int req_mode, uint8_t *tag_type);
    int mfrc522_select_tag(uint8_t *serial_num);
    int mfrc522_anticoll(uint8_t *serial_num);
    int mfrc522_auth(int auth_mode, int block_addr,
                 int *sector_key, uint8_t *serial_num);
    int mfrc522_read_block(int block_addr, uint8_t *recv_data);
    int mfrc522_write_block(int block_addr, uint8_t *write_data);
    void mfrc522_clear_bit_mask(int reg, int mask);
    void mfrc522_halt();
};
#endif //TRAFFICCONTROLSYSTEM_MFRC522_HPP
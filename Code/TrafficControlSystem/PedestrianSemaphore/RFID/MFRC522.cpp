#include "MFRC522.hpp"

#include <cstdint>
#include <cstring>
#include <iostream>
#include <unistd.h>
#include <linux/spi/spi.h>
#include <linux/spi/spidev.h>
#include <sys/ioctl.h>


/**
 * @brief Constructor with default SPI parameters
 */
MFRC522::MFRC522(MFRC522callback cardCallback, std::atomic<bool>& shutdownRequested) :
            mfrc522Thread(t_readRFID), _shutdown_requested(shutdownRequested)
{
    spi_driver = std::make_unique<SPI_DeviceDriver>(SPI_PATH, SPI_MODE, SPI_BITS, SPI_SPEED);
    mfrc522Callback = std::move(cardCallback);

    mfrc522_init();
    std::cout<< "MFRC522 Initialized successfully\n"<<std::endl;
}

/**
 * @brief Constructor with custom SPI parameters.
 * @param devpath Path to SPI device
 * @param mode SPI mode
 * @param bits Number of SPI bits
 * @param speed SPI speed in Hz
 */
MFRC522::MFRC522( MFRC522callback cardCallback, std::atomic<bool>& shutdownRequested,const char* devpath,const int mode,
        const int bits,const int speed) :
    mfrc522Thread(t_readRFID), _shutdown_requested(shutdownRequested)
{
    spi_driver = std::make_unique<SPI_DeviceDriver>(SPI_PATH, SPI_MODE, SPI_BITS, SPI_SPEED);
    mfrc522Callback = std::move(cardCallback);


    mfrc522_init();
    std::cout<< "Initialized successfully\n"<<std::endl;
}

/**
 * @brief Destructor, frees SPI driver.
 */
MFRC522::~MFRC522()
{
    std::cout<< "MFRC Destroyed successfully\n"<<std::endl;
    mfrc522Thread.join();
}

void MFRC522::start()
{
    mfrc522Thread.run(this);
}

void MFRC522::stop()
{
    mfrc522Thread.join();
}

/**
 * @brief Write a value to an MFRC522 register.
 * @param reg Register address
 * @param value Value to write
 * @return 0 on success, -1 on error
 */
int MFRC522::mfrc522_write_register(int reg, int value)
{
    uint8_t tx_data[2];

    // Write command: bit 7 clear, address in bits 6-1, bit 0 clear
    tx_data[0] = (reg << 1) & 0x7E;
    tx_data[1] = value;

    spi_ioc_transfer transfer{
        .tx_buf        = (unsigned long)tx_data,
        .rx_buf        = 0,
        .len           = 2,
        .speed_hz      = (__u32)spi_driver->get_speed_hz(),
        .delay_usecs   = 0,
        .bits_per_word = (__u8)spi_driver->get_spibits(),
        .cs_change     = 0
    };


    int ret = ioctl(spi_driver->get_fd(), SPI_IOC_MESSAGE(1), &transfer);

    if (ret < 0) {
        perror("SPI write register failed");
        return -1;
    }

    return 0;
}

/**
 * @brief Read a value from an MFRC522 register.
 * @param reg Register address
 * @return Register value or -1 on error
 */
int MFRC522::mfrc522_read_register(const int reg)
{
    uint8_t tx_data[2];
    uint8_t rx_data[2];

    // Read command: bit 7 set, address in bits 6-1, bit 0 clear
    tx_data[0] = ((reg << 1) & 0x7E) | 0x80;
    tx_data[1] = 0x00;  // Dummy byte

    spi_ioc_transfer transfer{
        .tx_buf        = (unsigned long)tx_data,
        .rx_buf        = (unsigned long)rx_data,
        .len           = 2,
        .speed_hz      = (__u32)spi_driver->get_speed_hz(),
        .delay_usecs   = 0,
        .bits_per_word = (__u8)spi_driver->get_spibits(),
        .cs_change     = 0
    };


    int ret = ioctl(spi_driver->get_fd(), SPI_IOC_MESSAGE(1), &transfer);

    if (ret < 0) {
        perror("SPI read register failed");
        return -1;
    }

    return rx_data[1];  // Data comes back in second byte
}

/**
 * @brief Read a 16-byte block from the RFID card.
 * @param block_addr Block address
 * @param recv_data Buffer to store received data
 * @return OK if successful, ERR otherwise
 */
int MFRC522::mfrc522_read_block(int block_addr, uint8_t *recv_data) {
    int status;
    int unLen;

    recv_data[0] = PICC_READ;
    recv_data[1] = block_addr;
    mfrc522_calculate_crc(recv_data, 2, &recv_data[2]);

    status = mfrc522_to_card(PCD_TRANSCEIVE, recv_data, 4, recv_data, &unLen);

    if ((status != OK) || (unLen != 0x90)) {
        status = ERR;
    }

    return status;
}

/**
 * @brief Write a 16-byte block to the RFID card.
 * @param block_addr Block address
 * @param write_data Data to write (16 bytes)
 * @return OK if successful, ERR otherwise
 */
int MFRC522::mfrc522_write_block(int block_addr, uint8_t *write_data) {
    int status;
    int recv_bits;
    int i;
    uint8_t buffer[18];

    buffer[0] = PICC_WRITE;
    buffer[1] = block_addr;
    mfrc522_calculate_crc(buffer, 2, &buffer[2]);

    status = mfrc522_to_card(PCD_TRANSCEIVE, buffer, 4, buffer, &recv_bits);

    if ((status != OK) || (recv_bits != 4) || ((buffer[0] & 0x0F) != 0x0A)) {
        status = ERR;
    }

    if (status == OK) {
        for (i = 0; i < 16; i++) {
            buffer[i] = write_data[i];
        }
        mfrc522_calculate_crc(buffer, 16, &buffer[16]);

        status = mfrc522_to_card(PCD_TRANSCEIVE, buffer, 18, buffer, &recv_bits);

        if ((status != OK) || (recv_bits != 4) || ((buffer[0] & 0x0F) != 0x0A)) {
            status = ERR;
        }
    }

    return status;
}

/**
 * @brief Put the RFID card into halt state.
 */
void MFRC522::mfrc522_halt() {
    uint8_t buffer[4];
    int unLen;

    buffer[0] = PICC_HALT;
    buffer[1] = 0;
    mfrc522_calculate_crc(buffer, 2, &buffer[2]);

    mfrc522_to_card(PCD_TRANSCEIVE, buffer, 4, buffer, &unLen);
}

/**
 * @brief Reset the MFRC522 (soft reset).
 */
void MFRC522::mfrc522_reset ()
{
    // Perform Soft Reset
    mfrc522_write_register(CommandReg, PCD_SOFT_RESET);
    // Section 8.8.2 - ~38us delay -> (generous) 100us wait for internal delay
    usleep(10000);
}

/**
 * @brief Start the MFRC522 in transceive mode.
 */
void MFRC522::mfrc522_start ()
{
    // Init in Transceive Mode
    mfrc522_write_register(CommandReg, PCD_TRANSCEIVE);
}

/*
 * Timer is required for IEEE delay between sent messages
* AN12057 - The default FWI is 4
* ieee FWT = (256 x 16/fc) x 2^FWI <=> FWT ~ 6ms
*
* Section 9.3.3.10 MFRC522 datasheet
 */
/**
 * @brief Configure MFRC522 internal timer.
 */
void MFRC522::mfrc522_timer_config ()
{
    mfrc522_write_register(TModeReg, 0x80);      // TAuto
    mfrc522_write_register(TPrescalerReg, 0xA9); //
    mfrc522_write_register(TReloadRegL, 0x03);
    mfrc522_write_register(TReloadRegH, 0xE8);
}

/**
 * @brief Configure MFRC522 for transmission.
 * @note Force 100% ASK modulation
 */
void MFRC522::mrfrc522_TX_config()
{
    mfrc522_write_register(TxASKReg, 0x40);  //
    // TxWaitRF 1 and PolMFin active High and  CRCPreset 01 in order to copmly with IEEE norm for CRC initial value
    mfrc522_write_register(ModeReg, 0x3D); // 0x3D
}

/**
 * @brief Initialize MFRC522 device.
 */
void MFRC522::mfrc522_init ()
{
    mfrc522_reset();
    mfrc522_timer_config ();
    mrfrc522_TX_config ();
    mfrc522_antenna_on();
}

/**
 * @brief Set specific bits of a register.
 * @param reg Register address
 * @param mask Bits to set
 */
void MFRC522::mfrc522_set_bit_mask(int reg, int mask) {
    int tmp = mfrc522_read_register(reg);
    mfrc522_write_register(reg, tmp | mask);
}

/**
 * @brief Clear specific bits of a register.
 * @param reg Register address
 * @param mask Bits to clear
 */
void MFRC522::mfrc522_clear_bit_mask(int reg, int mask) {
    int tmp = mfrc522_read_register(reg);
    mfrc522_write_register(reg, tmp & (~mask));
}

/**
 * @brief Turn on the MFRC522 antenna.
 */

/*
 * Section 8.6.3 - Transmitter power-down mode
 *  TxControlReg bit 0 and 1 must be set to 1 always
 */
void MFRC522::mfrc522_antenna_on() {
    int temp = mfrc522_read_register(TxControlReg);
    if (!(temp & 0x03))
        mfrc522_set_bit_mask(TxControlReg, 0x03);
}

/**
 * @brief Calculate CRC for MFRC522.
 * @param data Data to compute CRC on
 * @param len Length of data
 * @param result Buffer to store 2-byte CRC
 */
void MFRC522::mfrc522_calculate_crc(uint8_t *data, int len, uint8_t *result) {
    int i, n;

    mfrc522_clear_bit_mask(DivIrqReg, 0x04);
    mfrc522_set_bit_mask(FIFOLevelReg, 0x80);

    for (i = 0; i < len; i++) {
        mfrc522_write_register(FIFODataReg, data[i]);
    }

    mfrc522_write_register(CommandReg, PCD_CALCCRC);

    i = 0xFF;
    while (1) {
        n = mfrc522_read_register(DivIrqReg);
        i--;
        if (!(i != 0 && !(n & 0x04))) {
            break;
        }
    }

    result[0] = mfrc522_read_register(CRCResultRegL);
    result[1] = mfrc522_read_register(CRCResultRegH);
}

/**
 * @brief Communicate with RFID card.
 * @param command MFRC522 command
 * @param send_data Data to send
 * @param send_len Length of data to send
 * @param back_data Buffer to receive data
 * @param back_len Pointer to store received length (in bits)
 * @return OK, ERR, or NOTAGERR
 */
int MFRC522::mfrc522_to_card(int command, uint8_t *send_data,
                    int send_len, uint8_t *back_data, int *back_len) {
    int status = ERR;
    int irq_en = 0x00;
    int wait_irq = 0x00;
    int last_bits;
    int n;
    int i;

    switch (command) {
        case PCD_AUTHENT:
            irq_en = 0x12;
            wait_irq = 0x10;
            break;
        case PCD_TRANSCEIVE:
            irq_en = 0x77;
            wait_irq = 0x30;
            break;
        default:
            break;
    }

    mfrc522_write_register(ComIEnReg, irq_en | 0x80);
    mfrc522_clear_bit_mask(ComIrqReg, 0x80);
    mfrc522_set_bit_mask(FIFOLevelReg, 0x80);
    mfrc522_write_register(CommandReg, PCD_IDLE);

    // Write data to FIFO
    for (i = 0; i < send_len; i++) {
        mfrc522_write_register(FIFODataReg, send_data[i]);
    }

    // Execute command
    mfrc522_write_register(CommandReg, command);
    if (command == PCD_TRANSCEIVE) {
        mfrc522_set_bit_mask(BitFramingReg, 0x80);
    }

    // Wait for completion
    i = 2000;
    while (1) {
        n = mfrc522_read_register(ComIrqReg);
        i--;
        if (!(i != 0 && !(n & 0x01) && !(n & wait_irq))) {
            break;
        }
    }

    mfrc522_clear_bit_mask(BitFramingReg, 0x80);

    if (i != 0) {
        if (!(mfrc522_read_register(ErrorReg) & 0x1B)) {
            status = OK;

            if (n & irq_en & 0x01) {
                status = NOTAGERR;
            }

            if (command == PCD_TRANSCEIVE) {
                n = mfrc522_read_register(FIFOLevelReg);
                last_bits = mfrc522_read_register(ControlReg) & 0x07;

                if (last_bits) {
                    *back_len = (n - 1) * 8 + last_bits;
                } else {
                    *back_len = n * 8;
                }

                if (n == 0) {
                    n = 1;
                }
                if (n > 16) {
                    n = 16;
                }

                for (i = 0; i < n; i++) {
                    back_data[i] = mfrc522_read_register(FIFODataReg);
                }
            }
        } else {
            status = ERR;
        }
    }

    return status;
}


/**
 * @brief Authenticate to a block on the card.
 * @param auth_mode Authentication mode
 * @param block_addr Block address
 * @param sector_key Sector key (6 bytes)
 * @param serial_num Card serial number (4 bytes)
 * @return OK if success, ERR otherwise
 */
int MFRC522::mfrc522_auth(int auth_mode, int block_addr,
                 int *sector_key, uint8_t *serial_num) {
    int status;
    uint8_t buffer[12];
    int recv_bits;
    int i;

    buffer[0] = auth_mode;
    buffer[1] = block_addr;

    for (i = 0; i < 6; i++) {
        buffer[i + 2] = sector_key[i];
    }
    for (i = 0; i < 4; i++) {
        buffer[i + 8] = serial_num[i];
    }

    status = mfrc522_to_card(PCD_AUTHENT, buffer, 12, buffer, &recv_bits);

    if ((status != OK) || (!(mfrc522_read_register(Status2Reg) & 0x08))) {
        status = ERR;
    }

    return status;
}

/**
 * @brief Anti-collision detection to get card serial number.
 * @param serial_num Buffer to store serial number
 * @return OK if successful, ERR otherwise
 */
int MFRC522::mfrc522_anticoll(uint8_t *serial_num)
{
    int status;
    int i;
    int serial_num_check = 0;
    int unLen;

    mfrc522_write_register(BitFramingReg, 0x00);

    serial_num[0] = PICC_ANTICOLL;
    serial_num[1] = 0x20;

    status = mfrc522_to_card(PCD_TRANSCEIVE, serial_num, 2, serial_num, &unLen);

    if (status == OK) {
        // Check card serial number
        for (i = 0; i < 4; i++) {
            serial_num_check ^= serial_num[i];
        }
        if (serial_num_check != serial_num[i]) {
            status = ERR;
        }
    }

    return status;
}

/**
 * @brief Request a card and get its type.
 * @param req_mode Request mode
 * @param tag_type Buffer to store tag type
 * @return OK if successful, ERR otherwise
 */
int MFRC522::mfrc522_request(int req_mode, uint8_t *tag_type)
{
    int status;
    int back_bits;

    mfrc522_write_register(BitFramingReg, 0x07);

    tag_type[0] = req_mode;
    status = mfrc522_to_card(PCD_TRANSCEIVE, tag_type, 1, tag_type, &back_bits);

    if ((status != OK) || (back_bits != 0x10)) {
        status = ERR;
    }

    return status;
}

/**
 * @brief Select a tag by serial number.
 * @param serial_num Card serial number
 * @return SAK value if success, 0 otherwise
 */
int MFRC522::mfrc522_select_tag(uint8_t *serial_num)
{
    int status;
    int i;
    uint8_t buffer[9];
    int recv_bits;

    buffer[0] = PICC_SELECTTAG;
    buffer[1] = 0x70;

    for (i = 0; i < 5; i++) {
        buffer[i + 2] = serial_num[i];
    }

    mfrc522_calculate_crc(buffer, 7, &buffer[7]);
    status = mfrc522_to_card(PCD_TRANSCEIVE, buffer, 9, buffer, &recv_bits);

    if ((status == OK) && (recv_bits == 0x18)) {
        return buffer[0];
    } else {
        return 0;
    }
}

/*----Thread -----------------------------------------------------------------------------------*/
void* MFRC522::t_readRFID(void* arg)
{
    auto self = static_cast<MFRC522*>(arg);

    int status;
    uint8_t tag_type[2];
    uint8_t serial_num[5];

    while (!self->_shutdown_requested)
    {
        {
            // Request card
            CppWrapper::LockGuard lock (self->mfrc522Mutex);
            status = self->mfrc522_request(PICC_REQIDL, tag_type);

            if (status == OK)
            {
                std::cerr<< "Card Detected\n";
                status = self->mfrc522_anticoll(serial_num);
                if (status == OK)
                {
                    std::cerr<< "Got UUID\n";
                    const uint32_t uuid = serial_num[0] << 24  | serial_num[1] << 16 | serial_num[2] << 8 | serial_num[3] ;
                    self->mfrc522Callback(uuid);
                }
            }
        } // Lock Release
        self->mfrc522Timer.timerRun(1.1);
        self->mfrc522Timer.timerWait();
    }
    return arg;
}
#ifndef TRAFFICCONTROLSYSTEM_SPI_DEVICEDRIVER_HPP
#define TRAFFICCONTROLSYSTEM_SPI_DEVICEDRIVER_HPP

#include <string>

/**
 * @class SPI_DeviceDriver
 * @brief Encapsulates Linux SPI device driver operations.
 *
 * Provides methods to configure, access, and manage a SPI device file.
 */
class SPI_DeviceDriver {
private:
    int spi_fd;
    int32_t speed_hz;
    int spi_bits;
public:
    SPI_DeviceDriver (const char *path, int mode, int bits, int32_t speed);
    ~SPI_DeviceDriver ();

    [[nodiscard]] int get_fd () const;
    [[nodiscard]] int get_speed_hz () const;
    [[nodiscard]] int get_spibits () const;
};


#endif //TRAFFICCONTROLSYSTEM_SPI_DEVICEDRIVER_HPP
#include "SPI_DeviceDriver.hpp"

#include <cerrno>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <fcntl.h>
#include <iostream>
#include <unistd.h>
#include <linux/spi/spi.h>
#include <linux/spi/spidev.h>
#include <sys/ioctl.h>

/**
 * @brief Constructor: Initialize and configure a SPI device.
 *
 * Opens the SPI device at `devpath` and configures mode, bits per word, and speed.
 * @param devpath Path to SPI device (e.g., "/dev/spidev0.0")
 * @param mode SPI mode (CPOL and CPHA configuration)
 * @param bits Bits per word (usually 8)
 * @param speed Transmission speed in Hz
 *
 * @throws std::runtime_error If opening device or configuring SPI fails
 */
SPI_DeviceDriver::SPI_DeviceDriver (const char *devpath, const int mode, const int bits, const int32_t speed)
{
    // Open device
    spi_fd = open(devpath, O_RDWR);

    if (spi_fd < 0)
        std::cout << (std::string("SPI open failed: ") + strerror(errno) + "\n");

    if (
    // Configure SPI mode: CPOL, CPHA
    ioctl(spi_fd, SPI_IOC_WR_MODE32, &mode)
    <0 ||

    // Configure Bits per word - 8 bits
    ioctl(spi_fd, SPI_IOC_WR_BITS_PER_WORD, &bits)
    <0 ||
    // Configure Transmission Speed - 1MHz
    ioctl(spi_fd, SPI_IOC_WR_MAX_SPEED_HZ, &speed)
    <0
    )
    {
        close(spi_fd);
        throw std::runtime_error("SPI ioctl configuration failed");
    }
    spi_bits = bits;
    speed_hz = speed;
}

/**
 * @brief Destructor: Close the SPI device file.
 *
 * Closes the SPI device file descriptor if open.
 * @throws std::runtime_error If closing the file descriptor fails
 */
SPI_DeviceDriver:: ~SPI_DeviceDriver ()
{
    if (spi_fd >= 0)
    {
        if (close(spi_fd) <0)
        {
            std::cout << (std::string("SPI close failed: ") + strerror(errno) + "\n");
        }
    }
}

/**
 * @brief Get the SPI device file descriptor.
 * @return File descriptor of the SPI device
 */
int SPI_DeviceDriver::get_fd () const
{
    return spi_fd;
}

/**
 * @brief Get the configured SPI speed.
 * @return SPI speed in Hz
 */
int SPI_DeviceDriver::get_speed_hz () const
{
    return speed_hz;
}

/**
 * @brief Get the configured bits for communication.
 * @return bits number
 */
int SPI_DeviceDriver::get_spibits () const
{
    return spi_bits;
}
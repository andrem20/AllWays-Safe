
#include "PWM_DeviceDriver.hpp"

#include <iostream>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <cstdlib>
#include <fcntl.h>
#include <unistd.h>
#include <cstring>
#include <stdexcept>
#include <cerrno>

extern "C"{
#include "LinuxDeviceDriver/pwm_dd.h"
}

#define SUCCESS 0
#define ALREADY_PERFORMED_ACTION 256
#define ROOT_USER 0

#define KERNEL_OBJECT "/root/pwm.ko"
#define DEVICE_PATH "/dev/pwm"

/*---Constructor/Destructor---------------------------------------------------------------------------------------*/

PWM_DeviceDriver::PWM_DeviceDriver()
{
    file_descriptor = -1;
    // Force: insert module into the kernel
    while (!insertKernelModule()){usleep(100000);};
    // Open device to be ready for Write/Read Operations
    open_dd();
    setValue(0,0);
}
PWM_DeviceDriver::~PWM_DeviceDriver()
{
    // Close device
    close_dd();
    // Force: remove module from the kernel
    while (!removeKernelModule()){};

    file_descriptor = -1;
}

/*---System Handling----------------------------------------------------------------------------------------------*/

int PWM_DeviceDriver::setValue(int32_t freq, int duty) const
{
    if (freq < 0 || duty < 0 || duty > 100)
        return -EINVAL;
    ioctl_pwm_config_t conf = {freq, duty};
    ioctl (file_descriptor, PWM_SET_CONFIG, &conf);  //update value

    return 0;
}
void PWM_DeviceDriver::disable() const
{
    ioctl_pwm_config_t conf; // empty
    ioctl (file_descriptor, PWM_DISABLE, &conf);
}

/* Perform insmod via software */
bool PWM_DeviceDriver::insertKernelModule() {
    if (geteuid() != ROOT_USER) { // ALWAYS FALSE - on RPi I'm always root user
        std::cerr << "This program must be run as root" << std::endl;
        return false;
    }
    char modulePath[64];
    strcpy(modulePath, KERNEL_OBJECT);

    // struct stat: POSIX; contains file information
    struct stat buffer{};

    // Check if file exists and I have permission
    if (stat(modulePath, &buffer) != 0) {
        std::cerr << "Module file not found: " << modulePath << std::endl;
        return false;
    }

    // Build insmod command
    std::string cmd = "insmod ";
    cmd += modulePath;

    // Execute insmod (requires root privileges) - system call
    int result = system(cmd.c_str());
    sleep (1);

    if (result == SUCCESS) {
        std::cout << "Module LOADED successfully" << std::endl;
        return true;
    } else if (result == ALREADY_PERFORMED_ACTION)
    {
        std::cout << "Module ALREADY loaded" << std::endl;
        return true;
    }
    else {
        std::cerr << "Failed to load module. Error code: " << result << std::endl;
        return false;
    }
}

bool PWM_DeviceDriver::removeKernelModule()
{
    char modulePath[64];
    strcpy(modulePath, KERNEL_OBJECT);
    struct stat buffer{};

    // Check if file exists and I have permission
    if (stat(modulePath, &buffer) != 0) {
        std::cerr << "Module file not found: " << modulePath << std::endl;
        return false;
    }

    // Build rmmod command
    std::string cmd = "rmmod ";
    cmd += modulePath;

    // Execute rmmod (requires root privileges) - system call
    int result = system(cmd.c_str());

    if (result == SUCCESS) {
        std::cout << "Module REMOVED successfully" << std::endl;
        return true;
    } else if (result == ALREADY_PERFORMED_ACTION)
    {
        std::cout << "Module ALREADY removed" << std::endl;
        return true;
    }
    else {
        std::cerr << "Failed to remove module. Error code: " << result << std::endl;
        return false;
    }
}

// Opens device, gets  file_descriptor
int PWM_DeviceDriver::open_dd()
{
    // Open device
    std::string dev = std::string(DEVICE_PATH) + "0";
    int fd = open(dev.c_str(), O_RDWR);
    if (fd < 0) {
        std::cerr << "open failed: " << strerror(errno) << "\n";
        return -1;
    }
    file_descriptor = fd;
    return file_descriptor;

}
// Closes device based on file_descriptor
int PWM_DeviceDriver::close_dd()
{
    // Check if driver is loaded
    if (file_descriptor >= 0)
    {
        // Close device
        std::string dev = std::string(DEVICE_PATH) + "0";
        int fd = close(file_descriptor);
        if (fd < 0) {
            std::cerr << "open failed: " << strerror(errno) << "\n";
            return -1;
        }
        file_descriptor = -1; // No value attributed
        return fd;
    }else
    {
        std::cerr << "Module not loaded"<< "\n";
        return -1;
    }
}


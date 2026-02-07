#include <iostream>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <cstdlib>
#include <fcntl.h>
#include <unistd.h>
#include <cstring>
#include <stdexcept>

#include "PWM_DeviceDriver.hpp"

using namespace std;

extern "C"{
#include "../LinuxDeviceDriver/pwm_dd.h"
}

#define SUCCESS 0
#define ALREADY_PERFORMED_ACTION 256
#define ROOT_USER 0

#define KERNEL_OBJECT "/root/pwm.ko"
#define DEVICE_PATH "/dev/pwm"

PWM_DeviceDriver::PWM_DeviceDriver()
{
    file_descriptor = -1;
    // Force: insert module into the kernel
    while (!insertKernelModule()){};
    // Open device to be ready for Write/Read Operations
    open_dd();
}
PWM_DeviceDriver::~PWM_DeviceDriver()
{
    // Close device
    close_dd();
    // Force: remove module from the kernel
    while (!removeKernelModule()){};

    file_descriptor = -1;
}

/* Perform insmod via software */
bool PWM_DeviceDriver::insertKernelModule() {
    if (geteuid() != ROOT_USER) { // ALWAYS FALSE - on RPi I'm always root user
        cerr << "This program must be run as root" << endl;
        return false;
    }
    char modulePath[64];
    strcpy(modulePath, KERNEL_OBJECT);

    // struct stat: POSIX; contains file information
    struct stat buffer;

    // Check if file exists and I have permission
    if (stat(modulePath, &buffer) != 0) {
        cerr << "Module file not found: " << modulePath << endl;
        return false;
    }

    // Build insmod command
    string cmd = "insmod ";
    cmd += modulePath;

    // Execute insmod (requires root privileges) - system call
    int result = system(cmd.c_str());
    sleep (1);

    if (result == SUCCESS) {
        cout << "Module LOADED successfully" << endl;
        return true;
    } else if (result == ALREADY_PERFORMED_ACTION)
    {
        cout << "Module ALREADY loaded" << endl;
        return true;
    }
    else {
        cerr << "Failed to load module. Error code: " << result << endl;
        return false;
    }
}

bool PWM_DeviceDriver::removeKernelModule(void)
{
    char modulePath[64];
    strcpy(modulePath, KERNEL_OBJECT);
    struct stat buffer;

    // Check if file exists and I have permission
    if (stat(modulePath, &buffer) != 0) {
        cerr << "Module file not found: " << modulePath << endl;
        return false;
    }

    // Build rmmod command
    string cmd = "rmmod ";
    cmd += modulePath;

    // Execute rmmod (requires root privileges) - system call
    int result = system(cmd.c_str());

    if (result == SUCCESS) {
        cout << "Module REMOVED successfully" << endl;
        return true;
    } else if (result == ALREADY_PERFORMED_ACTION)
    {
        cout << "Module ALREADY removed" << endl;
        return true;
    }
    else {
        cerr << "Failed to remove module. Error code: " << result << endl;
        return false;
    }
}

// Opens device, gets  file_descriptor
int PWM_DeviceDriver::open_dd(void)
{
    // Open device
    string dev = string(DEVICE_PATH) + "0";
    int fd = open(dev.c_str(), O_RDWR);
    if (fd < 0) {
        std::cerr << "open failed: " << strerror(errno) << "\n";
        return -1;
    }
    file_descriptor = fd;
    return file_descriptor;

}
// Closes device based on file_descriptor
int PWM_DeviceDriver::close_dd(void)
{
    // Check if driver is loaded
    if (file_descriptor >= 0)
    {
        // Close device
        string dev = string(DEVICE_PATH) + "0";
        int fd = close(file_descriptor);
        if (fd < 0) {
            std::cerr << "open failed: " << strerror(errno) << "\n";
            return -1;
        }
        file_descriptor = -1; // No value attributed
        return fd;
    }else
    {
        cerr << "Module not loaded"<< "\n";
        return -1;
    }
}

int PWM_DeviceDriver::setValue(int32_t freq, int duty) const
{
    if (freq < 0 || duty < 0 || duty > 100)
        return -1;
    ioctl_pwm_config_t conf = {freq, duty};
    ioctl (file_descriptor, PWM_SET_CONFIG, &conf);  //update value

    return 0;
}
void PWM_DeviceDriver::disable() const
{
    ioctl_pwm_config_t conf; // empty
    ioctl (file_descriptor, PWM_DISABLE, &conf);
}

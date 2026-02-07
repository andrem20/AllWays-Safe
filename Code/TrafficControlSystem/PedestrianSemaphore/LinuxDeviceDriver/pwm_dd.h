#ifndef TRAFFICCONTROLSYSTEM_PWM_DD_H
#define TRAFFICCONTROLSYSTEM_PWM_DD_H

/*  PWM Linux Device Driver developed targeting RPi4 Model B
 *
 *  All rights reserved to
 *      - André Martins
 *      - Mariana Martins
 */

#include <linux/ioctl.h>

// IOCTL configurations
#define PWM_IOC_MAGIC 'p'

// type
typedef struct {
    int frequency;
    int duty_cycle;
} ioctl_pwm_config_t;

// define operations	 op   distinguish   op_num  expected args
#define PWM_SET_CONFIG  _IOW(PWM_IOC_MAGIC, 1, ioctl_pwm_config_t)
#define PWM_DISABLE     _IO(PWM_IOC_MAGIC, 3)

static long pwm_device_ioctl(struct file *filp, unsigned int cmd, unsigned long arg);

#endif //TRAFFICCONTROLSYSTEM_PWM_DD_H
/*  PWM Linux Device Driver developed targeting RPi4 Model B
*
 *  All rights reserved to
 *      - André Martins
 *      - Mariana Martins
 */
#ifndef __PWM_IOCTL_DD_H__
#define __PWM_IOCTL_DD_H__

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

#endif // __PWM_IOCTL_DD_H__
/*  PWM Linux Device Driver developed targeting RPi4 Model B
 * 
 *  All rights reserved to 
 *      - André Martins
 *      - Mariana Martins 
 */

/* KERNEL INCLUDE SECTION */
#include <linux/init.h>     // init/cleanup macros
#include <linux/module.h>   // metadata macros
#include <linux/fs.h>       // File table Structures: alloc_chrdev_region
#include <linux/cdev.h>     // cdev functions 
#include <linux/io.h>       // I/O mem access; ioremap; __iomem
#include <asm/io.h>
#include <linux/mm.h>
#include <linux/device.h>   // device_create
#include <linux/kernel.h>   // printk 
#include <linux/err.h>      // Kernel Error messages
#include <linux/delay.h>    // usleep
#include <linux/uaccess.h>  // copy data between user space and kernel space

/* USER INCLUDE SECTION */
#include "utility.h"
#include "pwm_dd.h"

/* MACROS */
#define DEVICE_NAME "pwm"
#define CLASS_NAME  "pwmclass"
#define NUM_PINS   1

MODULE_LICENSE("GPL");
MODULE_AUTHOR("AM_MA");
MODULE_DESCRIPTION("PWM device driver");

/* VARIABLE DECLARATION (INITIALIZATION) */
static struct class *pwm_class;
static dev_t DEV_major_minor;
static struct cdev c_dev;

static PWMRegister __iomem *s_PWM;
static CLKRegister  __iomem *s_CLK;
static GPIORegister __iomem *s_GPIO;

/* Declaration and Initialization of the PWM GPIOs*/
static const PWM_GPIO Pins [NUM_PINS] = {GPIO12};

static bool g_class_created = false;
static int  g_devices_created = 0;
static bool g_cdev_added = false;

/* HELPER FUNCTIONS */
int pin_for_minor(int minor) {
    if (minor >= 0 && minor < NUM_PINS)
        return PWMconfig[Pins[minor]].pin;
    return -1;
}

static void mem_cleanup (void){
    if (s_CLK) {
        iounmap(s_CLK);
        s_CLK = NULL;
    }
    if (s_PWM) {
        iounmap(s_PWM);
        s_PWM = NULL;
    }
    if (s_GPIO) {
        iounmap(s_GPIO);
        s_GPIO = NULL;
    }

    if (g_cdev_added)
        cdev_del(&c_dev);
    g_cdev_added = false;

    if (g_devices_created > 0) {
        int i;
        for (i = 0; i < g_devices_created; i++)
            device_destroy(pwm_class, MKDEV(MAJOR(DEV_major_minor), i));
    }
    g_devices_created = 0;
    
    if (g_class_created)
        class_destroy(pwm_class);
    g_class_created = false;

    if (DEV_major_minor)
        unregister_chrdev_region(DEV_major_minor, NUM_PINS);
}

/* FUNCTIONS */

int pwm_device_open(struct inode* p_inode, struct file *p_file)
{
    pr_alert("%s: called\n", __FUNCTION__);
    p_file->private_data = s_PWM;
    return 0;
}

int pwm_device_close(struct inode* p_inode, struct file *p_file)
{
    pr_alert("%s: called\n", __FUNCTION__);
    p_file->private_data = NULL;
    return 0;
}

ssize_t pwm_device_read(struct file *pfile, char __user *p_buff, 
                        size_t len, loff_t *poffset)
{
    pr_alert("%s: called (%zu)\n", __FUNCTION__, len);
    return 0;
}

ssize_t pwm_device_write(struct file *filp,
                         const char __user *buf,
                         size_t len,
                         loff_t *ppos)
{
    char kbuf[24];
    size_t copy_len;
    int frequency = 0, duty_cycle = 0;
    int minor, pin;
    PWM_GPIO gpio_id = -1;
    PWMRegister __iomem *pdev;

    pr_alert("%s: called (%zu)\n", __func__, len);

    if (unlikely(!filp->private_data))
        return -EFAULT;

    /* Map minor -> GPIO pin number */
    minor = iminor(file_inode(filp));
    pin   = pin_for_minor(minor);

    /* Find the gpio_id that matches this pin - get the enum identifier */
    for (int i = 0; i < NUM_PWM_PINS; i++) {
        if (PWMconfig[i].pin == pin) {
            gpio_id = (PWM_GPIO)i;
            break;
        }
    }

    if (gpio_id < 0) {
        pr_err("pwm_device_write: no PWM config for pin %d (minor %d)\n",
               pin, minor);
        return -ENODEV;
    }

    /* Copy and terminate user buffer */
    copy_len = min(len, sizeof(kbuf) - 1);
    if (copy_from_user(kbuf, buf, copy_len))
        return -EFAULT;

    kbuf[copy_len] = '\0';

    /* Expected Input Format: "<frequency> <duty-cycle>" */
    if (sscanf(kbuf, "%d %d", &frequency, &duty_cycle) != 2) {
        pr_alert("Invalid format. Expected: <frequency> <duty_cycle>\n");
        return -EINVAL;
    }

    /* Validate input ranges */
    if (frequency < 0 || frequency > 1000000) {
        pr_alert("Frequency out of range: %d (1-1000000 Hz allowed)\n", frequency);
        return -EINVAL;
    }
    
    if (duty_cycle < 0 || duty_cycle > 100) {
        pr_alert("Duty cycle out of range: %d (0-100%% allowed)\n", duty_cycle);
        return -EINVAL;
    }

    // Temporary pointer MMU safe to map memory area: __iomem
    pdev = (PWMRegister __iomem *)filp->private_data;

    /* Call SetPWM to update the hardware */
    SetPWM(pdev, gpio_id, frequency, duty_cycle, s_CLK);
    
    pr_alert("PWM updated: pin=%d freq=%d duty=%d%%\n", PWMconfig[gpio_id].pin, frequency, duty_cycle);

    return len;
}

static long pwm_device_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
    // Start Validation
    ioctl_pwm_config_t config;
    int minor, pin;
    PWM_GPIO gpio_id = -1;
    PWMRegister __iomem *pdev;

    if (unlikely(!filp->private_data))
        return -EFAULT;

    /* Map minor -> GPIO pin number */
    minor = iminor(file_inode(filp));
    pin   = pin_for_minor(minor);

    /* Find the gpio_id that matches this pin - get the enum identifier */
    for (int i = 0; i < NUM_PWM_PINS; i++) {
        if (PWMconfig[i].pin == pin) {
            gpio_id = (PWM_GPIO)i;
            break;
        }
    }

    if (gpio_id < 0) {
        pr_err("pwm_device_ioctl: no PWM config for pin %d (minor %d)\n",
               pin, minor);
        return -ENODEV;
    }

    // Start ioctl operation
    pdev = (PWMRegister __iomem *)filp->private_data;
    switch (cmd) {
        case PWM_SET_CONFIG:
            if (copy_from_user(&config, (ioctl_pwm_config_t __user *)arg, sizeof(config)))
                return -EFAULT;

            /* Validate input */
            if (config.frequency < 0 || config.frequency > 1000000) {
                pr_alert("Frequency out of range: %d\n", config.frequency);
                return -EINVAL;
            }
            
            if (config.duty_cycle < 0 || config.duty_cycle > 100) {
                pr_alert("Duty cycle out of range: %d\n", config.duty_cycle);
                return -EINVAL;
            }

            /* Apply configuration */
            SetPWM(pdev, gpio_id, config.frequency, config.duty_cycle, s_CLK);
            
            pr_alert("PWM configured: pin=%d freq=%d duty=%d%%\n",
                     pin, config.frequency, config.duty_cycle);
            break;
        case PWM_DISABLE:
            /* Disable PWM by setting frequency to 0 */
            SetPWM(pdev, gpio_id, 0, 0, s_CLK);
            pr_alert("PWM disabled on pin %d\n", pin);
            break;
        default:
            return -EINVAL;
    }
    
    return 0;
}

static struct file_operations pwmDevice_fops = {
    .owner = THIS_MODULE,
    .write = pwm_device_write,			// terminal
    .read = pwm_device_read,
    .unlocked_ioctl = pwm_device_ioctl,		// user-applications
    .release = pwm_device_close,
    .open = pwm_device_open,
};

static int __init pwm_init(void)
{
    int ret;
    struct device *dev_ret;

    pr_alert("PWM Driver Init function called\n");

    /* Allocate device numbers */
    if ((ret = alloc_chrdev_region(&DEV_major_minor, 0, NUM_PINS, DEVICE_NAME)) != 0) {
        printk(KERN_DEBUG "Can't register device\n");
        return ret;
    }

    /* Create class */
    if (IS_ERR(pwm_class = class_create(CLASS_NAME))) {
        unregister_chrdev_region(DEV_major_minor, NUM_PINS);
        return PTR_ERR(pwm_class);
    }
    g_class_created = true;

    /* Create device nodes for each minor */
    for (int i = 0; i < NUM_PINS; i++) {
        char dev_name[16];
        snprintf(dev_name, sizeof(dev_name), "pwm%d", i);
        if (IS_ERR(dev_ret = device_create(pwm_class, NULL, MKDEV(MAJOR(DEV_major_minor), i), NULL, dev_name))) {
            mem_cleanup();
            return PTR_ERR(dev_ret);
        }
        g_devices_created++;
    }

    /* Initialize character device - specify fops */
    cdev_init(&c_dev, &pwmDevice_fops);
    c_dev.owner = THIS_MODULE;

    /* Add character device to system */
    if ((ret = cdev_add(&c_dev, DEV_major_minor, NUM_PINS)) < 0) {
        printk(KERN_NOTICE "Error %d adding device", ret);
        mem_cleanup ();
        return ret;
    }

    g_cdev_added = true;

    /* Map hardware registers */
    s_GPIO  = (GPIORegister *)  ioremap(GPIO_BASE_ADDR, sizeof(GPIORegister));
    s_PWM   = (PWMRegister *)   ioremap(PWM_CONFIG_ADDR, sizeof(PWMRegister));
    s_CLK   = (CLKRegister *)   ioremap(CLK_CONFIG_ADDR, sizeof(CLKRegister));

    if (!s_GPIO || !s_PWM || !s_CLK) {
        pr_err("Failed to map hardware registers\n");
        mem_cleanup();
        return -ENOMEM;
    }

    pr_alert("GPIO Map to virtual address: %p\n", s_GPIO);
    pr_alert("PWM Map to virtual address: %p\n", s_PWM);
    pr_alert("CLK Map to virtual address: %p\n", s_CLK);

    /* Set up PWM clock first */
    init_clk(s_CLK);

    /* Configure GPIO pins for PWM respective alternate function */
    for (int i = 0; i < NUM_PINS; i++)
        SetGPIOFunction(s_GPIO, Pins[i], PWMconfig[Pins[i]].FSELx, s_CLK);

    for (int i = 0; i < NUM_PINS; i++)
        SetPWM(s_PWM, Pins[i], 0, 0, s_CLK);

    /* Initialize with a default PWM setting (1kHz, 50% duty) */
    // SetPWM(s_PWM, Pins[0], 1000, 50, s_CLK);

    /* Check initial PWM state before any user configuration */
    pr_alert("=== INITIAL PWM STATE ===\n");
    pr_alert("CTL:  0x%08x\n", readl(&s_PWM->CTL));
    pr_alert("RNG1: %u\n", readl(&s_PWM->RNG1));
    pr_alert("DAT1: %u\n", readl(&s_PWM->DAT1));
    pr_alert("RNG2: %u\n", readl(&s_PWM->RNG2));
    pr_alert("DAT2: %u\n", readl(&s_PWM->DAT2));

    return 0;
}

static void __exit cleanup(void)
{
    pr_alert("PWM Driver Exit function called\n");
    
    /* Disable PWM by setting pins back to default state (input) */
    for (int i = 0; i < NUM_PINS; i++)
        SetGPIOFunction(s_GPIO, Pins[i], INPUT, s_CLK);

    /* Cleanup resources */
    mem_cleanup ();
    
    pr_alert("PWM Driver cleanup complete\n");
}

module_init(pwm_init);
module_exit(cleanup);

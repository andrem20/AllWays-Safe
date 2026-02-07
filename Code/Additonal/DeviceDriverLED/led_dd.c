/* KERNEL INCLUDE SECTION */
// init/cleanup macros
#include <linux/init.h>
// File table Structures: alloc_chrdev_region
#include <linux/fs.h>       
// cdev functions
#include <linux/cdev.h>     
// I/O mem access; ioremap
#include <linux/io.h>   
#include <asm/io.h>
#include <linux/mm.h>   // Memory management API
// device_create
#include <linux/device.h>  
// printk 
#include <linux/kernel.h>   
// Kernel Error messages
#include <linux/err.h>    

#include "utility.h"

#define DEVICE_NAME "led"          // name of DD
#define CLASS_NAME  "ledclass"
#define NUM_LEDS   2      // number of minor numbers required = number of instances of the devices managed by the driver


/*
module is   licensed under the GNU General Public License
            allowed to access GPL-only kernel symbols
*/
MODULE_LICENSE("GPL");

static struct class *led_class;
static dev_t DEV_major_minor;  // In case of minors >1, this stores Major and base minor 
static struct cdev c_dev;

static GPIORegister* s_GPIO;

static const int Pins [NUM_LEDS] = {20, 16};

/* HELPER FUNCTIONS */

int pin_for_minor(int minor) {
    if (minor >= 0 && minor < NUM_LEDS)
        return Pins[minor];
    return -1; // invalid
}

/* FUNCTIONS */
/* Callback for open command
 *  - p_inode: pointer to the inode of the device file 
 *  - p_file: pointer to the file representing the open instance
 *      p_file->private_data maps to where operations occur
 * 
 *  - Stores a pointer in private_data (only Write Mode on Open - first instance)
 *  to the data strucuture where the memory is mapped. All following file operations
 *  will write on it
 */ 
int led_device_open (struct inode* p_indode, struct file *p_file){
    pr_alert("%s: called\n",__FUNCTION__);
	p_file->private_data = (GPIORegister *) s_GPIO;
	return 0;
}

int led_device_close (struct inode* p_indode, struct file *p_file){
    pr_alert("%s: called\n",__FUNCTION__);
	p_file->private_data = NULL;
	return 0;
}

ssize_t led_device_read(struct file *pfile, char __user *p_buff,size_t len, loff_t *poffset){
	pr_alert("%s: called (%zu)\n",__FUNCTION__,len);
    return 0;
}

ssize_t led_device_write(struct file *pfile, const char __user *buf, size_t len, loff_t *offset){
    

    pr_alert("%s: called (%zu)\n",__FUNCTION__,len);

    // Branch predicting hint macro on Linux Kernel
    if (unlikely(pfile->private_data == NULL)) 
        return -EFAULT;

    // Identify Minor
    int minor = iminor (pfile->f_inode); // get minor from inode
    int pin = pin_for_minor(minor);

    // Get one byte from user space
    char kbuf;
    if (copy_from_user(&kbuf, buf, 1))
        return -EFAULT;

    // "Take control" of the current working registers
    GPIORegister* pdev = (GPIORegister*) pfile->private_data;

    if (kbuf == '0')
        SetGPIOState (pdev, pin, 0);
    else
        SetGPIOState (pdev, pin, 1);
    
    return len;
}

/* Map the file operations <=> entrypoints to the implemented functions here
*   Communication between Device Driver and Kernel
*
*   - The functions that implement those operations must have strict protoypes 
*/
static struct file_operations ledDevice_fops  = {
    .owner = THIS_MODULE,
	.write = led_device_write,
	.read = led_device_read,
	.release = led_device_close,
	.open = led_device_open,
};

/*
 * Init module device driver
 */
static int __init 
    led_init (void){
    
    int ret;
    struct device *dev_ret;

    pr_alert ("Init function called\n");

    /* Search/Scan in the Kernel's global device-number registry
     *      Reserves available major number - bookkeeping inside the kernel
     * - Major number attributed is returned on DEV_structure
     * - First Minor Number value is also returned on DEV_structure
     * - Device name will be the name the inode in VFS
     * 
     * /proc/devices
    */
    if ((ret = alloc_chrdev_region(&DEV_major_minor, 0, NUM_LEDS, DEVICE_NAME)) != 0){
       printk(KERN_DEBUG "Can't register device\n"); return ret;
    }

    /* /sysfs/class entry 
     *  Creates /sysfs/class/<CLASS_NAME> entry
     *   and Enables device nodes creation : /dev/xxx
    */

    if (IS_ERR(led_class = class_create(CLASS_NAME))){
        unregister_chrdev_region (DEV_major_minor, NUM_LEDS);
        return (PTR_ERR(led_class));
    }

    /* Creates instance of the driver- Register a struct device on the Kernel
     *  - /sysfs/class/<CLASS_NAME>/<DEVICE_NAME>
     *  - Triggers a uevent -> udev (Linux device events) in userspace 
     * creates corresponding /dev entry <=> mknod cmd
     *      - /dev/<DEVICE_NAME>
    */
    for (int i = 0; i < NUM_LEDS; i++) {
        char dev_name[16];
        snprintf(dev_name, sizeof(dev_name), "led%d", i);
        if (IS_ERR(dev_ret = device_create(led_class, NULL, MKDEV(MAJOR(DEV_major_minor), i), NULL, dev_name))) {
            class_destroy(led_class);
            for (int j = 0; j < i; j++) 
                device_destroy(led_class, MKDEV(MAJOR(DEV_major_minor), j));
            unregister_chrdev_region (DEV_major_minor, NUM_LEDS);
            return PTR_ERR(dev_ret);
        }
    }
    /* Init character device structure with corresponding file operations
     *  - character device structure: c_dev
    */
    cdev_init(&c_dev, &ledDevice_fops);
    c_dev.owner = THIS_MODULE;

    /* Enables the kernel to know which functions to call on which operation
     *  - Link my cdev to the allocated device numbers - dev_t from alloc_chrdev_region
    */
	if ((ret = cdev_add(&c_dev, DEV_major_minor, NUM_LEDS)) < 0) {
		printk(KERN_NOTICE "Error %d adding device", ret);
        for (int i = 0; i < NUM_LEDS; i++) 
		    device_destroy(led_class, MKDEV(MAJOR(DEV_major_minor), i));
		class_destroy(led_class);
		unregister_chrdev_region(DEV_major_minor, NUM_LEDS);
		return ret;
	}

    /* ioremap: enforce mem protection: given addr is physical, it maps onto 
        the virtual memory addr, since the kernel works with those */
    s_GPIO = (GPIORegister *)ioremap(GPIO_BASE_ADDR, sizeof(GPIORegister));
	pr_alert("Map to virtual address: %p\n", s_GPIO);

    for(int i = 0; i < NUM_LEDS; i++)
        SetGPIOFunction(s_GPIO, Pins[i], OUTPUT);

    return (PTR_ERR(led_class)); // led_class = 0 = SUCCESS and PTR_ERR returns 0
}

static void __exit 
    cleanup (void){

    pr_alert ("Exit function called\n");

    // Set pin to input (default state)
    for(int i = 0; i < NUM_LEDS; i++)
        SetGPIOFunction(s_GPIO, Pins[i], INPUT);
    /* Create/Allocate pair functions: Delete/Remove  */
    iounmap (s_GPIO);
    cdev_del(&c_dev);
    for (int i = 0; i < NUM_LEDS; i++)
        device_destroy(led_class, MKDEV(MAJOR(DEV_major_minor), i));
    class_destroy(led_class);
    unregister_chrdev_region(DEV_major_minor, NUM_LEDS);
}

module_init(led_init);
module_exit(cleanup);

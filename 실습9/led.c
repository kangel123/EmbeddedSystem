#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <asm/io.h>

#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/types.h>
#include <linux/ioport.h>

#define base_lwFPGA     0xFF200000
#define len_lwFPGA      0x200000
#define addr_LED        0
#define LED_DEVMAJOR    239
#define LED_DEVNAME             "led"

static void *mem_base;
static unsigned int *led_addr;

static int led_open(struct inode *minode, struct file *mfile){
        return 0;
}

static int led_release(struct inode *minode, struct file *mfile){
        return 0;
}

static ssize_t led_write (struct file *file, const char __user *buf, size_t count, loff_t *f_pos){
        unsigned int led_data = 0;
        get_user(led_data, (unsigned char *)buf);
        *led_addr=led_data;
        return count;
}

static ssize_t led_read (struct file *file, char __user *buf, size_t count, loff_t *f_pos){
        unsigned int led_data;
        led_data = *led_addr;
        put_user(led_data,buf);
        return 4;
}

static struct file_operations led_fops = {
        .read           =               led_read,
        .write          =               led_write,
        .open           =               led_open,
        .release        =               led_release,
//      .ioctl          =               led_ioctl,
};

static int __init led_init(void){
        int res;
        res=register_chrdev(LED_DEVMAJOR,LED_DEVNAME,&led_fops);
        if(res < 0){
                printk(KERN_ERR " leds : failed to register device.\n");
                return res;
        }
        mem_base = ioremap_nocache(base_lwFPGA,len_lwFPGA);
        if( !mem_base){
                printk("Error mapping memory\n");
                release_mem_region(base_lwFPGA, len_lwFPGA);
                return -EBUSY;
        }
        led_addr = mem_base + addr_LED;

        printk("Device: %s      MAJOR: %d\n",LED_DEVNAME,LED_DEVMAJOR);
        return 0;
}

static void __exit led_exit(void){
        unregister_chrdev(LED_DEVMAJOR,LED_DEVNAME);
        printk(" %s unregisterd.\n",LED_DEVNAME);
        iounmap(mem_base);
}

module_init(led_init);
module_exit(led_exit);
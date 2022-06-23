#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <asm/io.h>

#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/types.h>
#include <linux/ioport.h>
#include <linux/cdev.h>
#include <linux/device.h>

#include <linux/sched.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("JinWoo Kim");
MODULE_DESCRIPTION("A Calculator");

void init_add_timer(void);
void remove_timer(void);
void calculator_timer_function(unsigned long ptr);

#define base_lwFPGA 0xFF200000
#define len_lwFPGA 0x200000

#define addr_LED 0
#define addr_HEX0 0x20
#define addr_HEX1 0x30
#define add_SW  0x40
#define addr_KEY        0x50

#define offset_INTMASK 0x08
#define offset_EDGE 0x0C

static void* mem_base;
static void* key_addr;
static void* hex0_addr;   //hex3-hex0
static void* hex1_addr;   //hex5-hex4
static unsigned int data = -1;

static unsigned int mode = 0;
#define NOFILL 4
#define BLINK 8

unsigned int hex0, hex1;

int hex_conversion[16] = {
        0x3F, 0x06, 0x5B, 0x4F, 0x66, 0x6D, 0x7D, 0x07,
         0x7F, 0x67, 0x77, 0x7C, 0x39, 0x5E, 0x79, 0x71
};

static DECLARE_WAIT_QUEUE_HEAD(key_queue);
static int flag = 0;
unsigned int key_data;

//write
static ssize_t calculator_write(struct file *file, const char __user *buf, size_t count, loff_t *f_pos){
	unsigned int hex_data = 0;
	unsigned int nofill = 0;
	
	get_user(hex_data, (unsigned int *)buf);
	
	hex_data =hex_data & 0xFFFFFF;
	data = hex_data;
	
	if(mode & NOFILL) nofill = 1;
	
	hex1 = 0;
	hex0 = hex_conversion[hex_data&0xF];
	
	do{
	hex_data >>= 4;
	if(nofill && hex_data==0) break;
	hex0 |= hex_conversion[hex_data&0xF]<<8;
	
	hex_data >>= 4;
	if(nofill && hex_data==0) break;
	hex0 |= hex_conversion[hex_data&0xF]<<16;
	
	hex_data >>= 4;
	if(nofill && hex_data==0) break;
	hex0 |= hex_conversion[hex_data&0xF]<<24;
	
	hex_data >>= 4;
	if(nofill && hex_data==0) break;
	hex0 |= hex_conversion[hex_data&0xF];
	
	hex_data >>= 4;
	if(nofill && hex_data==0) break;
	hex1 |= hex_conversion[hex_data&0xF]<<8;
	}while(0);
	
	iowrite32(hex0 ,hex0_addr);
	iowrite32(hex1, hex1_addr);
	
	return 4;
}


//read
static ssize_t calculator_read(struct file *file, char __user *buf, size_t count, loff_t *f_pos){
	wait_event_interruptible(key_queue, flag !=0);
	flag = 0;
	put_user(key_data, buf);
	return 4;
}

irq_handler_t irq_handler(int irq, void *dev_id, struct pt_regs *regs){
	key_data = ioread32(key_addr + offset_EDGE);
	iowrite32(0xf, key_addr + offset_EDGE);

	flag =1 ;
	wake_up_interruptible(&key_queue);

	return (irq_handler_t) IRQ_HANDLED;
}

static int calculator_open(struct inode *minode, struct file *mfile){
        return 0;
}

static int calculator_release(struct inode *minode, struct file *mfile){
        return 0;
}


static struct file_operations calculator_fops = {
        .read   =       calculator_read,
        .write  =       calculator_write,
        .open   =       calculator_open,
        .release        =       calculator_release,
};

static struct cdev calculator_cdev;
static struct class *cl;
static dev_t dev_no;

#define DEVICE_NAME "calculator"

static int __init calculator_init(void){
        //allocate char device
        if(alloc_chrdev_region(&dev_no, 0, 1, DEVICE_NAME) < 0){
                printk(KERN_ERR "alloc_chrdev_region() error\n");
                return -1;
        }
        //init cdev
        cdev_init(&calculator_cdev, &calculator_fops);

        //add_cdev
        if(cdev_add(&calculator_cdev, dev_no, 1) < 0){
                printk(KERN_ERR "cdev_add() error\n");
                goto unreg_chrdev;
        }
        //create device
        cl = class_create (THIS_MODULE, DEVICE_NAME);
        if (cl==NULL){
                printk(KERN_ALERT "class_create() error\n");
                goto unreg_chrdev;
        }
        //create device
        if(device_create(cl, NULL, dev_no, NULL, DEVICE_NAME) == NULL){
                printk (KERN_ALERT "device_create() error\n");
                goto unreg_class;
        }

        mem_base = ioremap_nocache(base_lwFPGA, len_lwFPGA);
        if(mem_base == NULL) {
                printk(KERN_ERR "ioremap_nocache() error\n");
                goto un_device;
        }
      	hex0_addr = mem_base + addr_HEX0;
       	hex1_addr = mem_base + addr_HEX1;

	key_addr = mem_base + addr_KEY;
	iowrite32(0xf, key_addr + offset_EDGE);
	iowrite32(0xf, key_addr + offset_INTMASK);

 	printk("Device : %s / Major : %d %x\n", DEVICE_NAME, MAJOR(dev_no), dev_no);
	

        return request_irq(73, (irq_handler_t)irq_handler, IRQF_SHARED, "key_irq_handler", (void *) (irq_handler));

        un_device:
                device_destroy(cl, dev_no);
        unreg_class:
                class_destroy(cl);
        unreg_chrdev:
                unregister_chrdev_region(dev_no, 1);
                return -1;

}

static void __exit calculator_exit(void){
        iowrite32(0, hex0_addr);
        iowrite32(0, hex1_addr);
	
	free_irq(73, (void*) irq_handler);
        iounmap(mem_base);
        device_destroy(cl, dev_no);
        class_destroy(cl);
        unregister_chrdev_region(dev_no, 1);
        printk(" %s unregisterd.\n", DEVICE_NAME);
}

module_init(calculator_init);
module_exit(calculator_exit);

static int turnoff = 0;
static struct timer_list calculator_timer;

void init_add_timer(void){
        init_timer(&calculator_timer);
        calculator_timer.function = calculator_timer_function;
        calculator_timer.expires = jiffies + HZ;
        calculator_timer.data = 0;

        add_timer(&calculator_timer);
}

void remove_timer(void){
        del_timer(&calculator_timer);
}

void calculator_timer_function(unsigned long ptr){
        if ( !(mode & BLINK) ) return;
        turnoff = !turnoff;
        if(turnoff){
                iowrite32(0, hex0_addr);
                iowrite32(0, hex1_addr);
        } else{
                iowrite32(hex0, hex0_addr);
                iowrite32(hex1, hex1_addr);
        }

        init_add_timer();
}

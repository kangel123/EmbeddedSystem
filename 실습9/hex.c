#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <asm/io.h>

#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/types.h>
#include <linux/ioport.h>

#define base_lwFPGA 0xFF200000
#define len_lwFPGA 0x200000

#define addr_LED 0
#define addr_HEX0 0x20
#define addr_HEX1 0x30

#define HEX_DEVMAJOR 240
#define HEX_DEVNAME "hex"

static void *mem_base;
static void *hex0_addr;   //hex3-hex0
static void *hex1_addr;   //hex5-hex4
static unsigned int data = -1;

static unsigned int mode = 0;
#define NOFILL 4
#define BLINK 8

unsigned int hex0, hex1;
static int turnoff = 0;

int hex_conversion[16] = {
  0x3F, 0x06, 0x5B, 0x4F, 0x66, 0x6D, 0x7D, 0x07,
  0x7F, 0x67, 0x77, 0x7C, 0x39, 0x5E, 0x79, 0x71
};

static int hex_open(struct inode *minode, struct file *mfile){return 0;}
static int hex_release(struct inode *minode, struct file *mfile){return 0;}

//write
static ssize_t hex_write(struct file *mfile, const char __user *buf, size_t count, loff_t *f_pos){
  unsigned int hex_data = 0;
  unsigned int nofill = 0;

  get_user(hex_data, (unsigned int *)buf);

  hex_data = hex_data & 0xFFFFFF;
  data = hex_data;

  if(mode & NOFILL) nofill = 1;

  hex1 = 0;
  hex0 = hex_conversion[hex_data&0xF];

  do{
    hex_data >>= 4;
    if(nofill && hex_data==0) break;
    hex0 |= hex_conversion[hex_data&0xF]<<8;

    hex_data >>=4;
    if(nofill && hex_data==0) break;
    hex0 |= hex_conversion[hex_data&0xF]<<16;

    hex_data >>=4;
    if(nofill && hex_data==0) break;
    hex0 |= hex_conversion[hex_data&0xF]<<24;

    hex_data >>=4;
    if(nofill && hex_data==0) break;
    hex1 |= hex_conversion[hex_data&0xF];

    hex_data >>=4;
    if(nofill && hex_data==0) break;
    hex1 |= hex_conversion[hex_data&0xF]<<8;
  }while(0);

  iowrite32(hex0, hex0_addr);
  iowrite32(hex1, hex1_addr);
  return 4;
}

//read
static ssize_t hex_read(struct file *mfile, const char __user *buf, size_t count, loff_t *f_pos){
  put_user(data, (unsigned int*)buf);
  return 4;
}

static struct file_operations hex_fops = {
  .read = hex_read,
  .write = hex_write,
  .open = hex_open,
  .release = hex_release
};

static int __init hex_init(void){
  int res;

  res = register_chrdev(HEX_DEVMAJOR, HEX_DEVNAME, &hex_fops);
  if(res<0){
    printk(KERN_ERR " HEX : failed to register device. \n");
    return res;
  }
  mem_base = ioremap_nocache(base_lwFPGA, len_lwFPGA);
  if(!mem_base){
    printk("Error mapping memory.\n");
    release_mem_region(base_lwFPGA, len_lwFPGA);
    return -EBUSY;
  }

  printk("Device : %s / Major : %d \n", HEX_DEVNAME, HEX_DEVMAJOR);

  hex0_addr = mem_base + addr_HEX0;
  hex1_addr = mem_base + addr_HEX1;
  return 0;
}

static void __exit hex_exit(void){
  unregister_chrdev(HEX_DEVMAJOR, HEX_DEVNAME);
  printk(" %s unregistered. \n", HEX_DEVNAME);

  iowrite32(0, hex0_addr);
  iowrite32(0, hex1_addr);

  iounmap(mem_base);
}

module_init(hex_init);
module_exit(hex_exit);
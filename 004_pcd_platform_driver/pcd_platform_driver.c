/*
  TODO : As of now, the driver supports only a single devices
        make it more dynamic to be supported by other devices
*/

#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>

#define DEV1_MEM_SIZE 512
#define DEV1_MEM_SIZE 1024

char pcdev_1[DEV1_MEM_SIZE];
char pcdev_2[DEV1_MEM_SIZE];



dev_t device_number1;

/*
  TODO: MAKE THESE FUNCTIONS MORE DYNAMIC
*/

int pcd_open(struct inode *inode, struct file *filp){

    return 0;
}


loff_t pcd_lseek(struct file *filp, loff_t offset, int whence){

    switch (whence) {
      case SEEK_SET:
                  //file_offset = offset
                  filp->f_pos = offset;
          break;
      case SEEK_CUR:
                //Current location + offset
                filp->f_pos += offset;
          break;
      case SEEK_END:
                //Size of file + offset bytes

                filp->f_pos = DEV1_MEM_SIZE + offset;
          break;
      default:
                return -EINVAL;
    }

  return filp->f_pos;
}

ssize_t pcd_read(struct file *filp, char __user *buffer, size_t count, loff_t *f_pos){

  return 0;
}

ssize_t pcd_write(struct file *filp, const char __user *buffer, size_t count, loff_t *f_pos){

  return 0;
}

int pcd_release(struct inode *inode, struct file *filp){

  return 0;
}


struct file_operations pcd_fops = {
          .open = pcd_open,
          .read = pcd_read,
          .write = pcd_write,
          .release = pcd_release,
          .owner = THIS_MODULE
};


struct cdev pcdev1;
struct cdev pcdev2;

static int __init pcd_driver_init(void){

  alloc_chrdev_region(&device_number,0,2,"pseudo-char-device");

  cdev_init(&pcdev1, &pcd_fops);
  cdev_init(&pcdev2, &pcd_fops);

  pcdev_1.owner = THIS_MODULE;
  pcdev_2.owner = THIS_MODULE;
  cdev_add(&pcdev_1, &pcd_fops);
  cdev_add(&pcdev_2, &pcd_fops);

  //TODO: IMPLEMENT CLASS CREATE AND DEVICE CREATE

  return 0;

}

static void __exit pcd_driver_exit(void){


  pr_info("Module Unloaded");
}

module_init(pcd_driver_init);
module_exit(pcd_driver_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Yuvraj");
MODULE_DESCRIPTION("Platform PCD Driver");

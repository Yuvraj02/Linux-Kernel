/*
  TODO : As of now, the driver supports only a single devices
        make it more dynamic to be supported by other devices
*/

#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>

#define DEV1_MEM_SIZE 512
#define DEV2_MEM_SIZE 1024

char pdev_1[DEV1_MEM_SIZE];
dev_t device_number;

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

          pr_info("Read Requested for %zu bytes\n", count);
          pr_info("Initial f_pos value : %lld\n", *f_pos);

          if(*f_pos+count > DEV1_MEM_SIZE){
              count = DEV1_MEM_SIZE - *f_pos;
            /*
            Copy "count" number of bytes from device memory to user buffer
            */            
           if(copy_to_user(&buffer, &pdev_1+(*f_pos), count)){
                    return -EFAULT;
           }
          }
          //Update the f_pos pointer count
          *f_pos += count;

          pr_info("Number of bytes successfully read : %zu\n",count);
          pr_info("Updated f_pos value/position : %lld\n", *f_pos);

  return count;
}

ssize_t pcd_write(struct file *filp, const char __user *buffer, size_t count, loff_t *f_pos){

        /*
        Here we did (*f_pos+count) because we don't
        want to read every time from the start
        */

        pr_info("Number of bytes to write : %zu\n", count);
        pr_info("Initial f_pos value : %lld\n", *f_pos);

        if(*f_pos+count > DEV1_MEM_SIZE){
          count = DEV1_MEM_SIZE - *f_pos;
        }

        if(!count){
            pr_info("No Memory Left to be read on the device\n");
          return -ENOMEM;
        }
          
        if(copy_from_user(&pdev_1+(*f_pos), &buffer, count)){
          return -EFAULT;
        }

        f_pos += count;

        pr_info("Number of bytes successfully written : %zu\n",count);
        pr_info("Updated f_pos value : %lld\n",*f_pos);


  return 0;
}

int pcd_release(struct inode *inode, struct file *filp){
    pr_info("Device Closed successfullt\n");
  return 0;
}


struct file_operations pcd_fops = {
          .open = pcd_open,
          .read = pcd_read,
          .write = pcd_write,
          .release = pcd_release,
          .owner = THIS_MODULE
};

/*cdev structures for pseudo character devices*/
struct cdev pcdev1;


static int __init pcd_driver_init(void){

  pr_info("Module Loaded\n");

  alloc_chrdev_region(&device_number,0,1,"pseudo-char-device");

  cdev_init(&pcdev1, &pcd_fops);

  /*cdev structure has a owner field which has to be initialized
    to THIS_MODULE to indicate which module this device or structure
    belongs to, we initialize them after init, because init
    will blank out owner field
    */
  pcdev1.owner = THIS_MODULE;


  /*We add the add device with a cdev structure and device number in
    arguements but initialize with cdev structure and file
    operations*/
  cdev_add(&pcdev1, device_number,1);
  
  //TODO: IMPLEMENT CLASS CREATE AND DEVICE CREATE

  struct class *pcd_class = class_create(THIS_MODULE,"pcd_class");
  device_create(pcd_class,NULL, device_number,NULL,"pcdev-0");
  //This is the last step

  return 0;

}

static void __exit pcd_driver_exit(void){

  pr_info("Module Unloaded\n");
}

module_init(pcd_driver_init);
module_exit(pcd_driver_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Yuvraj");
MODULE_DESCRIPTION("Platform PCD Driver");

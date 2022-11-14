/*
  TODO : As of now, the driver supports only a single devices
        make it more dynamic to be supported by other devices
*/
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>

#define DEV1_MEM_SIZE 512

#define RDWR 0x11

char pdev_1[DEV1_MEM_SIZE];
int device_perm = RDWR;


dev_t device_number;
/*
  TODO: MAKE THESE FUNCTIONS MORE DYNAMIC
*/

int check_permission(int perm, int access){

    if(perm==RDWR)
      return 0;
return -EPERM;
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
           if(copy_to_user(buffer, &pdev_1[*f_pos], count)){
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

        if((*f_pos+count) > DEV1_MEM_SIZE){
          count = DEV1_MEM_SIZE - *f_pos;
        }

        if(!count){
            pr_info("No Memory Left to be read on the device\n");
          return -ENOMEM;
        }
          
        if(copy_from_user(&pdev_1[*f_pos], buffer, count)){
          return -EFAULT;
        }

        *f_pos += count;

        pr_info("Number of bytes successfully written : %zu\n",count);
        pr_info("Updated f_pos value : %lld\n",*f_pos);


  return count;
}

int pcd_open(struct inode *inode, struct file *filp){

    //   int ret,minor_num;
    //   minor_num = MINOR(inode->i_rdev);

    //   pr_info("Minor access = %d\n", minor_num);

    //   ret = check_permission(device_perm,filp->f_mode);

    //   (!ret) ? pr_info("Open Successfull\n") : pr_info("Open Failed\n"); 

    // return ret;
    pr_info("Open Successfull\n");
    return 0;
}

int pcd_release(struct inode *inode, struct file *filp){
    pr_info("Device Closed successfully\n");
  return 0;
}


struct file_operations pcd_fops = {
          .open = pcd_open,
          .read = pcd_read,
          .llseek = pcd_lseek,
          .write = pcd_write,
          .release = pcd_release,
          .owner = THIS_MODULE
};

/*cdev structures for pseudo character devices*/
struct cdev pcd_cdev;
struct class *pcd_class;
struct device *device_pcd;

static int __init pcd_driver_init(void){

  int ret;
 
  ret = alloc_chrdev_region(&device_number,0,1,"pcd_devices");
    if(ret<0){
        pr_info("Device Allocation Failed\n");
        goto out;
    }
  cdev_init(&pcd_cdev, &pcd_fops);

  /*cdev structure has a owner field which has to be initialized
    to THIS_MODULE to indicate which module this device or structure
    belongs to, we initialize them after init, because init
    will blank out owner field
    */
  pcd_cdev.owner = THIS_MODULE;


  /*We add the add device with a cdev structure and device number in
    arguements but initialize with cdev structure and file
    operations*/
  ret = cdev_add(&pcd_cdev, device_number,1);
    if(ret < 0){
        pr_info("Device Registration failed (cdev_add err)\n");
        goto unregister_chrdev;
    }
  //TODO: IMPLEMENT CLASS CREATE AND DEVICE CREATE

  pcd_class = class_create(THIS_MODULE,"pcd_class");

    if(IS_ERR(pcd_class)){
        pr_info("Class Creation Failed\n");
        ret = PTR_ERR(pcd_class);
        goto cdev_del;
    }

  device_pcd = device_create(pcd_class,NULL, device_number,NULL,"pcdev");
    if(IS_ERR(device_pcd)){
      pr_info("Device Creation Failed\n");
      ret = PTR_ERR(device_pcd);
      goto class_destroy;
    }
  //This is the last step

   pr_info("Module Init Successfull\n");
  return 0;

class_destroy:
          class_destroy(pcd_class);
cdev_del:
          cdev_del(&pcd_cdev);
unregister_chrdev:
          unregister_chrdev_region(device_number,1);
out:
    return ret;

}

static void __exit pcd_driver_exit(void){

    device_destroy(pcd_class, device_number);

    class_destroy(pcd_class);

    cdev_del(&pcd_cdev);

    unregister_chrdev_region(device_number, 1);

  pr_info("Module Unloaded\n");
}

module_init(pcd_driver_init);
module_exit(pcd_driver_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Yuvraj");
MODULE_DESCRIPTION("Platform PCD Driver");

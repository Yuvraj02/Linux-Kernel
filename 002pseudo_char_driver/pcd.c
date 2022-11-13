#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/kdev_t.h>
#include <linux/uaccess.h>
#define DEV_MEM_SIZE 512

#undef pr_fmt
#define pr_fmt(fmt) "%s:" fmt,__func__
//pseudo device memory
char device_buffer[DEV_MEM_SIZE];

//This holds a device number
dev_t device_number;
//Cdev variable
struct cdev pcd_cdev;

loff_t pcd_lseek (struct file *filp, loff_t offset, int whence){
	pr_info("lseek requested \n");

	loff_t temp;

	pr_info("Current value of file position : %lld", filp->f_pos);

	switch (whence) {
		case SEEK_SET:
							if((offset>DEV_MEM_SIZE) || (offset<0))
										return -EINVAL;
							filp->f_pos=offset;
							break;
		case SEEK_CUR:
							temp = filp->f_pos+offset;
							if((temp>DEV_MEM_SIZE) || (temp < 0))
									return -EINVAL;
							filp->f_pos = temp;
							break;
		case SEEK_END:
							temp = DEV_MEM_SIZE+offset;
							if((temp>DEV_MEM_SIZE) || (temp < 0))
									return -EINVAL;
							filp->f_pos = temp;
							break;
		default:
				return -EINVAL;
	}

	pr_info("New value of file position : %lld", filp->f_pos);

	return filp->f_pos;
}

ssize_t pcd_read (struct file *filp, char __user *buff, size_t count, loff_t *f_pos){
	pr_info("read requested for %zu bytes \n",count);
	pr_info("Current f_pos value = %lld\n", *f_pos);

	//1. Adjust the count
		if((*f_pos + count)>DEV_MEM_SIZE){
				count = DEV_MEM_SIZE - *f_pos;
		}
	//2. Copy to __user
		if(copy_to_user(buff,&device_buffer[*f_pos],count)){
			return -EFAULT;
		}
	/*3. Update the current file position*/
	*f_pos += count;

	pr_info("Number of bytes sucessfully read = %zu\n", count);
	pr_info("Updatedf_pos value = %lld\n", *f_pos);
	/*Return number of bytes sucessfully read*/
	return count;
}

ssize_t pcd_write (struct file *filp, const char __user *buff, size_t count, loff_t *f_pos){
	pr_info("wrtie requested for %zu bytes",count);
	pr_info("Current file position : %lld", *f_pos);
	/*1. Adjust the count*/
			if((*f_pos+count) > DEV_MEM_SIZE){
				count = DEV_MEM_SIZE - *f_pos;
			}

	/*2. Copy from user*/
	if(!count){
		pr_err("No space left on the device");
		return -ENOMEM;
	}

		if(copy_from_user(&device_buffer[*f_pos],buff, count)){
			return -EFAULT;
		}
	/*3. Update the current file*/
		*f_pos += count;

		pr_info("Number of bytes written : %zu\n",count);
		pr_info("Update file position : %lld\n",*f_pos);

	/*4. Return error (if the user cannot write to file, return
				Appropriate Error Message */
	return count;
}

int pcd_open (struct inode *inode, struct file *filp){
	pr_info("open was successfull");
	return 0;
}

int pcd_release (struct inode *inode, struct file *filp){
	pr_info("close was successfull");
	return 0;
}

//File operations of driver
struct file_operations pcd_fops = { .open = pcd_open,
                                    .write= pcd_write,
                                    .read = pcd_read,
                                    .llseek = pcd_lseek,
                                    .release = pcd_release,
                                    .owner = THIS_MODULE}; //Owner field is important

struct class *class_pcd;
struct device *device_pcd;

static int __init pcd_driver_init(void){
  /* 1. Dynamically allocate a device number
				This can also return an error, on successfull
				allocation, it returns 0. */
	int ret;
  ret = alloc_chrdev_region(&device_number, 0,1,"pcd_devices");
	if(ret<0){
		goto out;
	}
  // 2. Initialize the cdev structure with fops
	pr_info("Device number <major>:<minor> = %d:%d",MAJOR(device_number),MINOR(device_number));
  cdev_init(&pcd_cdev,&pcd_fops);
  /*3. Register the device (cdev structure) with VFS
        Our Device is nothing but a cdev structure, owner field
				will be blanked out when cdev_init executes, that's
				why we Initialized owner field after cdev_init*/
  pcd_cdev.owner = THIS_MODULE;
  ret = cdev_add(&pcd_cdev,device_number,1);
		if(ret<0)
				goto unregister_chrdev;
	//4. Create device class under /sys/class/
	class_pcd = class_create(THIS_MODULE,"pcd_class");
	if(IS_ERR(class_pcd)){
			pr_info("Class Creation failed\n");
			ret = PTR_ERR(class_pcd);
			goto cdev_del;
	}
	/* Popuplate the sysf with device information
		 or we can say device file creation
	*/
	device_pcd = device_create(class_pcd,NULL,device_number,NULL,"pcd");
	if(IS_ERR(device_pcd)){
		pr_info("Device Creation failed\n");
		ret = PTR_ERR(device_pcd);
		goto class_destroy;
	}

	pr_info("Module init was successfull");

return 0;

class_destroy:
		class_destroy(class_pcd);
cdev_del:
		cdev_del(&pcd_cdev);
unregister_chrdev:
		unregister_chrdev_region(device_number,1);
out:
		return ret;
}

static void __exit pcd_driver_exit(void){

	//1. Destory Device created
	device_destroy(class_pcd,device_number);

	//2. Destroy class created
	class_destroy(class_pcd);

	//3.Remove cdev registration from the Kernel VFS
	cdev_del(&pcd_cdev);

	//4. Unallocate/Unregister range of device numbers
	unregister_chrdev_region(device_number,1);

	pr_info("module unloaded\n");

}

module_init(pcd_driver_init);
module_exit(pcd_driver_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Yuvraj");
MODULE_DESCRIPTION("A Pseudo Device Driver");

#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/kdev_t.h>
#include <linux/uaccess.h>

#define NO_OF_DEVICES 4
#define MEM_SIZE_MAX_PCDEV1 1024
#define MEM_SIZE_MAX_PCDEV2 512
#define MEM_SIZE_MAX_PCDEV3 1024
#define MEM_SIZE_MAX_PCDEV4 512

#undef pr_fmt
#define pr_fmt(fmt) "%s:" fmt,__func__


/* Creating  4 pseudo devices memory */
char device_buffer_pcdev1[MEM_SIZE_MAX_PCDEV1];
char device_buffer_pcdev2[MEM_SIZE_MAX_PCDEV2];
char device_buffer_pcdev3[MEM_SIZE_MAX_PCDEV3];
char device_buffer_pcdev4[MEM_SIZE_MAX_PCDEV4];

/*struct for device's private data */

struct pcdev_private_data{
	char *buffer;
	unsigned size;
	int perm;
	char *serial_number;
	//Cdev variable
	struct cdev cdev;
};

/*structure for driver's private data (pcdev_private_data) */

struct pcdrv_private_data{
	int total_devices;
	/*This holds a device number
	Driver maintains a device number*/
	dev_t device_number;
	struct class *class_pcd;
	struct device *device_pcd;

	struct pcdev_private_data pcdev_data[NO_OF_DEVICES];


};


#define RDONLY 0x01
#define WRONLY 0x10
#define RDWR 	 0x11


struct pcdrv_private_data pcdrv_data = {
					.total_devices = NO_OF_DEVICES,
					.pcdev_data = {
							[0]={
							.buffer = device_buffer_pcdev1,
							.size = MEM_SIZE_MAX_PCDEV1,
							.serial_number = "PCDEV1XYZ123",
							.perm = RDONLY /*RDONLY*/
						},
						  [1]={
								.buffer = device_buffer_pcdev2,
								.size = MEM_SIZE_MAX_PCDEV2,
								.serial_number = "PCDEV2XYZ124",
								.perm = WRONLY /* WRONLY */
						},
							[2]={
								.buffer = device_buffer_pcdev3,
								.size = MEM_SIZE_MAX_PCDEV3,
								.serial_number = "PCDEV3XYZ125",
								.perm = RDWR /*RDRW*/
						},
							[3]={
							 .buffer = device_buffer_pcdev4,
							 .size = MEM_SIZE_MAX_PCDEV4,
							 .serial_number = "PCDEV4XYZ126",
							 .perm = 0x11 /*RDWR*/
						},
				}
};


loff_t pcd_lseek (struct file *filp, loff_t offset, int whence){

	struct pcdev_private_data *pcdev_data = (struct pcdev_private_data*)filp->private_data;
	int max = pcdev_data->size;
	pr_info("lseek requested \n");

	/*To store value of offset */

	loff_t temp;

	pr_info("Current value of file position : %lld", filp->f_pos);

	switch (whence) {
		case SEEK_SET:
							if((offset>max) || (offset<0))
										return -EINVAL;
							filp->f_pos=offset;
							break;
		case SEEK_CUR:
							temp = filp->f_pos+offset;
							if((temp>max) || (temp < 0))
									return -EINVAL;
							filp->f_pos = temp;
							break;
		case SEEK_END:
							temp = max+offset;
							if((temp>max) || (temp < 0))
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

/*filp->private data returns a void pointer that's why we need to cast it to the appropriate struct*/
	struct pcdev_private_data *pcdev_data = (struct pcdev_private_data*)filp->private_data;
	int max_size = pcdev_data->size;
	pr_info("read requested for %zu bytes \n",count);
	pr_info("Current f_pos value = %lld\n", *f_pos);

	//1. Adjust the count
		if((*f_pos + count)>max_size){
				count = max_size - *f_pos;
		}
	//2. Copy to __user
		if(copy_to_user(buff,&pcdev_data->buffer+(*f_pos),count)){
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

	/*filp->private_data returns a void pointer, that's why we need to cast it to appropriate struct*/
	struct pcdev_private_data *pcdev_data = (struct pcdev_private_data*)filp->private_data;

	int max_size = pcdev_data->size;

	pr_info("wrtie requested for %zu bytes",count);
	pr_info("Current file position : %lld", *f_pos);
	/*1. Adjust the count*/
			if((*f_pos+count) > max_size){
				count = max_size - *f_pos;
			}

	/*2. Copy from user*/
	if(!count){
		pr_err("No space left on the device");
		return -ENOMEM;
	}

		if(copy_from_user(&pcdev_data->buffer+(*f_pos),buff, count)){
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


int check_permission(int dev_perm,int acc_mode){

	if(dev_perm==RDWR)
		return 0;

		/*The device is READONLY*/
	if((dev_perm == RDONLY) && ((acc_mode & FMODE_READ) && !(acc_mode & FMODE_WRITE)))
		return 0;

	if((dev_perm == WRONLY) && ((acc_mode & FMODE_WRITE) && !(acc_mode & FMODE_READ)))
		return 0;

	return -EPERM;
}

int pcd_open (struct inode *inode, struct file *filp){
	int ret;
	int minor_no;
	struct pcdev_private_data *pcdev_data;

	minor_no = MINOR(inode->i_rdev);

	pr_info("minor access = %d\n", minor_no);

/*We want to get the struct of the opened device(device specific cdev), to get it, we use container_of
 	using inode->i_cdev we get the device specific cdev structure*/
	pcdev_data = container_of(inode->i_cdev,struct pcdev_private_data,cdev);

/*To supply device private data to other methods of the driver */
	filp->private_data = pcdev_data;

	ret = check_permission(pcdev_data->perm,filp->f_mode);
	/* If false(0), then Open was successfull else open was failed*/
	(!ret)?pr_info("Open was sucessufll\n"):pr_info("Open was failed");

	return ret;
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


static int __init pcd_driver_init(void){
  /* 1. Dynamically allocate a device number
				This can also return an error, on successfull
				allocation, it returns 0. */
	int ret;
	int i;
  ret = alloc_chrdev_region(&pcdrv_data.device_number, 0,NO_OF_DEVICES,"pcd_devices");
	if(ret<0){
		goto out;
	}

		//4. Create device class under /sys/class/
		pcdrv_data.class_pcd = class_create(THIS_MODULE,"pcd_class");
		if(IS_ERR(pcdrv_data.class_pcd)){
				pr_info("Class Creation failed\n");
				ret = PTR_ERR(pcdrv_data.class_pcd);
				goto unregister_chrdev;
		}


	for (i = 0; i < NO_OF_DEVICES; i++) {
		pr_info("Device number %d <major>:<minor> = %d:%d",i,MAJOR(pcdrv_data.device_number+i),MINOR(pcdrv_data.device_number+i));
		  /* 2. Initialize the cdev structure with fops*/
		cdev_init(&pcdrv_data.pcdev_data[i].cdev, &pcd_fops);

	//---------LEGACY FUNCTION->	cdev_init(&pcd_cdev,&pcd_fops);

		/*3. Register the device (cdev structure) with VFS
	        Our Device is nothing but a cdev structure, owner field
					will be blanked out when cdev_init executes, that's
					why we Initialized owner field after cdev_init*/
	  pcdrv_data.pcdev_data[i].cdev.owner = THIS_MODULE;
	  ret = cdev_add(&pcdrv_data.pcdev_data[i].cdev,pcdrv_data.device_number+i,1);
			if(ret<0)
					goto cdev_del;

			pcdrv_data.device_pcd = device_create(pcdrv_data.class_pcd,NULL,pcdrv_data.device_number+i,NULL,"pcdev-%d",i);
			if(IS_ERR(pcdrv_data.device_pcd)){
						pr_info("Device Creation failed\n");
						ret = PTR_ERR(pcdrv_data.device_pcd);
						goto class_destroy;
			}
	}
	/*	pr_info("Device number <major>:<minor> = %d:%d",MAJOR(device_number),MINOR(device_number));*/

		/* Popuplate the sysf with device information
			 or we can say device file creation
		*/


	pr_info("Module init was successfull");

return 0;

cdev_del:
	/*	cdev_del(&pcdrv_data.pcdev_data[i].cdev); */
class_destroy:
		while(i>=0){
			device_destroy(pcdrv_data.class_pcd,pcdrv_data.device_number+i);
			cdev_del(&pcdrv_data.pcdev_data[i].cdev);
			i--;
		}
		class_destroy(pcdrv_data.class_pcd);
unregister_chrdev:
		unregister_chrdev_region(pcdrv_data.device_number,NO_OF_DEVICES);
out:
		pr_info("Module insertion failed\n");
		return ret;
}

static void __exit pcd_driver_exit(void){

	int i;
	for (i = 0; i < NO_OF_DEVICES; i++) {
		device_destroy(pcdrv_data.class_pcd,pcdrv_data.device_number+i);
		cdev_del(&pcdrv_data.pcdev_data[i].cdev);
	}

	class_destroy(pcdrv_data.class_pcd);

		unregister_chrdev_region(pcdrv_data.device_number,NO_OF_DEVICES);
	// //1. Destory Device created
	// device_destroy(class_pcd,device_number);
	//
	// //2. Destroy class created
	// class_destroy(class_pcd);
	//
	// //3.Remove cdev registration from the Kernel VFS
	// cdev_del(&pcd_cdev);
	//
	// //4. Unallocate/Unregister range of device numbers
	// unregister_chrdev_region(device_number,1);
	//
	 pr_info("module unloaded\n");

}

module_init(pcd_driver_init);
module_exit(pcd_driver_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Yuvraj");
MODULE_DESCRIPTION("A Pseudo Device Driver which handles n devices");

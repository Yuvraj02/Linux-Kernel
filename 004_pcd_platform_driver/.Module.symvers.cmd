cmd_/home/reckon/workspace/ldd/custom_drivers/004_pcd_platform_driver/Module.symvers := sed 's/\.ko$$/\.o/' /home/reckon/workspace/ldd/custom_drivers/004_pcd_platform_driver/modules.order | scripts/mod/modpost -m -a  -o /home/reckon/workspace/ldd/custom_drivers/004_pcd_platform_driver/Module.symvers -e -i Module.symvers   -T -

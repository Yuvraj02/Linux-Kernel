cmd_/home/reckon/workspace/ldd/custom_drivers/003_psuedo_char_driver_multiple/modules.order := {   echo /home/reckon/workspace/ldd/custom_drivers/003_psuedo_char_driver_multiple/pcd_n.ko; :; } | awk '!x[$$0]++' - > /home/reckon/workspace/ldd/custom_drivers/003_psuedo_char_driver_multiple/modules.order

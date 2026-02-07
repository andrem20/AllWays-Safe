# Kernel Character Device Driver for PWM

This folder's content aims to do the following:
	
	- Address one or multiple GPIO pins designed for PWM prupose;
	- Set a its pin configuration: INPUT/PWM (alternate function);
	- Control PWM's frequency and duty-cycle.

	- Expected input:   "<frequency> <duty-cycle>"
	
## Commands for Test Case On Terminal

	In folder's directory,
	
	make
	scp pwm.ko <root@rasp.lan>: /root
	
	Inside RPi,
	
	cd /root
	ls
	insmod pwm.ko
	cd /dev
	ls | grep pwm
	echo "1000 50" > pwm<x>		#1kHz, 50%
	echo "2 25" > pwm<x>		#2Hz, 25%
	echo "0 0" > pwm<x>		#wave off
	rmmod pwm.ko
	ls | grep 'pwm'
	cd /root
	ls
	rm pwm.ko
	ls	
## Commands for Test Case On C/C++ program

Must: 
	- #include <sys/ioctl>
	- Kernel object (.ko) file must already be in the destination device
	
**APIs to call**

*file_descriptor* is the reference obtained from open operation on the file's path\
*PWM_SET_CONFIG/PWM_DISABLE* operation to be performed\
*config* is a struct of type *ioctl_pwm_config_t*\

	ioctl (file_descriptor, PWM_SET_CONFIG, config);
	ioctl (file_descriptor, PWM_DISABLE, config); 		// config is irrelevant here
	

### Observations
Replace <root@rasp.lan> by how you identify your RPi
Replace <x> by the minor number (aka device) you want to control

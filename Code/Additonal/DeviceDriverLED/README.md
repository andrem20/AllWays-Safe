# Kernel Character Device Driver for GPIO pin

This folder's content aims to do the following:
	
	- Address one or multiple GPIO pins;
	- Set a GPIO pin configuration: INPUT/OUTPUT;
	- Control GPIO pin state: 1/0.
	
## Commands for Test Case ON/OFF

	In folder's directory,
	
	make
	scp led.ko <root@rasp.lan>: /root
	
	Inside RPi,
	
	cd /root
	ls
	insmod led.ko
	cd /dev
	ls | grep 'led'
	echo '1' > led<x>
	echo '0 > led<x>
	rmmod led.ko
	ls | grep 'led'
	cd /root
	ls
	rm led.ko
	ls	
	
### Observations
Replace <root@rasp.lan> by how you identify your RPi
Replace <x> by the minor number (aka device) you want to control

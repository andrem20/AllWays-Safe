KDIR := /home/mariana/buildroot/buildroot-2025.02.8/output/build/linux-custom
ARCH ?= arm64
CROSS_COMPILE ?= /home/mariana/buildroot/buildroot-2025.02.8/output/host/bin/aarch64-buildroot-linux-gnu-
export ARCH CROSS_COMPILE
KBUILD_TARGETS	:= clean help modules modules_install

obj-m := pwm.o
pwm-objs:= pwm_dd.o utility.o

.PHONY: all $(KBUILD_TARGETS)

all: modules

$(KBUILD_TARGETS):
	#$(MAKE) -C $(KDIR) M=$(shell pwd) $@ EXTRA_CFLAGS="$(MY_CFLAGS)"
	$(MAKE) -C $(KDIR) M="$(shell pwd)" $@

# Include path for UAPI headers relative to kernel/ folder
ccflags-y := -I$(src)/../include

# Define target module name (vinput.ko)
obj-m += vinput.o

# Specify all object files that compose vinput.ko
vinput-objs := vinput_main.o \
               vinput_ringbuf.o \
               vinput_macro.o \
               vinput_input.o \
               vinput_stats.o \
               vinput_virtual.o

# Fallback to current kernel headers if KDIR isn't explicitly set
KDIR ?= /lib/modules/$(shell uname -r)/build

all:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean

.PHONY: all clean
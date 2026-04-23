obj-m += timer_module.o

all:
	@sudo make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

clean:
	@sudo make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean

load:
	@sudo insmod timer_module.ko

unload:
	@sudo rmmod timer_module

format:
	@sudo clang-format -i *.c *.h

check:
	@sudo cppcheck --enable=all *.c *.h

.PHONY: all clean load unload format check

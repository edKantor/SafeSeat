#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>

extern "C" int main(void)
{
	printk("SafeSeat starting\n");
	return 0;
}

#include <zephyr/kernel.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(main, LOG_LEVEL_DBG);

extern "C" int main(void)
{
	LOG_INF("SafeSeat starting");
	return 0;
}

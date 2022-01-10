#include <kernel.h>
#include <soc.h>
#include "fw_version.h"
#include <crc.h>

const struct fw_version *fw_version_get_current(void)
{
	const struct fw_version *ver =
		(struct fw_version *)soc_boot_get_fw_ver_addr();

	return ver;
}

void fw_version_dump(const struct fw_version *ver)
{
	printk("firmware version: 0x%x\n", ver->version_code);
	printk("system version: 0x%x\n",ver->system_version_code);
	printk("version name: %s\n", ver->version_name);
	printk("board name: %s\n", ver->board_name);
}

int fw_version_check(const struct fw_version *ver)
{
	u32_t checksum;

	if (ver->magic != FIRMWARE_VERSION_MAGIC)
		return -1;

	checksum = utils_crc32(0, (const u8_t *)ver, sizeof(struct fw_version) - 4);

	if (ver->checksum != checksum)
		return -1;
	
	return 0;
}

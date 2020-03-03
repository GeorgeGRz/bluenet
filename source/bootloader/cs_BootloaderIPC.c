#include <cs_BootloaderIPC.h>
#include <cfg/cs_HardwareVersions.h>
#include <cs_BootloaderConfig.h>
#include <string.h>

/**
 * Has hardware / bootloader version info.
 */
int ipc_bootloader(char *args, char *result) {
	if (args == NULL) return 0;
	char command = args[0];
	char length = args[1];
	switch(command) {
	case 0: 
		memcpy(result, get_hardware_version(), length);
		break;
	case 1:
		memcpy(result, g_BOOTLOADER_VERSION, length);
		break;
	default:
		return 0;
	} 
	return 1;
}



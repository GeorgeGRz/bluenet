#pragma once

#include <ipc/cs_IPC.h>

/**
 * The function that is registered by the bootloader.
 */
int ipc_bootloader(char *args, char *result);

/**
 * Register the function at location 1.
 */
REGISTER_IPC_HANDLER(ipc_bootloader, 1);

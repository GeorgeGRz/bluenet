/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: 13 Dec., 2019
 * License: LGPLv3, Apache License 2.0, and/or MIT (triple-licensed)
 */

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <ipc/cs_IPC.h>

/**
 * The function that is registered by the Crownstone itself.
 */
int ipc_crownstone(char *args, char *result);

/**
 * Register the function at location 0.
 */
REGISTER_IPC_HANDLER(ipc_crownstone, 0);

#ifdef __cplusplus
}
#endif

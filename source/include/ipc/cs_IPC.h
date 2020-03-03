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

#include <stddef.h>

typedef int (*ipc_func_t)(char* args, char* result);

/**
 * The function handler to be written by the module that is registering itself. It is registering:
 *   - a function itself which accepts a char array as argument and returns an integer;
 *   - an index (between 0 and 9) which offsets this function in the table with function handlers;
 *   - a start address of the code that registers this function;
 *   - the length of the code that registers this function;
 *   - a checksum that can be used by Crownstone to make sure the function is callable and not corrupted.
 *
 * It is not assumed that it is already known what the values for start_address, length, and checksum is gonna be.
 * They need to be removed and added again by an srec_cat step after linking.
 */
struct ipc_handler {
	ipc_func_t f;
	char ind;
	unsigned int start_address;
	unsigned int length;
	unsigned int checksum;
};

typedef struct ipc_handler ipc_handler_t;

/**
 * The maximum number of handlers we allow (each corresponds to a IPCX memory segment.
 */
#define MAX_IPC_HANDLERS 5

/**
 * This macro can be used by modules that want to register a function for the Crownstone code to be called across
 * process binaries. The very first function is registered by the Crownstone code itself. It can be used by a module
 * to reach back to Crownstone code with particular information. This functions always returns so we unroll the stack.
 */
#define REGISTER_IPC_HANDLER(func, index)               \
	static ipc_handler_t __ipc_handler__ ## index       \
	__attribute((__section__(".ipc_handler" # index)))  \
	__attribute((__used__)) = {                         \
	    .f = func,                                      \
		.ind = index,                                   \
		.start_address = 0xFF,                          \
		.length = 0xFF,                                 \
		.checksum = 0xFF,                               \
	}

ipc_handler_t handlers[MAX_IPC_HANDLERS];

void getIPCHandlers();

int IPCHandlerExists(unsigned char index);

ipc_handler_t * getIPCHandler(unsigned char index);

#ifdef __cplusplus
}
#endif

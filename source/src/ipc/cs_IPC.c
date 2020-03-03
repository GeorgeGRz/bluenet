#include <ipc/cs_IPC.h>

#ifdef __cplusplus
extern "C" {
#endif

extern ipc_handler_t __start_ipc_handler0;
extern ipc_handler_t __start_ipc_handler1;
extern ipc_handler_t __start_ipc_handler2;
extern ipc_handler_t __start_ipc_handler3;
extern ipc_handler_t __start_ipc_handler4;

void getIPCHandlers() {
	ipc_handler_t *handler;

	unsigned char i = 0;
	handler = &__start_ipc_handler0;
	if (handler != NULL) {
		handlers[i] = *handler;
	} else {
		handlers[i].ind = -1;
	}
	i++;
	handler = &__start_ipc_handler1;
	if (handler != NULL) {
		handlers[i] = *handler;
	} else {
		handlers[i].ind = -1;
	}
	i++;
	handler = &__start_ipc_handler2;
	if (handler != NULL) {
		handlers[i] = *handler;
	} else {
		handlers[i].ind = -1;
	}
	i++;
	handler = &__start_ipc_handler3;
	if (handler != NULL) {
		handlers[i] = *handler;
	} else {
		handlers[i].ind = -1;
	}
	i++;
	handler = &__start_ipc_handler4;
	if (handler != NULL) {
		handlers[i] = *handler;
	} else {
		handlers[i].ind = -1;
	}
}

int IPCHandlerExists(unsigned char index) {
	if (handlers[index].ind == index) { 
		return 1;
	}
	return 0;
}

ipc_handler_t * getIPCHandler(unsigned char index) {
	return &handlers[index];
}

#ifdef __cplusplus
}
#endif

#include <ipc/cs_IPC_main.h>

#ifdef __cplusplus
extern "C" {
#endif

int ipc_crownstone(char *args, char *result) {
	if (args == NULL) return 0;
	if (result == NULL) return 0;
	for (int i = 0; i < 2; ++i) {
		result[i] = args[i] + 1;
	}
	return 1;
}

#ifdef __cplusplus
}
#endif

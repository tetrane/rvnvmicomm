#pragma once

 #ifdef __cplusplus
extern "C" {
#endif

void test_server_close();

typedef enum {
	VMI_CB_PUT_RESPONSE = 0,
	VMI_CB_READ_VIRTUAL_MEMORY,
	VMI_CB_READ_REGISTER,
	VMI_CB_SET_BREAKPOINT,
	VMI_CB_ENABLE_SYNC_WAIT,
	VMI_CB_DISABLE_SYNC_WAIT,
	VMI_CB_REMOVE_BREAKPOINT,
	VMI_CB_REMOVE_ALL_BREAKPOINTS,
	VMI_CB_SET_WATCHPOINT,
	VMI_CB_REMOVE_WATCHPOINT,
	VMI_CB_REMOVE_ALL_WATCHPOINTS,
	VMI_CB_PAUSE_VM,
	VMI_CB_STEP_VM,
	VMI_CB_CONTINUE_ASYNC_VM,

	VMI_CB_NONE
} CallbackFunction;

typedef struct {
	CallbackFunction function;
	uint64_t params[3];
} CallbackCalled;

CallbackCalled get_last_callback();
void reset_last_callback();
void set_cb_return_value(int err);

#ifdef __cplusplus
}
#endif

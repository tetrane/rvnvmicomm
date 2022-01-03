#pragma once

#include <stdint.h>

#include <rvnvmicomm_common/reven-vmi.h>

#ifdef __cplusplus
extern "C" {
#endif

// users should override following functions...

extern int vmis_start(const char *device);

extern void vmis_cb_enable_sync_wait(void);
extern void vmis_cb_disable_sync_wait(void);

extern void vmis_cb_put_response(const uint8_t *buf, uint32_t size);

extern int vmis_cb_read_physical_memory(uint64_t addr, uint32_t len, uint8_t * buffer) __attribute__((nonnull(3)));
extern int vmis_cb_read_register(int32_t reg_group, int32_t reg_id, uint64_t * reg_val) __attribute__((nonnull(3)));
extern int vmis_cb_read_cpuid_attributes(vmi_cpuid_values_t* attributes) __attribute__((nonnull(1)));

extern int vmis_cb_set_breakpoint(uint64_t va);
extern int vmis_cb_remove_breakpoint(uint64_t va);
extern int vmis_cb_remove_all_breakpoints(void);

extern int vmis_cb_set_watchpoint(uint64_t va, uint32_t len, int wp);
extern int vmis_cb_remove_watchpoint(uint64_t va);
extern int vmis_cb_remove_all_watchpoints(void);

extern int vmis_cb_pause_vm(void);
extern int vmis_cb_step_vm(void);
extern int vmis_cb_continue_async_vm(void);

/*
typedef enum RunState {
	RUN_STATE_DEBUG,
	RUN_STATE_INMIGRATE,
	RUN_STATE_INTERNAL_ERROR,
	RUN_STATE_IO_ERROR,
	RUN_STATE_PAUSED,
	RUN_STATE_POSTMIGRATE,
	RUN_STATE_PRELAUNCH,
	RUN_STATE_FINISH_MIGRATE,
	RUN_STATE_RESTORE_VM,
	RUN_STATE_RUNNING,
	RUN_STATE_SAVE_VM,
	RUN_STATE_SHUTDOWN,
	RUN_STATE_SUSPENDED,
	RUN_STATE_WATCHDOG,
	RUN_STATE_GUEST_PANICKED,
	RUN_STATE_COLO,
	RUN_STATE__MAX,
} RunState;
*/
extern int /* RunState */ vmis_cb_run_state(void);

// so they can use the following function

extern void vmis_handle_request(const vmi_request_t *req);

#ifdef __cplusplus
}
#endif

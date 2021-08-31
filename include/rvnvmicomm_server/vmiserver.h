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

extern int vmis_cb_set_breakpoint(uint64_t va);
extern int vmis_cb_remove_breakpoint(uint64_t va);
extern int vmis_cb_remove_all_breakpoints(void);

extern int vmis_cb_set_watchpoint(uint64_t va, uint32_t len, int wp);
extern int vmis_cb_remove_watchpoint(uint64_t va);
extern int vmis_cb_remove_all_watchpoints(void);

extern int vmis_cb_pause_vm(void);
extern int vmis_cb_step_vm(void);
extern int vmis_cb_continue_async_vm(void);

// so they can use the following function

extern void vmis_handle_request(const vmi_request_t *req);

#ifdef __cplusplus
}
#endif

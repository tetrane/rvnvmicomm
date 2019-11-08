#pragma once

#include <stdint.h>

#include <rvnvmicomm_common/reven-vmi.h>

#ifdef __cplusplus
extern "C" {
#endif

// users should override following functions...

extern int vmiserver_start(const char *device) __attribute__((nonnull(1), weak));

extern void enable_sync_wait() __attribute__((weak));
extern void disable_sync_wait() __attribute__((weak));

extern void put_response(const uint8_t *buf, uint32_t size) __attribute__((weak));

extern int read_virtual_memory(uint64_t va, uint32_t len, uint8_t * buffer) __attribute__((nonnull(3), weak));
extern int read_register(int32_t reg_group, int32_t reg_id, uint64_t * reg_val) __attribute__((nonnull(3), weak));

extern int set_breakpoint(uint64_t va) __attribute__((weak));
extern int remove_breakpoint(uint64_t va) __attribute__((weak));
extern int remove_all_breakpoints(void) __attribute__((weak));

extern int set_watchpoint(uint64_t va, uint32_t len, int wp) __attribute__((weak));
extern int remove_watchpoint(uint64_t va, uint32_t len) __attribute__((weak));
extern int remove_all_watchpoints(void) __attribute__((weak));

extern int pause_vm(void) __attribute__((weak));
extern int step_vm(void) __attribute__((weak));
extern int continue_async_vm(void) __attribute__((weak));

// so they can use the following function

extern void handle_request(const vmi_request_t *req);

#ifdef __cplusplus
}
#endif
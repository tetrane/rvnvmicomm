#pragma once

#include <stdint.h>

typedef enum vmic_error_t {
	OK = 0,

	// QMP errors
	QMP_SOCKET_CREATE_FAILED,
	QMP_RECV_FAILED,
	QMP_SEND_FAILED,
	QMP_CONNECTION_FAILED,
	QMP_CONNECTION_BAD_ADDRESS,
	QMP_ACTIVATE_COMMAND_MODE_FAILED,
	QMP_OPEN_VMI_FAILED,
	QMP_CLOSE_VMI_FAILED,

	// VMI errors
	VMI_SOCKET_PATH_TOO_LONG,
	VMI_SOCKET_CREATE_FAILED,
	VMI_SOCKET_CONNECT_FAILED,
	VMI_SOCKET_CLOSE_FAILED,
	VMI_SOCKET_SEND_FAILED,
	VMI_SOCKET_RECV_FAILED,

	// Data generic errors
	DATA_BAD_RESPONSE,
	DATA_BAD_REQUEST,

	// Specific
	REGISTER_READ_FAILED,
	MEMORY_READ_FAILED,
	BREAKPOINT_SET_FAILED,
	BREAKPOINT_REMOVE_FAILED,
	BREAKPOINT_REMOVE_ALL_FAILED,
	WATCHPOINT_READ_SET_FAILED,
	WATCHPOINT_WRITE_SET_FAILED,
	WATCHPOINT_REMOVE_FAILED,
	EXECUTION_PAUSE_FAILED,
	EXECUTION_STEP_FAILED,
	EXECUTION_CONTINUE_FAILED,
	EXECUTION_CONTINUE_ASYNC_FAILED,
	EXECUTION_STATUS_FAILED,
} vmic_error_t;

typedef enum vmic_run_state_t {
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
} vmic_run_state_t;

#ifdef __cplusplus
extern "C" {
#endif

struct cpuid_values_t;

extern vmic_error_t vmic_connect(const char * vmi_unix_socket, int *fd);
extern vmic_error_t vmic_close(int fd);

extern vmic_error_t vmic_read_register(int fd, unsigned reg_group, unsigned reg_id, uint64_t *reg_value);
extern vmic_error_t vmic_read_physical_memory(int fd, uint64_t addr, uint32_t length, uint8_t *buffer);
extern vmic_error_t vmic_read_cpuid_attributes(int fd, struct cpuid_values_t* attributes);

extern vmic_error_t vmic_write_physical_memory(int fd, uint64_t addr, uint32_t length, const uint8_t *buffer);
extern vmic_error_t vmic_write_linear_memory(int fd, uint64_t addr, uint32_t length, const uint8_t *buffer);

extern vmic_error_t vmic_set_breakpoint(int fd, uint64_t va);
extern vmic_error_t vmic_remove_breakpoint(int fd, uint64_t va);
extern vmic_error_t vmic_remove_all_breakpoints(int fd);

extern vmic_error_t vmic_set_read_watchpoint(int fd, uint64_t va, uint32_t len);
extern vmic_error_t vmic_set_write_watchpoint(int fd, uint64_t va, uint32_t len);

extern vmic_error_t vmic_remove_watchpoint(int fd, uint64_t va);

extern vmic_error_t vmic_pause_vm(int fd);
extern vmic_error_t vmic_step_vm(int fd);
extern vmic_error_t vmic_continue_vm(int fd);
extern vmic_error_t vmic_continue_vm_async(int fd);

extern vmic_run_state_t vmic_status_vm(int fd);

#ifdef __cplusplus
}
#endif

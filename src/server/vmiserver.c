#include <stddef.h>
#include <stdlib.h>

#include <rvnvmicomm_common/reven-vmi.h>
#include <rvnvmicomm_server/vmiserver.h>

#define unused(x) (void)x

int vmiserver_start(const char *device)
{
	unused(device);
	return -1;
}

void enable_sync_wait() {}

void disable_sync_wait() {}

void put_response(const uint8_t *buf, uint32_t size) { unused(buf); unused(size); }

int read_virtual_memory(uint64_t va, uint32_t len, uint8_t * buffer)
{
	unused(va); unused(len); unused(buffer);
	return -1;
}

int read_register(int32_t reg_group, int32_t reg_id, uint64_t * reg_val)
{
	unused(reg_group), unused(reg_id); unused(reg_val);
	return -1;
}

int set_breakpoint(uint64_t va)
{
	unused(va);
	return -1;
}

int remove_breakpoint(uint64_t va)
{
	unused(va);
	return -1;
}

int remove_all_breakpoints(void)
{
	return -1;
}

int set_watchpoint(uint64_t va, uint32_t len, int wp)
{
	unused(va); unused(len); unused(wp);
	return -1;
}

int remove_watchpoint(uint64_t va, uint32_t len)
{
	unused(va); unused(len); return -1;
}

int remove_all_watchpoints(void)
{
	return -1;
}

int pause_vm(void)
{
	return -1;
}

int step_vm(void)
{
	return -1;
}

int continue_async_vm(void)
{
	return -1;
}

static inline void put_empty_response(void) { put_response(NULL, 0); }

#define put_typed_response(resp) put_response((const uint8_t*)resp, sizeof(*(resp)))

void handle_request(const vmi_request_t *req)
{
	disable_sync_wait();

	switch (req->request_type)
	{
	case MEM_READ: {
		uint32_t mem_size = req->request_data.memory_size;
		if (mem_size == 0) {
			put_empty_response();
		} else {
			uint8_t *buf = malloc(mem_size);
			if (read_virtual_memory(req->request_data.virtual_address, mem_size, buf) >= 0) {
				put_response(buf, mem_size);
			} else {
				put_empty_response();
			}
			free(buf);
		}
		return;
	}

	case REG_READ: {
		union {
			uint64_t reg_val_64;
			uint32_t reg_val_32;
		} reg_val;
		int reg_size = read_register(req->request_data.register_group, req->request_data.register_id, &reg_val.reg_val_64);
		switch (reg_size)
		{
		case 8:
			put_typed_response(&reg_val.reg_val_64);
			break;

		case 4:
			put_typed_response(&reg_val.reg_val_32);
			break;

		default:
			return put_empty_response();
		}
		return;
	}

	case BP: {
		int bp_err;
		switch (req->request_action)
		{
		case SET:
			bp_err = set_breakpoint(req->request_data.virtual_address);
			break;

		case REM:
			bp_err = remove_breakpoint(req->request_data.virtual_address);
			break;

		case REM_ALL:
			bp_err = remove_all_breakpoints();
			break;

		default:
			return put_empty_response();
		}
		return put_typed_response(&bp_err);
	}

	case WP_READ:
	case WP_WRITE: {
		int wp_err;
		switch (req->request_action)
		{
		case SET:
			wp_err = set_watchpoint(req->request_data.virtual_address, req->request_data.memory_size, req->request_type);
			break;

		case REM:
			wp_err = remove_watchpoint(req->request_data.virtual_address, req->request_data.memory_size);
			break;

		case REM_ALL:
			wp_err = remove_all_watchpoints();
			break;

		default:
			return put_empty_response();
		}
		return put_typed_response(&wp_err);
	}

	case EXEC: {
		int ex_err;
		switch (req->request_action)
		{
		case PAUSE:
			ex_err = pause_vm();
			break;

		case STEP:
			enable_sync_wait();
			ex_err = step_vm();
			return; // no immediate response

		case CONTINUE:
			enable_sync_wait();
			ex_err = continue_async_vm();
			return; // no immediate response

		case CONTINUE_ASYNC:
			ex_err = continue_async_vm();
			break; // response immediately

		default:
			return put_empty_response();
		}
		return put_typed_response(&ex_err);
	}

	default:
		return put_empty_response();
	}
}
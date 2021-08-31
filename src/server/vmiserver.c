#include <stddef.h>
#include <stdlib.h>

#include <rvnvmicomm_common/reven-vmi.h>
#include <rvnvmicomm_server/vmiserver.h>

static inline void put_empty_response(void) { vmis_cb_put_response(NULL, 0); }

#define put_typed_response(resp) vmis_cb_put_response((const uint8_t*)resp, sizeof(*(resp)))

void vmis_handle_request(const vmi_request_t *req)
{
	vmis_cb_disable_sync_wait();

	switch (req->request_type)
	{
	case MEM_READ: {
		uint32_t mem_size = req->request_data.memory_size;
		if (mem_size == 0) {
			put_empty_response();
		} else {
			uint8_t *buf = malloc(mem_size);
			if (vmis_cb_read_physical_memory(req->request_data.address, mem_size, buf) >= 0) {
				vmis_cb_put_response(buf, mem_size);
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
		int reg_size = vmis_cb_read_register(req->request_data.register_group, req->request_data.register_id, &reg_val.reg_val_64);
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

	case ATT_READ: {
		vmi_cpuid_values_t buf = {0};
		if (vmis_cb_read_cpuid_attributes(&buf) == 0) {
			put_typed_response(&buf);
		} else {
			put_empty_response();
		}
		return;
	}

	case BP: {
		int bp_err;
		switch (req->request_action)
		{
		case SET:
			bp_err = vmis_cb_set_breakpoint(req->request_data.address);
			break;

		case REM:
			bp_err = vmis_cb_remove_breakpoint(req->request_data.address);
			break;

		case REM_ALL:
			bp_err = vmis_cb_remove_all_breakpoints();
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
			wp_err = vmis_cb_set_watchpoint(req->request_data.address, req->request_data.memory_size, req->request_type);
			break;

		case REM:
			wp_err = vmis_cb_remove_watchpoint(req->request_data.address);
			break;

		case REM_ALL:
			wp_err = vmis_cb_remove_all_watchpoints();
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
			ex_err = vmis_cb_pause_vm();
			break;

		case STEP:
			vmis_cb_enable_sync_wait();
			ex_err = vmis_cb_step_vm();
			if (ex_err)
				break; // Immediate reponse on error

			return; // no immediate response

		case CONTINUE:
			vmis_cb_enable_sync_wait();
			ex_err = vmis_cb_continue_async_vm();
			if (ex_err)
				break; // Immediate reponse on error

			return; // no immediate response

		case CONTINUE_ASYNC:
			ex_err = vmis_cb_continue_async_vm();
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

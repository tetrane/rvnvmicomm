#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <netdb.h>
#include <unistd.h>
#include <errno.h>

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <sys/time.h>

#include <rvnvmicomm_common/reven-vmi.h>
#include <rvnvmicomm_client/vmiclient.h>

static char send_error_msg_format[] = "rvnvmicomm: Encountered an error when sending a request: %s (%d)\n";

// Read server response package [data length: 4 bytes] + [data buffer: data length]
static ssize_t recv_block(int sockfd, void *buf, size_t len)
{
	if (len < sizeof(uint32_t)) {
		return -1;
	}

	// read the header containing the length of the rest
	uint32_t resp_data_len = 0;
	ssize_t recv_count = recv(sockfd, &resp_data_len, sizeof(resp_data_len), MSG_WAITALL);
	if (recv_count < 0) {
		fprintf(stderr, "rvnvmicomm: Encountered an error when receiving size: %s (%d)\n", strerror(errno), errno);
		return -1;
	} else if(recv_count != sizeof(resp_data_len)) {
		fprintf(stderr, "rvnvmicomm: Encountered an error when receiving size: unexpected size of %ld (expected %ld)\n", recv_count, sizeof(resp_data_len));
		return -1;
	}

	memcpy(buf, &resp_data_len, sizeof(resp_data_len));

	// read the rest
	ssize_t data_count = 0;
	if (resp_data_len == 0) {
		return sizeof(resp_data_len);
	}
	else if (resp_data_len == len - sizeof(resp_data_len)) {
		data_count = recv(sockfd, (uint8_t*)buf + sizeof(resp_data_len), resp_data_len, MSG_WAITALL);

		if (data_count < 0) {
			fprintf(stderr, "rvnvmicomm: Encountered an error when receiving data: %s (%d)\n", strerror(errno), errno);
			return data_count;
		} else if (data_count < resp_data_len) {
			fprintf(stderr, "rvnvmicomm: Encountered an error when receiving data: unexpected size of %ld (expected %d)\n", data_count, resp_data_len);
			return data_count;
		}

		return data_count + sizeof(resp_data_len);
	}
	else {
		fprintf(stderr, "rvnvmicomm: Size is different than expected (%lu != %u)\n", len - sizeof(resp_data_len), resp_data_len);

		uint8_t* tmp_buf = malloc(resp_data_len);
		if (!tmp_buf) {
			fprintf(stderr, "rvnvmicomm: Could not allocate data\n");
			return -1;
		}

		recv(sockfd, tmp_buf, resp_data_len, MSG_WAITALL);

		free(tmp_buf);
		return -1;
	}
}

vmic_error_t __attribute((nonnull(1), nonnull(2))) vmic_connect(const char * vmi_unix_socket, int *fd) {
	int sockfd = -1;
	struct sockaddr_un addr;

	if (strlen(vmi_unix_socket) + 1 > sizeof(addr.sun_path)) {
		return VMI_SOCKET_PATH_TOO_LONG;
	}
	strcpy(addr.sun_path, vmi_unix_socket);

	sockfd = socket(AF_UNIX, SOCK_STREAM, 0);
	if (sockfd < 0) {
		return VMI_SOCKET_CREATE_FAILED;
	}

	addr.sun_family = AF_UNIX;

	if (connect(sockfd, (const struct sockaddr*)&addr, sizeof(addr)) < 0) {
		close(sockfd);
		return VMI_SOCKET_CONNECT_FAILED;
	}

	*fd = sockfd;
	return OK;
}

vmic_error_t vmic_close(int fd) {
	int sockfd = -1;

	sockfd = close(fd);
	if (sockfd < 0) {
		return VMI_SOCKET_CLOSE_FAILED;
	}

	return OK;
}

vmic_error_t __attribute((nonnull(4))) vmic_read_register(int fd, unsigned reg_group, unsigned reg_id, uint64_t *reg_value) {
	vmi_request_t req;
	ssize_t msg_len = -1;
	struct __attribute__((__packed__)) gp_response_t {
		uint32_t len;
		uint64_t value;
	} gp_resp;

	struct __attribute__((__packed__)) seg_response_t {
		uint32_t len;
		uint32_t value;
	} seg_resp;

	memset(&req, 0, sizeof(req));

	req.request_type = (vmi_request_type_t)REG_READ;
	if (reg_group > (vmi_x86_register_group_t)MSR) {
		return DATA_BAD_REQUEST;
	}
	req.request_data.register_group = (vmi_x86_register_group_t)reg_group;
	if (reg_id > (vmi_x86_register_t)MSR_KERNELGSBASE) {
		return DATA_BAD_REQUEST;
	}
	req.request_data.register_id = (vmi_x86_register_t)reg_id;

	msg_len = send(fd, &req, sizeof(req), 0);
	if (msg_len < 0) {
		fprintf(stderr, send_error_msg_format, strerror(errno), errno);
		return VMI_SOCKET_SEND_FAILED;
	} else if (msg_len != sizeof(req)) {
		return VMI_SOCKET_SEND_FAILED;
	}

	switch (reg_group) {
		case PC:
		case GP:
		case CTRL:
		case MSR:
			// type punning
			msg_len = recv_block(fd, &gp_resp, sizeof(gp_resp));
			if (msg_len != sizeof(gp_resp)) {
				return DATA_BAD_RESPONSE;
			}
			if (gp_resp.len == 0) {
				return REGISTER_READ_FAILED;
			}
			if (gp_resp.len != sizeof(gp_resp.value)) {
				return DATA_BAD_RESPONSE;
			}
			*reg_value = gp_resp.value;
			break;

		case SEG:
			// type punning
			msg_len = recv_block(fd, &seg_resp, sizeof(seg_resp));
			if (msg_len != sizeof(seg_resp)) {
				return DATA_BAD_RESPONSE;
			}
			if (seg_resp.len == 0) {
				return REGISTER_READ_FAILED;
			}
			if (seg_resp.len != sizeof(seg_resp.value)) {
				return DATA_BAD_RESPONSE;
			}
			*reg_value = seg_resp.value;
			break;

		default: // never happens
			break;
	}


	return OK;
}

vmic_error_t __attribute((nonnull(4)))
vmic_read_physical_memory(int fd, uint64_t addr, uint32_t length, uint8_t *buffer) {
	vmi_request_t req;
	ssize_t msg_len = -1;
	uint8_t *msg_buffer;
	ssize_t msg_buffer_length = 0;
	uint32_t embedded_length = 0;

	memset(&req, 0, sizeof(req));

	req.request_type = (vmi_request_type_t)MEM_READ;
	req.request_data.address = addr;
	req.request_data.memory_size = length;

	msg_len = send(fd, &req, sizeof(req), 0);
	if (msg_len < 0) {
		fprintf(stderr, send_error_msg_format, strerror(errno), errno);
		return VMI_SOCKET_SEND_FAILED;
	} else if (msg_len != sizeof(req)) {
		return VMI_SOCKET_SEND_FAILED;
	}

	// buffer: [data length: 4 bytes] + [data buffer: data length]
	msg_buffer_length = sizeof(uint32_t) + length;
	msg_buffer = malloc(msg_buffer_length);
	msg_len = recv_block(fd, msg_buffer, msg_buffer_length);

	if (msg_len < (ssize_t)sizeof(uint32_t)) {
		free(msg_buffer);
		return DATA_BAD_RESPONSE;
	}

	// cannot cast (e.g. (uint32_t)msg_buffer) because of memory alignment
	embedded_length = msg_buffer[0] + (msg_buffer[1] << 8) + (msg_buffer[2] << 16) + (msg_buffer[3] << 24);

	if (msg_len == sizeof(uint32_t)) {
		if (embedded_length == 0) {
			free(msg_buffer);
			return MEMORY_READ_FAILED;
		} else {
			free(msg_buffer);
			return DATA_BAD_RESPONSE;
		}
	}

	if (msg_len != msg_buffer_length || embedded_length != length) {
		free(msg_buffer);
		return DATA_BAD_RESPONSE;
	}

	memcpy(buffer, msg_buffer + sizeof(uint32_t), length);
	free(msg_buffer);
	return OK;
}

vmic_error_t __attribute((nonnull(2)))
vmic_read_cpuid_attributes(int fd, vmi_cpuid_values_t* attributes) {
	vmi_request_t req;
	ssize_t msg_len = -1;
	struct __attribute__((__packed__)) response_t {
		uint32_t len;
		vmi_cpuid_values_t value;
	} resp;

	memset(&req, 0, sizeof(req));

	req.request_type = (vmi_request_type_t)ATT_READ;

	msg_len = send(fd, &req, sizeof(req), 0);
	if (msg_len < 0) {
		fprintf(stderr, send_error_msg_format, strerror(errno), errno);
		return VMI_SOCKET_SEND_FAILED;
	} else if (msg_len != sizeof(req)) {
		return VMI_SOCKET_SEND_FAILED;
	}

	// type punning
	msg_len = recv_block(fd, &resp, sizeof(resp));
	if (msg_len != sizeof(resp) || resp.len != sizeof(resp.value)) {
		return DATA_BAD_RESPONSE;
	}

	memcpy(attributes, &resp.value, sizeof(resp.value));

	return OK;
}

static vmic_error_t breakpoint(int fd, uint64_t va, bool set_or_rem) {
	vmi_request_t req;
	ssize_t msg_len = -1;
	struct __attribute__((__packed__)) response_t {
		uint32_t len;
		int32_t value;
	} resp;

	memset(&req, 0, sizeof(req));

	req.request_type = (vmi_request_type_t)BP;
	if (set_or_rem) {
		req.request_action = (vmi_request_action_t)SET;
	} else {
		req.request_action = (vmi_request_action_t)REM;
	}
	req.request_data.address = va;

	msg_len = send(fd, &req, sizeof(req), 0);
	if (msg_len < 0) {
		fprintf(stderr, send_error_msg_format, strerror(errno), errno);
		return VMI_SOCKET_SEND_FAILED;
	} else if (msg_len != sizeof(req)) {
		return VMI_SOCKET_SEND_FAILED;
	}

	// type punning
	msg_len = recv_block(fd, &resp, sizeof(resp));
	if (msg_len != sizeof(resp) || resp.len != sizeof(resp.value)) {
		return DATA_BAD_RESPONSE;
	}

	if (resp.value != 0) {
		if (set_or_rem) {
			return BREAKPOINT_SET_FAILED;
		} else {
			return BREAKPOINT_REMOVE_FAILED;
		}
	}

	return OK;
}

vmic_error_t vmic_set_breakpoint(int fd, uint64_t va) {
	return breakpoint(fd, va, true);
}

vmic_error_t vmic_remove_breakpoint(int fd, uint64_t va) {
	return breakpoint(fd, va, false);
}

vmic_error_t vmic_remove_all_breakpoints(int fd) {
	vmi_request_t req;
	ssize_t msg_len = -1;
	struct __attribute__((__packed__)) response_t {
		uint32_t len;
		int32_t value;
	} resp;

	memset(&req, 0, sizeof(req));

	req.request_type = (vmi_request_type_t)BP;
	req.request_action = (vmi_request_action_t)REM_ALL;

	msg_len = send(fd, &req, sizeof(req), 0);
	if (msg_len < 0) {
		fprintf(stderr, send_error_msg_format, strerror(errno), errno);
		return VMI_SOCKET_SEND_FAILED;
	} else if (msg_len != sizeof(req)) {
		return VMI_SOCKET_SEND_FAILED;
	}

	msg_len = recv_block(fd, &resp, sizeof(resp));
	if (msg_len != sizeof(resp) || resp.len != sizeof(resp.value)) {
		return DATA_BAD_RESPONSE;
	}

	if (resp.value != 0) {
		return BREAKPOINT_REMOVE_ALL_FAILED;
	}

	return OK;
}

static vmic_error_t watchpoint(int fd, uint64_t va, uint32_t len, bool set_or_remove, bool read_or_write) {
	vmi_request_t req;
	ssize_t msg_len = -1;
	struct __attribute__((__packed__)) gp_response_t {
		uint32_t len;
		uint32_t value;
	} gp_resp;
	int acc_bits;

	memset(&req, 0, sizeof(req));

	if (read_or_write) {
		req.request_type = (vmi_request_type_t)WP_READ;
	} else {
		req.request_type = (vmi_request_type_t)WP_WRITE;
	}

	if (set_or_remove) {
		req.request_action = (vmi_request_action_t)SET;
	} else {
		req.request_action = (vmi_request_action_t)REM;
	}

	req.request_data.address = va;

	if (set_or_remove) {
		req.request_data.memory_size = len;
	} else {
		// doesn't care
	}

	msg_len = send(fd, &req, sizeof(req), 0);
	if (msg_len < 0) {
		fprintf(stderr, send_error_msg_format, strerror(errno), errno);
		return VMI_SOCKET_SEND_FAILED;
	} else if (msg_len != sizeof(req)) {
		return VMI_SOCKET_SEND_FAILED;
	}

	// type punning
	msg_len = recv_block(fd, &gp_resp, sizeof(gp_resp));
	if (msg_len != sizeof(gp_resp) || gp_resp.len != sizeof(gp_resp.value)) {
		return DATA_BAD_RESPONSE;
	}

	if (gp_resp.value != 0) {
		acc_bits = ((read_or_write ? 1 : 0) << 1) + (set_or_remove ? 1 : 0);
		switch (acc_bits)
		{
		case 0b00:
		case 0b10:
			return WATCHPOINT_REMOVE_FAILED;

		case 0b01:
			return WATCHPOINT_WRITE_SET_FAILED;

		case 0b11:
			return WATCHPOINT_READ_SET_FAILED;

		default:
			break;
		}
	}

	return OK;
}

vmic_error_t vmic_set_read_watchpoint(int fd, uint64_t va, uint32_t len) {
	return watchpoint(fd, va, len, true, true);
}

vmic_error_t vmic_set_write_watchpoint(int fd, uint64_t va, uint32_t len) {
	return watchpoint(fd, va, len, true, false);
}

vmic_error_t vmic_remove_watchpoint(int fd, uint64_t va) {
	return watchpoint(fd, va, 0, false, false);
}

static vmic_error_t execution(int fd, int act) {
	vmi_request_t req;
	ssize_t msg_len;
	struct __attribute__((__packed__)) gp_response_t {
		uint32_t len;
		uint32_t value;
	} gp_resp;

	memset(&req, 0, sizeof(req));

	switch ((vmi_request_action_t)act) {
	case PAUSE:
	case STEP:
	case CONTINUE:
	case CONTINUE_ASYNC:
		break;
	default:
		return DATA_BAD_REQUEST;
	}

	req.request_type = (vmi_request_type_t)EXEC;
	req.request_action = (vmi_request_action_t)act;

	msg_len = send(fd, &req, sizeof(req), 0);
	if (msg_len < 0) {
		fprintf(stderr, send_error_msg_format, strerror(errno), errno);
		return VMI_SOCKET_SEND_FAILED;
	} else if (msg_len != sizeof(req)) {
		return VMI_SOCKET_SEND_FAILED;
	}

	msg_len = recv_block(fd, &gp_resp, sizeof(gp_resp));
	if (msg_len != sizeof(gp_resp) || gp_resp.len != sizeof(gp_resp.value)) {
		return DATA_BAD_RESPONSE;
	}

	if (gp_resp.value != 0) {
		switch ((vmi_request_action_t)act) {
		case PAUSE:
			return EXECUTION_PAUSE_FAILED;

		case STEP:
			return EXECUTION_STEP_FAILED;

		case CONTINUE:
			return EXECUTION_CONTINUE_FAILED;

		case CONTINUE_ASYNC:
			return EXECUTION_CONTINUE_ASYNC_FAILED;

		default: // never happen
			return DATA_BAD_RESPONSE;
		}
	}

	return OK;
}

vmic_error_t vmic_pause_vm(int fd) {
	return execution(fd, (vmi_request_action_t)PAUSE);
}

vmic_error_t vmic_step_vm(int fd) {
	return execution(fd, (vmi_request_action_t)STEP);
}

vmic_error_t vmic_continue_vm(int fd) {
	return execution(fd, (vmi_request_action_t)CONTINUE);
}

vmic_error_t vmic_continue_vm_async(int fd) {
	return execution(fd, (vmi_request_action_t)CONTINUE_ASYNC);
}

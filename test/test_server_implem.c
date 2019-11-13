#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/epoll.h>

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdbool.h>

#include <pthread.h>

#include <rvnvmicomm_common/reven-vmi.h>
#include <rvnvmicomm_server/vmiserver.h>
#include <rvnvmicomm_client/vmiclient.h>

#include "test_server_implem.h"

#define unused(x) (void)x

// implement server support functions

static int server_sockfd = -1;
static int client_sockfd = -1;
static pthread_t vmiserver_thread;
static void *vmiserver(void *args);

static CallbackCalled last_callback;
static int cb_value_to_return;

void set_last_callback(CallbackFunction func, uint64_t param_1, uint64_t param_2, uint64_t param_3);
void fill_buffer(void* buffer, size_t size);

int vmis_start(const char *device)
{
	server_sockfd = socket(AF_UNIX, SOCK_STREAM, 0);
	if (server_sockfd < 0) {
		return -1;
	}

	struct sockaddr_un sock_addr = {
		.sun_family = AF_UNIX,
	};
	snprintf(sock_addr.sun_path, sizeof(sock_addr.sun_path), "%s", device);
	unlink(device);
	if (bind(server_sockfd, (struct sockaddr*)&sock_addr, sizeof(sock_addr)) != 0) {
		close(server_sockfd);
		return -2;
	}

	if (listen(server_sockfd, 128) != 0) {
		close(server_sockfd);
		return -3;
	}

	if (pthread_create(&vmiserver_thread, NULL, vmiserver, NULL) !=0) {
		close(server_sockfd);
		return -4;
	}

	reset_last_callback();
	set_cb_return_value(0);

	return 0;
}

void test_server_close() {
	pthread_join(vmiserver_thread, NULL);
}

static void handle_connection(int fd)
{
	client_sockfd = fd;

	vmi_request_t req;
	for (;;) {
		if (recv(fd, &req, sizeof(req), 0) != sizeof(req)) {
			return;
		}
		vmis_handle_request(&req);
	}
	close(fd);
}

static void *vmiserver(void *args)
{
	struct sockaddr_un sock_addr;
	for (;;) {
		socklen_t sock_addr_len = sizeof(sock_addr);
		int client_fd = accept(server_sockfd, (struct sockaddr*)&sock_addr, &sock_addr_len);
		if (client_fd < 0) {
			continue;
		}

		handle_connection(client_fd);
		close(client_fd);
		break;
	}

	close(server_sockfd);
}

void vmis_cb_put_response(const uint8_t *buf, uint32_t size)
{
	if (size > 0) {
		uint32_t resp_len = size + sizeof(uint32_t);
		uint8_t *resp_buf = (uint8_t*)malloc(resp_len);

		memcpy(resp_buf, &size, sizeof(uint32_t));
		memcpy(resp_buf + sizeof(uint32_t), buf, size);

		send(client_sockfd, resp_buf, resp_len, 0);
		free(resp_buf);
	} else {
		send(client_sockfd, &size, sizeof(uint32_t), 0);
	}
}

int vmis_cb_read_virtual_memory(uint64_t va, uint32_t len, uint8_t *buffer)
{
	set_last_callback(VMI_CB_READ_VIRTUAL_MEMORY, va, len, 0);

	static const uint8_t memory[48] = {
		0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
		0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f,
		0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2a, 0x2b, 0x2c, 0x2d, 0x2e, 0x2f
	};

	static const uint64_t base_va = 0x4000;
	if (va < base_va || va >= base_va + sizeof(memory)) {
		return -1;
	}
	if (va + len >= base_va + sizeof(memory)) {
		return -1;
	}

	int offset = va - base_va;
	memcpy(buffer, memory + offset, len);

	return 0; // good
}

int vmis_cb_read_register(int32_t reg_group, int32_t reg_id, uint64_t *reg_val)
{
	set_last_callback(VMI_CB_READ_REGISTER, (uint32_t)reg_group, (uint32_t)reg_id, 0);

	fill_buffer(reg_val, sizeof(reg_val));

	return cb_value_to_return;
}

int vmis_cb_set_breakpoint(uint64_t va)
{
	set_last_callback(VMI_CB_SET_BREAKPOINT, va, 0, 0);

	return cb_value_to_return;
}

// Dummy functions required for compilation

void vmis_cb_enable_sync_wait()
{
	set_last_callback(VMI_CB_ENABLE_SYNC_WAIT, 0, 0, 0);

	return;
}

void vmis_cb_disable_sync_wait()
{
	set_last_callback(VMI_CB_DISABLE_SYNC_WAIT, 0, 0, 0);

	return;
}

int vmis_cb_remove_breakpoint(uint64_t va)
{
	set_last_callback(VMI_CB_REMOVE_BREAKPOINT, va, 0, 0);

	return cb_value_to_return;
}

int vmis_cb_remove_all_breakpoints(void)
{
	set_last_callback(VMI_CB_REMOVE_ALL_BREAKPOINTS, 0, 0, 0);

	return cb_value_to_return;
}

int vmis_cb_set_watchpoint(uint64_t va, uint32_t len, int wp)
{
	set_last_callback(VMI_CB_SET_WATCHPOINT, va, len, (uint32_t)wp);

	return cb_value_to_return;
}

int vmis_cb_remove_watchpoint(uint64_t va, uint32_t len)
{
	set_last_callback(VMI_CB_REMOVE_WATCHPOINT, va, len, 0);

	return cb_value_to_return;
}

int vmis_cb_remove_all_watchpoints(void)
{
	set_last_callback(VMI_CB_REMOVE_ALL_WATCHPOINTS, 0, 0, 0);

	return cb_value_to_return;
}

int vmis_cb_pause_vm(void)
{
	set_last_callback(VMI_CB_PAUSE_VM, 0, 0, 0);

	return cb_value_to_return;
}

int vmis_cb_step_vm(void)
{
	set_last_callback(VMI_CB_STEP_VM, 0, 0, 0);

	return cb_value_to_return;
}

int vmis_cb_continue_async_vm(void)
{
	set_last_callback(VMI_CB_CONTINUE_ASYNC_VM, 0, 0, 0);

	return cb_value_to_return;
}

CallbackCalled get_last_callback()
{
	return last_callback;
}

void set_last_callback(CallbackFunction func, uint64_t param_0, uint64_t param_1, uint64_t param_2) {
	last_callback.function = func;
	last_callback.params[0] = param_0;
	last_callback.params[1] = param_1;
	last_callback.params[2] = param_2;
}

void reset_last_callback()
{
	last_callback.function = VMI_CB_NONE;
	memset(last_callback.params, 0, sizeof(last_callback.params));
}

void set_cb_return_value(int err)
{
	cb_value_to_return = err;
}

void fill_buffer(void* buffer, size_t size)
{
	uint8_t* buf = (uint8_t*)buffer;
	for (uint64_t i=0; i < size; ++i)
		buf[i] = i;
}

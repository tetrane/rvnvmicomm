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
// #include <rvnvmicomm_server/vmiserver.h>
#include <rvnvmicomm_client/vmiclient.h>

extern void handle_request(const vmi_request_t *req);

// implement server support functions

static int server_sockfd = -1;
static int client_sockfd = -1;
static pthread_t vmiserver_thread;

static void handle_connection(int fd)
{
	client_sockfd = fd;

	vmi_request_t req;
	for (;;) {
		if (recv(fd, &req, sizeof(req), 0) != sizeof(req)) {
			return;
		}
		handle_request(&req);
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

void enable_sync_wait()
{
	return;
}

void disable_sync_wait()
{
	return;
}

void put_response(const uint8_t *buf, uint32_t size)
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

int read_virtual_memory(uint64_t va, uint32_t len, uint8_t *buffer)
{
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

int read_register(int32_t reg_group, int32_t reg_id, uint64_t *reg_val)
{
	typedef struct {
		uint64_t rax;
		uint64_t rcx;
		uint64_t rdx;
		uint64_t rbx;
		uint64_t rsp;
		uint64_t rbp;
		uint64_t rsi;
		uint64_t rdi;
		uint64_t r8;
		uint64_t r9;
		uint64_t r10;
		uint64_t r11;
		uint64_t r12;
		uint64_t r13;
		uint64_t r14;
		uint64_t r15;

		uint16_t es;
		uint16_t cs;
		uint16_t ss;
		uint16_t ds;
		uint16_t fs;
		uint16_t gs;

		uint64_t cr0;
		uint64_t cr1;
		uint64_t cr2;
		uint64_t cr3;
		uint64_t cr4;

		uint64_t msr_lstar;
		uint64_t msr_gsbase;
		uint64_t msr_kernelgsbase;
	} AMD64Cpu;

	AMD64Cpu cpu = {
		.rax = 0x0a,
		.rcx = 0x0c,
		.rdx = 0x0d,
		.rbx = 0x0b,
		.rsp = 0x0e,
		.rbp = 0x0f,
		.cr0 = 0xc0,
		.cr1 = 0xc1,
		.cr2 = 0xc2,
		.cr3 = 0xc3,
		.cr4 = 0xc4
	};

	if (reg_group < (vmi_x86_register_group_t)GP) {
		return -1;
	}
	if (reg_group > (vmi_x86_register_group_t)MSR) {
		return -1;
	}

	if (reg_group == GP) {
		switch (reg_id)
		{
		case RAX:
			*reg_val = cpu.rax;
			break;

		case RCX:
			*reg_val = cpu.rcx;
			break;

		case RDX:
			*reg_val = cpu.rdx;
			break;

		case RSP:
			*reg_val = cpu.rsp;
			break;

		case RBP:
			*reg_val = cpu.rbp;
			break;

		default:
			return -1;
		}

		return sizeof(uint64_t);
	}

	if (reg_group == CTRL) {
		switch (reg_id)
		{
		case CR0:
			*reg_val = cpu.cr0;
			break;

		case CR2:
			*reg_val = cpu.cr2;
			break;

		case CR3:
			*reg_val = cpu.cr3;
			break;

		case CR4:
			*reg_val = cpu.cr4;
			break;

		default:
			return -1;
		}

		return sizeof(uint64_t);
	}

	return -1;
}

int set_breakpoint(uint64_t va)
{
	return -1;
}

int vmiserver_start(const char *device)
{
	server_sockfd = socket(AF_UNIX, SOCK_STREAM, 0);
	if (server_sockfd < 0) {
		return -1; // error
	}

	struct sockaddr_un sock_addr = {
		.sun_family = AF_UNIX,
	};
	snprintf(sock_addr.sun_path, sizeof(sock_addr.sun_path), "%s", device);
	unlink(device);
	if (bind(server_sockfd, (struct sockaddr*)&sock_addr, sizeof(sock_addr)) != 0) {
		close(server_sockfd);
		return -2; // error
	}

	if (listen(server_sockfd, 128) != 0) {
		close(server_sockfd);
		return -3; // error
	}

	if (pthread_create(&vmiserver_thread, NULL, vmiserver, NULL) !=0) {
		close(server_sockfd);
		return -4; // error
	}
	pthread_join(vmiserver_thread, NULL);

	unlink(device);
	return 0; // good
}

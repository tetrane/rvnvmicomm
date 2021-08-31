#pragma once

#include <stdint.h>

typedef enum x86_register_group_t {
	GP,
	PC,
	SEG,
	CTRL,
	MSR,
} vmi_x86_register_group_t;

typedef enum x86_register_t {
	// general purpose
	RAX = 0,
	RCX,
	RDX,
	RBX,
	RSP,
	RBP,
	RSI,
	RDI,
	R8,
	R9,
	R10,
	R11,
	R12,
	R13,
	R14,
	R15,
	EFLAGS,
	PKRU,

	// TR
	GDTR_BASE,
	GDTR_LIMIT,
	LDTR_BASE,
	LDTR_LIMIT,
	IDTR_BASE,
	IDTR_LIMIT,

	// segment
	ES = 0,
	CS,
	SS,
	DS,
	FS,
	GS,
	ES_SHADOW,
	CS_SHADOW,
	SS_SHADOW,
	DS_SHADOW,
	FS_SHADOW,
	GS_SHADOW,

	// control
	CR0 = 0,
	CR1, // unused
	CR2,
	CR3,
	CR4,

	// MSR
	MSR_EFER = 0xc0000080,
	MSR_LSTAR = 0xc0000082,
	MSR_FSBASE = 0xc0000100,
	MSR_GSBASE = 0xc0000101,
	MSR_KERNELGSBASE = 0xc0000102,
} vmi_x86_register_t;

typedef enum request_type_t {
	// break/watch points (set/remove)
	BP = 0x10,
	WP_READ = 0x01,
	WP_WRITE = 0x02,
	WP_ACCESS = WP_READ | WP_WRITE,

	// memory/registers (read only)
	MEM_READ,
	REG_READ,

	// CPUID attributes (read only, static)
	ATT_READ,

	// continue, pause, step
	EXEC,
} vmi_request_type_t;

typedef enum request_action_t {
	SET,
	REM,
	REM_ALL,

	PAUSE,
	STEP,
	CONTINUE,
	CONTINUE_ASYNC,
} vmi_request_action_t;

typedef struct __attribute__((__packed__)) request_t {
	struct __attribute__((__packed__)) {
		uint32_t request_type;
		uint32_t request_action;
	};
	struct __attribute__((__packed__)) {
		union {
			uint64_t address;
			struct __attribute__((__packed__)) {
				uint32_t register_group;
				uint32_t register_id;
			};
		};
		uint32_t memory_size;
	} request_data;
} vmi_request_t;

typedef struct __attribute__((__packed__)) cpuid_values_t {
	uint32_t pat;
	uint32_t pse36;
	uint32_t pages_1gb;
	uint32_t max_phy_addr;
	uint32_t max_lin_addr;
} vmi_cpuid_values_t;

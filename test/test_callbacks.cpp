#define BOOST_TEST_MODULE RVN_VMICOMM_CALLBACKS
#include <boost/test/unit_test.hpp>

#include <rvnvmicomm_client/vmiclient.h>
#include <rvnvmicomm_server/vmiserver.h>
#include <rvnvmicomm_common/reven-vmi.h>

#include "test_common.h"
#include "test_server_implem.h"

#include <unistd.h>

void check_filled_buffer(void* buffer, size_t size)
{
	uint8_t* buf = (uint8_t*)buffer;
	for (uint64_t i=0; i < size; ++i)
		BOOST_CHECK_EQUAL(buf[i], i & 0xff);
}

void check_last_callback(CallbackFunction func, uint64_t param_0 = 0, uint64_t param_1 = 0, uint64_t param_2 = 0)
{
	auto cb = get_last_callback();

	BOOST_CHECK_EQUAL(cb.function, func);
	BOOST_CHECK_EQUAL(cb.params[0], param_0);
	BOOST_CHECK_EQUAL(cb.params[1], param_1);
	BOOST_CHECK_EQUAL(cb.params[2], param_2);
}

BOOST_FIXTURE_TEST_CASE(test_read_memory, VmiClientServerFixture)
{
	int err;

	uint64_t addr = 0xfffff00012345678;
	uint8_t buffer[0x10000];

	set_cb_return_value(0); // Callback read memory 0 = OK

	// Nominal read
	memset(buffer, 0, sizeof(buffer));
	err = vmic_read_physical_memory(cfd(), addr, 0x10, buffer);
	BOOST_CHECK_EQUAL(err, 0);
	check_last_callback(VMI_CB_READ_PHYSICAL_MEMORY, addr, 0x10);
	check_filled_buffer(buffer, 0x10);

	// Big read
	memset(buffer, 0, sizeof(buffer));
	err = vmic_read_physical_memory(cfd(), addr, 0x10000, buffer);
	BOOST_CHECK_EQUAL(err, 0);
	check_last_callback(VMI_CB_READ_PHYSICAL_MEMORY, addr, 0x10000);
	check_filled_buffer(buffer, 0x10000);

	// Read error
	set_cb_return_value(-1);
	err = vmic_read_physical_memory(cfd(), addr, 0x10, buffer);
	BOOST_CHECK_NE(err, 0);
	BOOST_CHECK_EQUAL(err, MEMORY_READ_FAILED);

	// Nominal read still works after error
	set_cb_return_value(0);
	memset(buffer, 0, sizeof(buffer));
	err = vmic_read_physical_memory(cfd(), addr, 0x10, buffer);
	BOOST_CHECK_EQUAL(err, 0);
	check_last_callback(VMI_CB_READ_PHYSICAL_MEMORY, addr, 0x10);
	check_filled_buffer(buffer, 0x10);
}

BOOST_FIXTURE_TEST_CASE(test_read_register, VmiClientServerFixture)
{
	int err;
	uint64_t result = 0;

	set_cb_return_value(8); // Callback register reads 8 bytes

	// Nominal cases
	err = vmic_read_register(cfd(), GP, RAX, &result);
	BOOST_CHECK_EQUAL(err, 0);
	check_last_callback(VMI_CB_READ_REGISTER, GP, RAX);
	check_filled_buffer(&result, sizeof(result)); // Buffer is correctly passed through pipe, tested only once.

	// Other value
	err = vmic_read_register(cfd(), GP, RCX, &result);
	BOOST_CHECK_EQUAL(err, 0);
	check_last_callback(VMI_CB_READ_REGISTER, GP, RCX);

	// Possibly negative argument
	err = vmic_read_register(cfd(), MSR, MSR_KERNELGSBASE, &result);
	BOOST_CHECK_EQUAL(err, 0);
	check_last_callback(VMI_CB_READ_REGISTER, MSR, MSR_KERNELGSBASE);

	// Segments are 32-bits
	result = 0;
	set_cb_return_value(4);
	err = vmic_read_register(cfd(), SEG, SS, &result);
	BOOST_CHECK_EQUAL(err, 0);
	check_last_callback(VMI_CB_READ_REGISTER, SEG, SS);
	check_filled_buffer(&result, sizeof(uint32_t));

	// Invalid registers
	err = vmic_read_register(cfd(), GP, MSR_KERNELGSBASE + 1, &result);
	BOOST_CHECK_NE(err, 0);
	err = vmic_read_register(cfd(), MSR + 1, MSR_KERNELGSBASE, &result);
	BOOST_CHECK_NE(err, 0);

	// Callback error
	set_cb_return_value(0);
	err = vmic_read_register(cfd(), GP, MSR_KERNELGSBASE + 1, &result);
	BOOST_CHECK_NE(err, 0);
	// BOOST_CHECK_EQUAL(err, REGISTER_READ_FAILED); // TODO: should be this, is DATA_BAD_REQUEST instead.

	// Nominal case still works after error
	set_cb_return_value(8);
	err = vmic_read_register(cfd(), GP, RAX, &result);
	BOOST_CHECK_EQUAL(err, 0);
	check_last_callback(VMI_CB_READ_REGISTER, GP, RAX);
}


BOOST_FIXTURE_TEST_CASE(test_read_register_size_failure, VmiClientServerFixture)
{
	int err;
	uint64_t result = 0;

	// Segments are 32-bits
	set_cb_return_value(8);
	err = vmic_read_register(cfd(), SEG, SS, &result);
	BOOST_CHECK_NE(err, 0);

	// Note: this is in its own separate tests, because once the client library receives bad data it cannot properly
	// communicate with the server, i.e. the following test would now fail:
	// set_cb_return_value(4);
	// err = vmic_read_register(cfd(), SEG, SS, &result);
	// BOOST_CHECK_EQUAL(err, 0);
	// This is a TODO
}

BOOST_FIXTURE_TEST_CASE(test_other_callbacks, VmiClientServerFixture)
{
	int err;
	uint64_t va = 0xfffff00012345678;

	// All other callbacks that do not return data

	// All OK
	set_cb_return_value(0);

	BOOST_CHECK_EQUAL(vmic_set_breakpoint(cfd(), va), 0);
	check_last_callback(VMI_CB_SET_BREAKPOINT, va);

	BOOST_CHECK_EQUAL(vmic_remove_breakpoint(cfd(), va), 0);
	check_last_callback(VMI_CB_REMOVE_BREAKPOINT, va);

	BOOST_CHECK_EQUAL(vmic_remove_all_breakpoints(cfd()), 0);
	check_last_callback(VMI_CB_REMOVE_ALL_BREAKPOINTS);

	BOOST_CHECK_EQUAL(vmic_set_read_watchpoint(cfd(), va, 0x10), 0);
	check_last_callback(VMI_CB_SET_WATCHPOINT, va, 0x10, WP_READ);

	BOOST_CHECK_EQUAL(vmic_set_write_watchpoint(cfd(), va, 0x10), 0);
	check_last_callback(VMI_CB_SET_WATCHPOINT, va, 0x10, WP_WRITE);

	BOOST_CHECK_EQUAL(vmic_remove_watchpoint(cfd(), va), 0);
	check_last_callback(VMI_CB_REMOVE_WATCHPOINT, va);

	// TODO: Missing call for remove all watchpoints

	BOOST_CHECK_EQUAL(vmic_pause_vm(cfd()), 0);
	check_last_callback(VMI_CB_PAUSE_VM);

	// TODO: not testable, no mechanism on the server side allows explicitely to release the server
	// apart from calling `put_typed_response` manually.
	// BOOST_CHECK_EQUAL(vmic_step_vm(cfd()), 0);
	// check_last_callback(VMI_CB_STEP_VM);

	// Same. Besides, will require a way in the test server to check that sync callbacks have been called
	// BOOST_CHECK_EQUAL(vmic_continue_vm(cfd()), 0);
	// check_last_callback(VMI_CB_CONTINUE_ASYNC_VM);

	BOOST_CHECK_EQUAL(vmic_continue_vm_async(cfd()), 0);
	check_last_callback(VMI_CB_CONTINUE_ASYNC_VM);

	// All errors
	set_cb_return_value(-1);

	BOOST_CHECK_EQUAL(vmic_set_breakpoint(cfd(), va), BREAKPOINT_SET_FAILED);

	BOOST_CHECK_EQUAL(vmic_remove_breakpoint(cfd(), va), BREAKPOINT_REMOVE_FAILED);

	BOOST_CHECK_EQUAL(vmic_remove_all_breakpoints(cfd()), BREAKPOINT_REMOVE_ALL_FAILED);

	BOOST_CHECK_EQUAL(vmic_set_read_watchpoint(cfd(), va, 0x10), WATCHPOINT_READ_SET_FAILED);

	BOOST_CHECK_EQUAL(vmic_set_write_watchpoint(cfd(), va, 0x10), WATCHPOINT_WRITE_SET_FAILED);

	BOOST_CHECK_EQUAL(vmic_remove_watchpoint(cfd(), va), WATCHPOINT_REMOVE_FAILED);

	// TODO: Missing call for remove all watchpoints

	BOOST_CHECK_EQUAL(vmic_pause_vm(cfd()), EXECUTION_PAUSE_FAILED);

	BOOST_CHECK_EQUAL(vmic_step_vm(cfd()), EXECUTION_STEP_FAILED);

	BOOST_CHECK_EQUAL(vmic_continue_vm(cfd()), EXECUTION_CONTINUE_FAILED);

	BOOST_CHECK_EQUAL(vmic_continue_vm_async(cfd()), EXECUTION_CONTINUE_ASYNC_FAILED);
}


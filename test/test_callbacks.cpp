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

	uint32_t n = 0;
	err = vmic_read_memory(cfd(), 0x4000, sizeof(n), (uint8_t *)&n);
	BOOST_REQUIRE_EQUAL(err, 0);
	BOOST_CHECK_EQUAL(n, 0x03020100);

	uint64_t m = 0;
	err = vmic_read_memory(cfd(), 0x4010, sizeof(m), (uint8_t *)&m);
	BOOST_REQUIRE_EQUAL(err, 0);
	BOOST_CHECK_EQUAL(m, 0x1716151413121110);
}

BOOST_FIXTURE_TEST_CASE(test_read_register, VmiClientServerFixture)
{
	int err;
	uint64_t result = 0;

	set_cb_return_value(8); // Callback register reads 8 bytes

	// Nominal cases
	err = vmic_read_register(cfd(), GP, RAX, &result);
	BOOST_REQUIRE_EQUAL(err, 0);
	check_last_callback(VMI_CB_READ_REGISTER, GP, RAX);
	check_filled_buffer(&result, sizeof(result)); // Buffer is correctly passed through pipe, tested only once.

	// Other value
	err = vmic_read_register(cfd(), GP, RCX, &result);
	BOOST_REQUIRE_EQUAL(err, 0);
	check_last_callback(VMI_CB_READ_REGISTER, GP, RCX);

	// Possibly negative argument
	err = vmic_read_register(cfd(), MSR, MSR_KERNELGSBASE, &result);
	BOOST_REQUIRE_EQUAL(err, 0);
	check_last_callback(VMI_CB_READ_REGISTER, MSR, MSR_KERNELGSBASE);

	// Segments are 32-bits
	result = 0;
	set_cb_return_value(4);
	err = vmic_read_register(cfd(), SEG, SS, &result);
	BOOST_REQUIRE_EQUAL(err, 0);
	check_last_callback(VMI_CB_READ_REGISTER, SEG, SS);
	check_filled_buffer(&result, sizeof(uint32_t));

	// Invalid registers
	err = vmic_read_register(cfd(), GP, MSR_KERNELGSBASE + 1, &result);
	BOOST_REQUIRE_NE(err, 0);
	err = vmic_read_register(cfd(), MSR + 1, MSR_KERNELGSBASE, &result);
	BOOST_REQUIRE_NE(err, 0);

	// Callback error
	set_cb_return_value(0);
	err = vmic_read_register(cfd(), GP, MSR_KERNELGSBASE + 1, &result);
	// BOOST_REQUIRE_EQUAL(err, REGISTER_READ_FAILED);

	// Nominal case still works
	set_cb_return_value(8);
	err = vmic_read_register(cfd(), GP, RAX, &result);
	BOOST_REQUIRE_EQUAL(err, 0);
	check_last_callback(VMI_CB_READ_REGISTER, GP, RAX);
}


BOOST_FIXTURE_TEST_CASE(test_read_register_size_failure, VmiClientServerFixture)
{
	int err;
	uint64_t result = 0;

	// Segments are 32-bits
	set_cb_return_value(8);
	err = vmic_read_register(cfd(), SEG, SS, &result);
	BOOST_REQUIRE_NE(err, 0);

	// Note: this is in its own separate tests, because once the client library receives bad data it cannot properly
	// communicate with the server, i.e. the following test would now fail:
	// set_cb_return_value(4);
	// err = vmic_read_register(cfd(), SEG, SS, &result);
	// BOOST_REQUIRE_EQUAL(err, 0);
}

BOOST_FIXTURE_TEST_CASE(test_start_stop_vmi, VmiClientServerFixture)
{
	int err;

	// dummy request just to notify the test server to stop
	// err = vmic_continue_vm_async(cfd());
	// BOOST_CHECK_NE(err, 0);
}


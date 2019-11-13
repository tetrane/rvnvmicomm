#define BOOST_TEST_MODULE RVN_VMICOMM_CALLBACKS
#include <boost/test/unit_test.hpp>

#include <rvnvmicomm_client/vmiclient.h>
#include <rvnvmicomm_server/vmiserver.h>
#include <rvnvmicomm_common/reven-vmi.h>

#include "test_common.h"
#include "test_server_implem.h"

#include <unistd.h>

BOOST_AUTO_TEST_CASE(test_read_memory)
{
	path_creator path_creator;
	int err;

	err = vmis_start(path_creator.path());
	BOOST_REQUIRE_EQUAL(err, 0);

	while (access(path_creator.path(), F_OK) != 0) {
		usleep(500);
	}

	int fd = -1;
	err = vmic_connect(path_creator.path(), &fd);
	BOOST_REQUIRE_EQUAL(err, 0);

	uint32_t n = 0;
	err = vmic_read_memory(fd, 0x4000, sizeof(n), (uint8_t *)&n);
	BOOST_REQUIRE_EQUAL(err, 0);
	BOOST_CHECK_EQUAL(n, 0x03020100);

	uint64_t m = 0;
	err = vmic_read_memory(fd, 0x4010, sizeof(m), (uint8_t *)&m);
	BOOST_REQUIRE_EQUAL(err, 0);
	BOOST_CHECK_EQUAL(m, 0x1716151413121110);

	err = vmic_close(fd);
	BOOST_REQUIRE_EQUAL(err, 0);

	test_server_close();
}

BOOST_AUTO_TEST_CASE(test_read_register)
{
	path_creator path_creator;
	int err;

	err = vmis_start(path_creator.path());
	BOOST_REQUIRE_EQUAL(err, 0);

	while (access(path_creator.path(), F_OK) != 0) {
		usleep(500);
	}

	int fd = -1;
	err = vmic_connect(path_creator.path(), &fd);
	BOOST_REQUIRE_EQUAL(err, 0);

	uint64_t rax = 0;
	err = vmic_read_register(fd, (uint32_t)GP, (uint32_t)RAX, &rax);
	BOOST_REQUIRE_EQUAL(err, 0);
	BOOST_CHECK_EQUAL(rax, 0x0a);

	err = vmic_close(fd);
	BOOST_CHECK_EQUAL(err, 0);

	test_server_close();
}

BOOST_AUTO_TEST_CASE(test_start_stop_vmi)
{
	path_creator path_creator;
	int err;

	err = vmis_start(path_creator.path());
	BOOST_REQUIRE_EQUAL(err, 0);

	while (access(path_creator.path(), F_OK) != 0) {
		usleep(500);
	}

	int fd = -1;
	err = vmic_connect(path_creator.path(), &fd);
	BOOST_REQUIRE_EQUAL(err, 0);

	// dummy request just to notify the test server to stop
	err = vmic_continue_vm_async(fd);
	BOOST_CHECK_NE(err, 0);

	err = vmic_close(fd);
	BOOST_REQUIRE_EQUAL(err, 0);

	test_server_close();
}


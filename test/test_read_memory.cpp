#define BOOST_TEST_MODULE RVN_BINRESOURCE_METADATA
#include <boost/test/unit_test.hpp>

#include <rvnvmicomm_client/vmiclient.h>
#include <rvnvmicomm_server/vmiserver.h>
#include <rvnvmicomm_common/reven-vmi.h>

#include "test_common.h"

#include <unistd.h>

BOOST_AUTO_TEST_CASE(test_read_memory)
{
	path_creator path_creator;

	pid_t pid = fork();
	BOOST_REQUIRE(pid >= 0);
	int err;

	if (pid == 0) {
		err = vmis_start(path_creator.path());
		BOOST_REQUIRE(err == 0);
	} else {
		while (access(path_creator.path(), F_OK) != 0) {
			usleep(500);
		}

		int fd = -1;
		err = vmic_connect(path_creator.path(), &fd);
		BOOST_REQUIRE(err == 0);

		uint32_t n;
		err = vmic_read_memory(fd, 0x4000, sizeof(n), (uint8_t *)&n);
		BOOST_REQUIRE(err == 0);
		BOOST_CHECK(n == 0x03020100);

		uint64_t m;
		err = vmic_read_memory(fd, 0x4010, sizeof(m), (uint8_t *)&m);
		BOOST_REQUIRE(err == 0);
		BOOST_CHECK(m == 0x1716151413121110);

		err = vmic_close(fd);
		BOOST_REQUIRE(err == 0);
	}
}

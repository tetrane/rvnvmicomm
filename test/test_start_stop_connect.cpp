#define BOOST_TEST_MODULE RVN_BINRESOURCE_METADATA
#include <boost/test/unit_test.hpp>

#include <rvnvmicomm_client/vmiclient.h>
#include <rvnvmicomm_server/vmiserver.h>
#include <rvnvmicomm_common/reven-vmi.h>

#include "test_common.h"

#include <unistd.h>

BOOST_AUTO_TEST_CASE(test_start_stop_vmi)
{
	path_creator path_creator;

	pid_t pid = fork();
	BOOST_REQUIRE(pid >= 0);
	int err;

	if (pid == 0) {
		err = vmis_start(path_creator.path());
		BOOST_REQUIRE(err != 0);
	} else {
		while (access(path_creator.path(), F_OK) != 0) {
			usleep(500);
		}

		int fd = -1;
		err = vmic_connect(path_creator.path(), &fd);
		BOOST_REQUIRE(err == 0);

		// dummy request just to notify the test server to stop
		err = vmic_continue_vm_async(fd);
		BOOST_CHECK(err != 0);

		err = vmic_close(fd);
		BOOST_REQUIRE(err == 0);
	}
}


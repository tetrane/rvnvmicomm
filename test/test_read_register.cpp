#define BOOST_TEST_MODULE RVN_BINRESOURCE_METADATA
#include <boost/test/unit_test.hpp>

#include <rvnvmicomm_client/vmiclient.h>
#include <rvnvmicomm_server/vmiserver.h>
#include <rvnvmicomm_common/reven-vmi.h>

#include <unistd.h>

static const char socket_path[] = "/tmp/reven_vmi";

BOOST_AUTO_TEST_CASE(test_read_register)
{
	unlink(socket_path);

	pid_t pid = fork();
	BOOST_REQUIRE(pid >= 0);
	int err;

	if (pid == 0) {
		err = vmiserver_start(socket_path);
		BOOST_REQUIRE(err == 0);
	} else {
		while (access(socket_path, F_OK) != 0) {
			usleep(500);
		}

		int fd = -1;
		err = vmic_connect(socket_path, &fd);
		BOOST_REQUIRE(err == 0);

		uint64_t rax;
		err = vmic_read_register(fd, (uint32_t)GP, (uint32_t)RAX, &rax);
		BOOST_REQUIRE(err == 0);
		BOOST_CHECK(rax == 0x0a);

		err = vmic_close(fd);
		BOOST_CHECK(err == 0);
	}
}

#pragma once

#include <boost/test/unit_test.hpp>
#include <boost/filesystem.hpp>

#include <rvnvmicomm_server/vmiserver.h>

#include "test_server_implem.h"

struct VmiClientServerFixture {
	VmiClientServerFixture()
	  : temp_path_(boost::filesystem::unique_path(boost::filesystem::temp_directory_path() / "reven_vmi_%%%%%%").native())
	{
		int err;
		err = vmis_start(path());
		BOOST_REQUIRE_EQUAL(err, 0);

		int fd = -1;
		err = vmic_connect(path(), &client_fd_);
		BOOST_REQUIRE_EQUAL(err, 0);
	}

	~VmiClientServerFixture()
	{
		int err;
		err = vmic_close(cfd());
		BOOST_CHECK_EQUAL(err, 0);
		test_server_close();
		boost::filesystem::remove_all(temp_path_);
	}

	const char* path() const { return temp_path_.c_str(); }
	int cfd() const { return client_fd_; }

private:
	boost::filesystem::path temp_path_;
	int client_fd_;
};

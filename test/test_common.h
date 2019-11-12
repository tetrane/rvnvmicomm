#pragma once

#include <boost/filesystem.hpp>

struct path_creator {
	path_creator()
	  : temp_path(boost::filesystem::unique_path(boost::filesystem::temp_directory_path() / "reven_vmi_%%%%%%").native())
	{
	}

	~path_creator() { boost::filesystem::remove_all(temp_path); }

	const char* path() const { return temp_path.c_str(); }
private:
	boost::filesystem::path temp_path;
};

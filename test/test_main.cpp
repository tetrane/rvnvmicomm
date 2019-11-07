#define BOOST_TEST_MODULE RVN_BINRESOURCE_METADATA
#include <boost/test/unit_test.hpp>

extern "C" {
	#include <rvnvmicomm_client/vmiclient.h>
}

// TODO: Change this useless test by real tests
BOOST_AUTO_TEST_CASE(nothing)
{
	vmi_close(0);
}

#include <boost/test/unit_test.hpp>
#include <gstreamer_helpers/Dummy.hpp>

using namespace gstreamer_helpers;

BOOST_AUTO_TEST_CASE(it_should_not_crash_when_welcome_is_called)
{
    gstreamer_helpers::DummyClass dummy;
    dummy.welcome();
}

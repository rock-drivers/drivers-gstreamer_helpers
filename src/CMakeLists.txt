rock_library(gstreamer_helpers
    SOURCES Dummy.cpp
    HEADERS Dummy.hpp)

rock_executable(gstreamer_helpers_bin Main.cpp
    DEPS gstreamer_helpers)


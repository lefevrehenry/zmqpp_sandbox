# - Try to find ZMQ
# Once done this will define
# ZMQPP_FOUND - System has ZMQPP
# ZMQPP_INCLUDE_DIR - The ZMQPP include directories
# ZMQPP_LIBRARY - The libraries needed to use ZMQPP
# ZMQPP - the target to link with

find_path(ZMQPP_INCLUDE_DIR zmqpp/zmqpp.hpp)
find_library(ZMQPP_LIBRARY NAMES zmqpp)

include(FindPackageHandleStandardArgs)
# handle the QUIETLY and REQUIRED arguments and set ZMQPP_FOUND to TRUE
# if all listed variables are TRUE
find_package_handle_standard_args(
    ZMQPP
    DEFAULT_MSG
    ZMQPP_INCLUDE_DIR
    ZMQPP_LIBRARY
)

if(ZMQPP_FOUND)
    # Create imported target ZMQPP
    add_library(ZMQPP INTERFACE IMPORTED)

    set_target_properties(ZMQPP PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES "${ZMQPP_INCLUDE_DIR}"
        INTERFACE_LINK_LIBRARIES "${ZMQPP_LIBRARY}"
    )
endif()

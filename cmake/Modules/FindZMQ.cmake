# - Try to find ZMQ
# Once done this will define
# ZMQ_FOUND - System has ZMQ
# ZMQ_INCLUDE_DIR - The ZMQ include directories
# ZMQ_LIBRARY - The libraries needed to use ZMQ
# ZMQ - the target to link with

find_path(ZMQ_INCLUDE_DIR zmq.h)
find_library(ZMQ_LIBRARY NAMES zmq)

include(FindPackageHandleStandardArgs)
# handle the QUIETLY and REQUIRED arguments and set ZMQ_FOUND to TRUE
# if all listed variables are TRUE
find_package_handle_standard_args(
    ZMQ
    DEFAULT_MSG
    ZMQ_INCLUDE_DIR
    ZMQ_LIBRARY
)

if(ZMQ_FOUND)
    # Create imported target ZMQ
    add_library(ZMQ INTERFACE IMPORTED)

    set_target_properties(ZMQ PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES "${ZMQ_INCLUDE_DIR}"
        INTERFACE_LINK_LIBRARIES "${ZMQ_LIBRARY}"
    )
endif()

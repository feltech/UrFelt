# Set project name
project (UrFelt)
# Set minimum version
cmake_minimum_required (VERSION 2.8.6)
if (COMMAND cmake_policy)
	cmake_policy (SET CMP0003 NEW)
	if (CMAKE_VERSION VERSION_GREATER 2.8.12 OR CMAKE_VERSION VERSION_EQUAL 2.8.12)
		cmake_policy (SET CMP0022 NEW) # INTERFACE_LINK_LIBRARIES defines the link interface
	endif ()
	if (CMAKE_VERSION VERSION_GREATER 3.0.0 OR CMAKE_VERSION VERSION_EQUAL 3.0.0)
		cmake_policy (SET CMP0026 OLD) # Disallow use of the LOCATION target property - therefore we set to OLD as we still need it
		cmake_policy (SET CMP0042 NEW) # MACOSX_RPATH is enabled by default
		cmake_policy (SET CMP0063 NEW) # Honor visibility properties for all target types
	endif ()
endif ()


set (URHO3D_HOME "${PROJECT_SOURCE_DIR}/vendor/Urho3D")
set (CMAKE_MODULE_PATH "${URHO3D_HOME}/CMake/Modules")
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_custom_target(Moon ALL find bin/Data/Scripts/ -name "*.moon" -exec moonc {} \\\\;)

find_package(Boost
	1.58							# Minimum or EXACT version e.g. 1.36.0
	REQUIRED						# Fail with error if Boost is not found
	coroutine system	# Boost libraries by their canonical name
)

find_package(ZLIB REQUIRED)

# E.g. LLVM version is installed outside of standard include path.
find_package(OpenMP)

if (NOT OpenMP_CXX_FOUND)
    message(FATAL_ERROR "OpenMP not found")
endif()

# Add common C++ definitions.
add_definitions(
	-std=c++14 -Wfatal-errors
	# Enable OpenMP
    ${OpenMP_CXX_FLAGS}
	# Cannot use coroutine2 on Ubuntu 16.04 by default.
	-DBOOST_COROUTINES_NO_DEPRECATION_WARNING -DBOOST_COROUTINE_NO_DEPRECATION_WARNING
	# For LuaJIT exceptions
	-DSOL_EXCEPTIONS_SAFE_PROPAGATION
	# Enable sol::stack::userdata_checker
	-DSOL_ENABLE_INTEROP
    # For boost compatibility with clang7
    -Wno-argument-outside-range
	# Force enable debugging
# 	-O0
	# Enable extra debugging/profiling
#	-ggdb -g -fno-omit-frame-pointer
	# Enable Felt library exceptions
#	-DFELT_EXCEPTIONS
	# Enable profiling
	# sudo perf record -g --call-graph dwarf -- ./UrhoFelt
	# sudo perf report -g graph --no-children
)

# Include Urho3D Cmake common module
include (UrhoCommon)

# Set include folders
set (FELT_INCLUDE vendor/Felt/include)
set (LUACPPMSG_INCLUDE vendor/LuaCppMsg/include)
set (SML_INCLUDE vendor/sml/include)
set (LUAHPP_INCLUDE ${URHO3D_HOME}/Source/ThirdParty/LuaJIT/src)
set (CIMG_INCLUDE vendor/CImg)
set (ZSTR_INCLUDE vendor/zstr/src)
set (VENDOR_INCLUDE vendor/include)

include_directories (
	include ${FELT_INCLUDE} ${LUACPPMSG_INCLUDE} ${LUAHPP_INCLUDE}
	${SML_INCLUDE} ${VENDOR_INCLUDE} ${Boost_INCLUDE_DIRS} ${CIMG_INCLUDE} ${ZLIB_INCLUDE_DIRS}
	${ZSTR_INCLUDE}
	#/usr/include/x86_64-linux-gnu/c++/5
)

#FILE(GLOB_RECURSE HPPS "include/*")
#add_custom_target(headers SOURCES ${HPPS})

# Define target name
set (TARGET_NAME UrhoFelt)

# Define source files
file (GLOB_RECURSE CPP_FILES src/*.cpp)
file (GLOB_RECURSE H_FILES include/*.hpp)

# Remove strings matching given regular expression from a list.
# @param(in,out) aItems Reference of a list variable to filter.
# @param aRegEx Value of regular expression to match.
function (filter_items aItems aRegEx)
    # For each item in our list
    foreach (item ${${aItems}})
        # Check if our items matches our regular expression
        if ("${item}" MATCHES ${aRegEx})
            # Remove current item from our list
            list (REMOVE_ITEM ${aItems} ${item})
        endif ("${item}" MATCHES ${aRegEx})
    endforeach(item)
    # Provide output parameter
    set(${aItems} ${${aItems}} PARENT_SCOPE)
endfunction (filter_items)
# Strip build artifacts from list of source files.
filter_items(CPP_FILES "^\\.|/\\.|CMakeFiles")
filter_items(H_FILES "^\\.|/\\.|CMakeFiles")

set (
	SOURCE_FILES ${CPP_FILES}
)

# Setup target with resource copying
setup_main_executable ()

# Link to external libraries.
target_link_libraries(
	UrhoFelt
	# Enable profiling
#	-g -fno-omit-frame-pointer -ggdb
	atomic ${Boost_LIBRARIES} ${ZLIB_LIBRARIES} ${OpenMP_CXX_LIB_NAMES}
)

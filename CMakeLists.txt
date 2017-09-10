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
	endif ()
endif ()

set (URHO3D_HOME "vendor/Urho3D")

set (CMAKE_MODULE_PATH "${URHO3D_HOME}/CMake/Modules")

# Add common C++ definitions.
add_definitions(
	-std=c++14 -fopenmp -Wfatal-errors 
	-DBOOST_COROUTINES_NO_DEPRECATION_WARNING -DBOOST_COROUTINE_NO_DEPRECATION_WARNING
)
# -DFELT_EXCEPTIONS)

# Include Urho3D Cmake common module
include (UrhoCommon)


find_package(Boost
	1.58							# Minimum or EXACT version e.g. 1.36.0
	REQUIRED						# Fail with error if Boost is not found
	coroutine system	# Boost libraries by their canonical name
) 


# Set include folders
set (FELT_INCLUDE vendor/Felt/include)
set (LUACPPMSG_INCLUDE vendor/LuaCppMsg/include)
set (MSM_INCLUDE vendor/msm-lite/include)
set (LUAHPP_INCLUDE ${URHO3D_HOME}/Source/ThirdParty/LuaJIT/src)

include_directories (
	include ${FELT_INCLUDE} ${LUACPPMSG_INCLUDE} ${LUAHPP_INCLUDE}
	${MSM_INCLUDE} ${Boost_INCLUDE_DIRS} #/usr/include/x86_64-linux-gnu/c++/5
)

#FILE(GLOB_RECURSE HPPS "include/*")
#add_custom_target(headers SOURCES ${HPPS})

# Define target name
set (TARGET_NAME UrFelt)

# Define source files
file (GLOB CPP_FILES src/*.cpp)
file (GLOB LUABIND_FILES src/bind/*.cpp)
file (GLOB LUABIND_UTIL ${URHO3D_HOME}/Source/Urho3D/LuaScript/ToluaUtils.cpp)
file (GLOB H_FILES include/*.h)
set (
	SOURCE_FILES ${CPP_FILES} ${LUABIND_UTIL} ${LUABIND_FILES} ${H_FILES}
)

# Setup target with resource copying
setup_main_executable ()

# Link to external libraries.
target_link_libraries(UrFelt gomp atomic ${Boost_LIBRARIES})

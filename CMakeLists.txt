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

# Set CMake modules search path
set (URHO3D_HOME ../Urho3D CACHE PATH "Path to Urho3D")
set (FELT_HOME ../Felt CACHE PATH "Path to Felt")

set(Felt_DIR ${FELT_HOME})
set (CMAKE_MODULE_PATH "${URHO3D_HOME}/CMake/Modules" "${FELT_HOME}")

# Add common C++ definitions.
add_definitions(-std=c++14 -fopenmp -Wfatal-errors)

# Find Urho3D library
find_package (Urho3D REQUIRED)

# Find Felt library
find_package (Felt REQUIRED)

# Include Urho3D Cmake common module
include (Urho3D-CMake-common)

# Set include folders
set (LUACPPMSG_INCLUDE vendor/LuaCppMsg/include)
set (LUAHPP_INCLUDE ${URHO3D_HOME}/Source/ThirdParty/LuaJIT/src)

include_directories (
    ${FELT_INCLUDE_DIRS} ${URHO3D_INCLUDE_DIRS} include ${LUACPPMSG_INCLUDE} ${LUAHPP_INCLUDE}
)

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
target_link_libraries(UrFelt gomp atomic)

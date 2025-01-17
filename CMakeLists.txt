cmake_minimum_required(VERSION 3.22)

IF(CMAKE_GENERATOR_PLATFORM STREQUAL "Switch")
	set(DEVKITPRO $ENV{DEVKITPRO})
	IF(DEVKITPRO)
		message(STATUS "Using DevKitPro")
		list(APPEND CMAKE_MODULE_PATH "$ENV{DEVKITPRO}/cmake")
		include(Switch)
		include(Platform/NintendoSwitch)
	ENDIF()
ENDIF()

#set(CMAKE_CXX_CLANG_TIDY clang-tidy;-checks=cppcoreguidelines-*;)
set(FETCHCONTENT_QUIET FALSE)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_FIND_LIBRARY_SUFFIXES ".a")

project(SymphonyOfEmpires CXX C)
include(FetchContent)

option(BUILD_ENGINE "Enable building Eng3D" ON)
option(BUILD_GAME "Enable building SymphonyOfEmpires" ON)
option(SOE_UBSAN "Enable UBSAN instrumentation" OFF)

IF(SOE_UBSAN)
	add_compile_options(-fsanitize=undefined)
	add_link_options(-fsanitize=undefined)
ENDIF()

IF(WIN32)
	add_definitions(-DNOMINMAX=1)
	add_definitions(-DWIN32_LEAN_AND_MEAN=1)
ELSEIF(ANDROID)
	set(NDK_PROJECT_PATH $ENV{PWD}) # Set project path to current directory
	set(ANDROID_STL c++_static) # Static libc++
	#IF(NOT DEFINED ANDROID_HOME)
	#	message(FATAL_ERROR "Please define ANDROID_NDK_HOME")
	#ENDIF()
	#IF(NOT DEFINED ANDROID_NDK_HOME)
	#	message(FATAL_ERROR "Please define ANDROID_NDK_HOME")
	#ENDIF()
	add_compile_options(-fPIE -fPIC)
	add_link_options(-fPIE -pie)
ENDIF()

IF(NO_COPY_MODS)
	add_compile_definitions(NO_COPY_MODS)
ENDIF()
IF(SANITIZE_ON)
	add_compile_options(-fsanitize=undefined -fsanitize=thread)
ENDIF()

# Backend selection
IF(E3D_BACKEND_GLES)
	add_compile_definitions(E3D_BACKEND_GLES)
ELSEIF(E3D_BACKEND_D3D9)
	IF(NOT WIN32)
		message(FATAL_ERROR "DirectX 9 is not supported in ${CMAKE_SYSTEM_NAME} platform")
	ENDIF()
	message("Using DirectX 9 SDK")
	add_compile_definitions(E3D_BACKEND_D3D9)
ELSE()
	IF(NOT E3D_BACKEND_OPENGL)
		message(WARNING "No backend specified - using OpenGL as default")
		set(E3D_BACKEND_OPENGL ON)
	ENDIF()
	IF(CMAKE_SYSTEM_NAME STREQUAL "NintendoSwitch" OR ANDROID)
		message(FATAL_ERROR "OpenGL is not supported in ${CMAKE_SYSTEM_NAME} platform, try GLES instead")
	ENDIF()
	message("Using OpenGL backend")
	IF(ANDROID)
		message(FATAL_ERROR "Use E3D_BACKEND_GLES instead on Android")
	ENDIF()
	add_compile_definitions(E3D_BACKEND_OPENGL)
ENDIF()

#
# Dependencies
#
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
add_subdirectory(dependencies)

#
# Compilation flags
#

IF (CMAKE_BUILD_TYPE STREQUAL "Release")
	IF(NOT MSVC)
		add_compile_options("-O3")
	ELSE()
		add_compile_options("/O2")
	ENDIF()
ENDIF()
IF(NOT MSVC)
	add_compile_options(-Wall -Wextra -Wpedantic -Wshadow)
ENDIF()
IF(UNIT_TEST)
	add_compile_definitions(UNIT_TEST)
ENDIF()

IF(WIN32)
	add_compile_definitions(E3D_TARGET_WINDOWS)
ELSEIF(UNIX)
	add_compile_definitions(E3D_TARGET_UNIX)
ELSEIF(ANDROID)
	add_compile_definitions(E3D_TARGET_UNIX E3D_TARGET_ANDROID)
	add_compile_definitions(E3D_HANDHELD)
ELSEIF(CMAKE_SYSTEM_NAME STREQUAL "NetBSD")
	add_compile_definitions(E3D_TARGET_UNIX)
	add_compile_definitions(E3D_TARGET_NETBSD)
ELSEIF(CMAKE_SYSTEM_NAME STREQUAL "NintendoSwitch")
	include_directories("${DEVKITPRO}/portlibs/switch/include/")
	add_compile_definitions(E3D_TARGET_UNIX E3D_TARGET_SWITCH)
	add_compile_definitions(E3D_HANDHELD)
	# Compiler is for 32bits and doesn't support long long
	add_compile_definitions(LUA_32BITS)
ELSEIF(CMAKE_SYSTEM_NAME STREQUAL "NintendoWii")
	link_directories("${DEVKITPRO}/libogc/lib")
	include_directories("${DEVKITPRO}/libogc/include")
	add_compile_definitions(E3D_TARGET_RVL)
ENDIF()

include_directories("${PROJECT_SOURCE_DIR}/game/src" "${CMAKE_SOURCE_DIR}/eng3d")

#
# Source code files
#

# C++ source files
file(GLOB_RECURSE MAIN_SOURCES
	"${PROJECT_SOURCE_DIR}/game/src/*.cpp"
)
IF(ANDROID)
	list(APPEND MAIN_SOURCES "${CMAKE_ANDROID_NDK}/sources/android/native_app_glue/android_native_app_glue.c")
ENDIF()
IF(ANDROID)
	add_executable(SymphonyOfEmpires "${MAIN_SOURCES}" "${CMAKE_ANDROID_NDK}/sources/android/native_app_glue/android_native_app_glue.c")
ELSEIF(WIN32)
	if(MSVC)
	endif()
	# Build as a windows application
	add_executable(SymphonyOfEmpires "${MAIN_SOURCES}")
ELSEIF(CMAKE_SYSTEM_NAME STREQUAL "NintendoSwitch")
	target_include_directories(SymphonyOfEmpires PRIVATE "/opt/devkitpro/portlibs/switch/include/")
ELSE()
	# The rest is a normal application
	add_executable(SymphonyOfEmpires "${MAIN_SOURCES}")
ENDIF()

#
# Linking
#
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# Pthread
find_package(Threads)
IF(Threads_FOUND)
	set(THREADS_PREFER_PTHREAD_FLAG ON)
	target_link_libraries(SymphonyOfEmpires PRIVATE Threads::Threads)
ENDIF()

target_link_libraries(SymphonyOfEmpires PRIVATE
	dependency_tbb
	dependency_lua
)

# Build stuff
IF(BUILD_ENGINE)
    add_subdirectory(${CMAKE_SOURCE_DIR}/eng3d ${CMAKE_BINARY_DIR}/eng3d)
	add_dependencies(SymphonyOfEmpires eng3d)
ENDIF()

IF(WIN32)
	target_link_directories(SymphonyOfEmpires PRIVATE "${CMAKE_BINARY_DIR}")
	target_link_libraries(SymphonyOfEmpires PRIVATE eng3d)
	target_link_libraries(SymphonyOfEmpires PRIVATE wsock32 ws2_32 iphlpapi)
ELSE()
	target_link_libraries(SymphonyOfEmpires PRIVATE eng3d)
ENDIF()

IF(ANDROID)
	set(APP_SHARED_LIBRARIES ${LIBRARY_OUTPUT_PATH}/libtbb.so)
	list(APPEND APP_SHARED_LIBRARIES ${LIBRARY_OUTPUT_PATH}/libSDL2.so)
	list(APPEND APP_SHARED_LIBRARIES ${LIBRARY_OUTPUT_PATH}/liblua.so)
	list(APPEND APP_SHARED_LIBRARIES ${LIBRARY_OUTPUT_PATH}/libeng3d.so)
	list(APPEND APP_SHARED_LIBRARIES ${LIBRARY_OUTPUT_PATH}/libassimp.so)
	android_create_apk(SymphonyOfEmpires "${CMAKE_BINARY_DIR}/apk" "${APP_SHARED_LIBRARIES}" "" "mods")
ELSEIF(CMAKE_SYSTEM_NAME STREQUAL "NintendoSwitch")
	message(STATUS "Creating NRO file")
	set(NROFLAGS "")
	string(APPEND NROFLAGS " --romfsdir=${CMAKE_SOURCE_DIR}/mods")
	add_custom_command(
		OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/SymphonyOfEmpires.nro
		COMMAND ${NX_ELF2NRO_EXE} $<TARGET_FILE:SymphonyOfEmpires> ${CMAKE_CURRENT_BINARY_DIR}/SymphonyOfEmpires.nro ${NROFLAGS}
		DEPENDS SymphonyOfEmpires
		VERBATIM
	)
ENDIF()

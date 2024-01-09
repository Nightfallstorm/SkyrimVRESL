cmake_minimum_required(VERSION 3.20)
set(NAME "skyrimvresl")
set(VERSION 1.1.0)

# ---- Options ----
option(COPY_BUILD "Copy the build output to the Skyrim directory." FALSE)
option(BUILD_SKYRIMVR "Build for Skyrim VR" ON)
option(BACKWARDS_COMPATIBLE "Use TESDataHandler for inheritance" ON)
option(PACKAGE "Package into .7z" OFF)

# ---- Cache build vars ----
macro(set_from_environment VARIABLE)
	if(NOT DEFINED ${VARIABLE} AND DEFINED ENV{${VARIABLE}})
		set(${VARIABLE} $ENV{${VARIABLE}})
	endif()
endmacro()

macro(find_commonlib_path)
	if(CommonLibName AND NOT ${CommonLibName} STREQUAL "")
		# Check extern
		find_path(CommonLibPath
			include/REL/Relocation.h
			PATHS external/${CommonLibName}
			extern/${CommonLibName})

		if(${CommonLibPath} STREQUAL "CommonLibPath-NOTFOUND")
			# Check path
			set_from_environment(${CommonLibName}Path)
			set(CommonLibPath ${${CommonLibName}Path})
		endif()
	endif()
endmacro()

set_from_environment(VCPKG_ROOT)

add_compile_definitions(SKYRIMVR)
add_compile_definitions(_CRT_SECURE_NO_WARNINGS)
set(CommonLibName "CommonLibVR")
set_from_environment(SkyrimVRPath)
set(SkyrimPath ${SkyrimVRPath})
set(SkyrimVersion "Skyrim VR")

if(BACKWARDS_COMPATIBLE)
	add_compile_definitions(BACKWARDS_COMPATIBLE)
endif()

find_commonlib_path()
message(
	STATUS
	"Building ${NAME} ${VERSION} for ${SkyrimVersion} at ${SkyrimPath} with ${CommonLibName} at ${CommonLibPath}."
)

if(DEFINED VCPKG_ROOT)
	set(CMAKE_TOOLCHAIN_FILE "${VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake" CACHE STRING "")
	set(VCPKG_TARGET_TRIPLET "x64-windows-static" CACHE STRING "")
else()
	message(
		WARNING
		"Variable VCPKG_ROOT is not set. Continuing without vcpkg."
	)
endif()

set(Boost_USE_STATIC_RUNTIME OFF CACHE BOOL "")

# ---- Project ----
project(
	${NAME}
	VERSION ${VERSION}
	LANGUAGES CXX
)

configure_file(
	${CMAKE_CURRENT_SOURCE_DIR}/cmake/Version.h.in
	${CMAKE_CURRENT_BINARY_DIR}/include/Version.h
	@ONLY
)

configure_file(
	${CMAKE_CURRENT_SOURCE_DIR}/cmake/version.rc.in
	${CMAKE_CURRENT_BINARY_DIR}/version.rc
	@ONLY
)

# ---- Include guards ----
if(PROJECT_SOURCE_DIR STREQUAL PROJECT_BINARY_DIR)
	message(
		FATAL_ERROR
		"In-source builds not allowed. Please make a new directory (called a build directory) and run CMake from there."
	)
endif()

# ---- Globals ----
add_compile_definitions(
	SKSE_SUPPORT_XBYAK
)

if(MSVC)
	if(NOT ${CMAKE_GENERATOR} STREQUAL "Ninja")
		add_compile_options(
			/MP # Build with Multiple Processes
		)
	endif()
endif()

set(CMAKE_INTERPROCEDURAL_OPTIMIZATION ON)
set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_DEBUG OFF)

set(Boost_USE_STATIC_LIBS ON)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>" CACHE STRING "")

# ---- Dependencies ----
if(DEFINED CommonLibPath AND NOT ${CommonLibPath} STREQUAL "" AND IS_DIRECTORY ${CommonLibPath})
	add_subdirectory(${CommonLibPath} ${CommonLibName})
else()
	message(
		FATAL_ERROR
		"Variable ${CommonLibName}Path is not set."
	)
endif()

find_package(robin_hood CONFIG REQUIRED)
find_library(Detours_LIBRARIES NAMES detours.lib)
find_path(SIMPLEINI_INCLUDE_DIRS "ConvertUTF.c")

# ---- Add source files ----
include(cmake/headerlist.cmake)
include(cmake/sourcelist.cmake)

source_group(
	TREE
	${CMAKE_CURRENT_SOURCE_DIR}
	FILES
	${headers}
	${sources}
)

source_group(
	TREE
	${CMAKE_CURRENT_BINARY_DIR}
	FILES
	${CMAKE_CURRENT_BINARY_DIR}/include/Version.h
)

# ---- Create DLL ----
add_library(
	${PROJECT_NAME}
	SHARED
	${headers}
	${sources}
	${CMAKE_CURRENT_BINARY_DIR}/include/Version.h
	${CMAKE_CURRENT_BINARY_DIR}/version.rc
	.clang-format
)

target_compile_features(
	${PROJECT_NAME}
	PRIVATE
	cxx_std_20
)

target_compile_definitions(
	${PROJECT_NAME}
	PRIVATE
	_UNICODE
)

target_include_directories(
	${PROJECT_NAME}
	PRIVATE
	${CMAKE_CURRENT_BINARY_DIR}/include
	${CMAKE_CURRENT_SOURCE_DIR}/src
	${SIMPLEINI_INCLUDE_DIRS}
)

target_link_libraries(
	${PROJECT_NAME}
	PRIVATE
	${CommonLibName}::${CommonLibName}
	${Detours_LIBRARIES}
	robin_hood::robin_hood
)

target_precompile_headers(
	${PROJECT_NAME}
	PRIVATE
	src/PCH.h
)

if(MSVC)
	target_compile_options(
		${PROJECT_NAME}
		PRIVATE
		/sdl # Enable Additional Security Checks
		/utf-8 # Set Source and Executable character sets to UTF-8
		/Zi # Debug Information Format

		/permissive- # Standards conformance

		/Zc:alignedNew # C++17 over-aligned allocation
		/Zc:auto # Deduce Variable Type
		/Zc:char8_t
		/Zc:__cplusplus # Enable updated __cplusplus macro
		/Zc:externC
		/Zc:externConstexpr # Enable extern constexpr variables
		/Zc:forScope # Force Conformance in for Loop Scope
		/Zc:hiddenFriend
		/Zc:implicitNoexcept # Implicit Exception Specifiers
		/Zc:lambda
		/Zc:noexceptTypes # C++17 noexcept rules
		/Zc:preprocessor # Enable preprocessor conformance mode
		/Zc:referenceBinding # Enforce reference binding rules
		/Zc:rvalueCast # Enforce type conversion rules
		/Zc:sizedDealloc # Enable Global Sized Deallocation Functions
		/Zc:strictStrings # Disable string literal type conversion
		/Zc:ternary # Enforce conditional operator rules
		/Zc:threadSafeInit # Thread-safe Local Static Initialization
		/Zc:tlsGuards
		/Zc:trigraphs # Trigraphs Substitution
		/Zc:wchar_t # wchar_t Is Native Type

		/external:anglebrackets
		/external:W0

		/W4 # Warning level
		/WX # Warning level (warnings are errors)

		"$<$<CONFIG:DEBUG>:>"
		"$<$<CONFIG:RELEASE>:/Zc:inline;/JMC-;/Ob3>"
	)

	target_link_options(
		${PROJECT_NAME}
		PRIVATE
		/WX # Treat Linker Warnings as Errors

		"$<$<CONFIG:DEBUG>:/INCREMENTAL;/OPT:NOREF;/OPT:NOICF>"
		"$<$<CONFIG:RELEASE>:/INCREMENTAL:NO;/OPT:REF;/OPT:ICF;/DEBUG:FULL>"
	)

	# fix a warning with /Ob2 being overridden with /Ob3
	string(REPLACE "/Ob2" "/Ob3" CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE}")
endif()

# ---- Post build ----
if(COPY_BUILD)
	if(DEFINED SkyrimPath)
		string(REGEX REPLACE "[/\\][Dd][Aa][Tt][Aa][/\\]?$" "" SkyrimPath "${SkyrimPath}") # remove " /data " if exists; cmake regex doesn't have /i
		add_custom_command(
			TARGET ${PROJECT_NAME}
			POST_BUILD
			COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:${PROJECT_NAME}> ${SkyrimPath}/Data/SKSE/Plugins/
			COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_PDB_FILE:${PROJECT_NAME}> ${SkyrimPath}/Data/SKSE/Plugins/
		)
	else()
		message(
			WARNING
			"Variable ${SkyrimPath} is not defined. Skipping post-build copy command."
		)
	endif()
endif()

# ---- Create Package ----
if(PACKAGE)
	set(ZIP_DIR "${CMAKE_CURRENT_BINARY_DIR}/zip")
	add_custom_target(build-time-make-directory ALL
		COMMAND ${CMAKE_COMMAND} -E make_directory "${ZIP_DIR}"
		"${ZIP_DIR}/SKSE/Plugins/"
	)

	message("Will copy mod into ${ZIP_DIR}.")
	add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
		COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:${PROJECT_NAME}> "${ZIP_DIR}/SKSE/Plugins/"
	)

	if(${CMAKE_BUILD_TYPE} STREQUAL Debug OR ${CMAKE_BUILD_TYPE} STREQUAL RelWithDebInfo OR ${CMAKE_BUILD_TYPE} STREQUAL Release)
		add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
			COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_PDB_FILE:${PROJECT_NAME}> "${ZIP_DIR}/SKSE/Plugins/")
	endif()

	set(TARGET_ZIP "${PROJECT_NAME}_${PROJECT_VERSION}.7z")
	message("Will zip ${ZIP_DIR} to ${CMAKE_CURRENT_BINARY_DIR}/${TARGET_ZIP}.")
	ADD_CUSTOM_COMMAND(
		TARGET ${PROJECT_NAME}
		POST_BUILD
		COMMAND ${CMAKE_COMMAND} -E tar cf ${CMAKE_CURRENT_BINARY_DIR}/${TARGET_ZIP} --format=7zip -- .
		WORKING_DIRECTORY ${ZIP_DIR}
	)
endif()

PROJECT(gcbanner)

# Main binary directory. Needed for git_version.h
INCLUDE_DIRECTORIES("${CMAKE_BINARY_DIR}")

# Check for getopt_long().
INCLUDE(CheckIncludeFiles)
INCLUDE(CheckSymbolExists)
CHECK_INCLUDE_FILES(getopt.h HAVE_GETOPT_H)
CHECK_SYMBOL_EXISTS(getopt_long "getopt.h" HAVE_GETOPT_LONG)
IF(HAVE_GETOPT_H AND HAVE_GETOPT_LONG)
	# System has getopt_long().
	UNSET(getopt_SRCS)
ELSE()
	# System is missing getopt_long().
	# Use vlc's getopt_long().
	SET(getopt_SRCS
		vlc_getopt/getopt.c
		)
ENDIF()

# Write the config.h file.
CONFIGURE_FILE("${CMAKE_CURRENT_SOURCE_DIR}/config.gcbanner.h.in" "${CMAKE_CURRENT_BINARY_DIR}/config.gcbanner.h")

# Sources.
SET(gcbanner_SRCS
	gcbanner.cpp
	rijndael.c
	wibn.cpp
	)

# Win32-specific sources.
IF(WIN32)
	SET(gcbanner_OS_SRCS
		win32/gcbanner.rc
		)
ENDIF(WIN32)

#########################
# Build the executable. #
#########################

ADD_EXECUTABLE(gcbanner
	${gcbanner_SRCS}
	${gcbanner_OS_SRCS}
	${getopt_SRCS}
	)
ADD_DEPENDENCIES(gcbanner git_version)
INCLUDE(SetMSVCDebugPath)
DO_SPLIT_DEBUG(gcbanner)
SET_WINDOWS_SUBSYSTEM(gcbanner CONSOLE)
SET_WINDOWS_NO_MANIFEST(gcbanner)
SET_WINDOWS_ENTRYPOINT(gcbanner main ON)

TARGET_INCLUDE_DIRECTORIES(gcbanner
	PUBLIC	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
		$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
	)

# libgctools
TARGET_LINK_LIBRARIES(gcbanner gctools)

# Win32: image version.
INCLUDE(Win32ImageVersionLinkerFlags)
WIN32_IMAGE_VERSION_LINKER_FLAGS(${VERSION_MAJOR} ${VERSION_MINOR})

# Compress the EXE.
IF(COMPRESS_EXE)
	INCLUDE(CompressExeWithUpx)
	COMPRESS_EXE_WITH_UPX(gcbanner)
ENDIF(COMPRESS_EXE)

#################
# Installation. #
#################

INCLUDE(DirInstallPaths)

INSTALL(TARGETS gcbanner
	RUNTIME DESTINATION "${DIR_INSTALL_EXE}"
	LIBRARY DESTINATION "${DIR_INSTALL_DLL}"
	ARCHIVE DESTINATION "${DIR_INSTALL_LIB}"
	COMPONENT "program"
	)
IF(INSTALL_DEBUG)
IF(MSVC)
	INSTALL(FILES "${CMAKE_CURRENT_BINARY_DIR}/gcbanner.pdb"
		DESTINATION "${DIR_INSTALL_EXE_DEBUG}"
		COMPONENT "debug"
		)
ELSEIF(SPLIT_DEBUG)
	INSTALL(FILES "${CMAKE_CURRENT_BINARY_DIR}/gcbanner.debug"
		DESTINATION "${DIR_INSTALL_EXE_DEBUG}"
		COMPONENT "debug"
		)
ENDIF(MSVC)
ENDIF(INSTALL_DEBUG)

# Platform-specific functionality.

# Hack to remove -rdynamic from CFLAGS and CXXFLAGS
# See http://public.kitware.com/pipermail/cmake/2006-July/010404.html
IF(CMAKE_SYSTEM_NAME STREQUAL "Linux")
	SET(CMAKE_SHARED_LIBRARY_LINK_C_FLAGS)
	SET(CMAKE_SHARED_LIBRARY_LINK_CXX_FLAGS)
ENDIF()

# Don't embed rpaths in the executables.
SET(CMAKE_SKIP_RPATH ON)

# Common flag variables:
# - MCRECOVER_BASE_C_FLAGS_COMMON
# - MCRECOVER_BASE_CXX_FLAGS_COMMON
# - MCRECOVER_BASE_EXE_LINKER_FLAGS_COMMON
# - MCRECOVER_BASE_SHARED_LINKER_FLAGS_COMMON
# - MCRECOVER_BASE_C_FLAGS_DEBUG
# - MCRECOVER_BASE_CXX_FLAGS_DEBUG
# - MCRECOVER_BASE_EXE_LINKER_FLAGS_DEBUG
# - MCRECOVER_BASE_SHARED_LINKER_FLAGS_DEBUG
# - MCRECOVER_BASE_C_FLAGS_RELEASE
# - MCRECOVER_BASE_CXX_FLAGS_RELEASE
# - MCRECOVER_BASE_EXE_LINKER_FLAGS_RELEASE
# - MCRECOVER_BASE_SHARED_LINKER_FLAGS_RELEASE
#
# DEBUG and RELEASE variables do *not* include COMMON.
IF(MSVC)
	INCLUDE(cmake/platform/msvc.cmake)
ELSE(MSVC)
	INCLUDE(cmake/platform/gcc.cmake)
ENDIF(MSVC)

# Platform-specific configuration.
IF(WIN32)
	INCLUDE(cmake/platform/win32.cmake)
ENDIF(WIN32)

SET(CMAKE_C_FLAGS_DEBUG			"${CMAKE_C_FLAGS_DEBUG} ${MCRECOVER_BASE_C_FLAGS_COMMON} ${MCRECOVER_BASE_C_FLAGS_DEBUG}")
SET(CMAKE_CXX_FLAGS_DEBUG		"${CMAKE_CXX_FLAGS_DEBUG} ${MCRECOVER_BASE_CXX_FLAGS_COMMON} ${MCRECOVER_BASE_CXX_FLAGS_DEBUG}")
SET(CMAKE_EXE_LINKER_FLAGS_DEBUG	"${CMAKE_EXE_LINKER_FLAGS_DEBUG} ${MCRECOVER_BASE_EXE_LINKER_FLAGS_COMMON} ${MCRECOVER_BASE_EXE_LINKER_FLAGS_DEBUG}")
SET(CMAKE_SHARED_LINKER_FLAGS_DEBUG	"${CMAKE_SHARED_LINKER_FLAGS_DEBUG} ${MCRECOVER_BASE_SHARED_LINKER_FLAGS_COMMON} ${MCRECOVER_BASE_SHARED_LINKER_FLAGS_DEBUG}")
SET(CMAKE_C_FLAGS_RELEASE		"${CMAKE_C_FLAGS_RELEASE} ${MCRECOVER_BASE_C_FLAGS_COMMON} ${MCRECOVER_BASE_C_FLAGS_RELEASE}")
SET(CMAKE_CXX_FLAGS_RELEASE		"${CMAKE_CXX_FLAGS_RELEASE} ${MCRECOVER_BASE_CXX_FLAGS_COMMON} ${MCRECOVER_BASE_CXX_FLAGS_RELEASE}")
SET(CMAKE_EXE_LINKER_FLAGS_RELEASE	"${CMAKE_EXE_LINKER_FLAGS_RELEASE} ${MCRECOVER_BASE_EXE_LINKER_FLAGS_COMMON} ${MCRECOVER_BASE_EXE_LINKER_FLAGS_RELEASE}")
SET(CMAKE_SHARED_LINKER_FLAGS_RELEASE	"${CMAKE_SHARED_LINKER_FLAGS_RELEASE} ${MCRECOVER_BASE_SHARED_LINKER_FLAGS_COMMON} ${MCRECOVER_BASE_SHARED_LINKER_FLAGS_RELEASE}")

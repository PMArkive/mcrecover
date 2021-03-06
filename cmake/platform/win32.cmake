# Win32-specific CFLAGS/CXXFLAGS.

# Basic platform flags:
# - Enable strict type checking in the Windows headers.
# - Define WIN32_LEAN_AND_MEAN to reduce the number of Windows headers included.
# - Define NOMINMAX to disable the MIN() and MAX() macros.
SET(MCR_C_FLAGS_WIN32 "-DSTRICT -DWIN32_LEAN_AND_MEAN -DNOMINMAX")

# NOTE: This program only supports Unicode on Windows.
# No support for ANSI Windows, i.e. Win9x.
SET(MCR_C_FLAGS_WIN32 "${MCR_C_FLAGS_WIN32} -DUNICODE -D_UNICODE")

# Minimum Windows version for the SDK is Windows 2000.
SET(MCR_C_FLAGS_WIN32 "${MCR_C_FLAGS_WIN32} -DWINVER=0x0500 -D_WIN32_WINNT=0x0500 -D_WIN32_IE=0x0500")

# Enable secure template overloads for C++.
# References:
# - MinGW's _mingw_secapi.h
# - http://msdn.microsoft.com/en-us/library/ms175759%28v=VS.100%29.aspx
SET(MCR_CXX_FLAGS_WIN32 "-D_CRT_SECURE_CPP_OVERLOAD_SECURE_NAMES=1")
SET(MCR_CXX_FLAGS_WIN32 "${MCR_CXX_FLAGS_WIN32} -D_CRT_SECURE_CPP_OVERLOAD_SECURE_NAMES_MEMORY=1")
SET(MCR_CXX_FLAGS_WIN32 "${MCR_CXX_FLAGS_WIN32} -D_CRT_SECURE_CPP_OVERLOAD_STANDARD_NAMES=1")
SET(MCR_CXX_FLAGS_WIN32 "${MCR_CXX_FLAGS_WIN32} -D_CRT_SECURE_CPP_OVERLOAD_STANDARD_NAMES_COUNT=1")
SET(MCR_CXX_FLAGS_WIN32 "${MCR_CXX_FLAGS_WIN32} -D_CRT_SECURE_CPP_OVERLOAD_STANDARD_NAMES_MEMORY=1")

# Determine the processorArchitecture for the manifest files.
IF(CPU_i386)
	SET(WIN32_MANIFEST_PROCESSOR_ARCHITECTURE "x86")
ELSEIF(CPU_amd64)
	SET(WIN32_MANIFEST_PROCESSOR_ARCHITECTURE "amd64")
ELSEIF(CPU_ia64)
	SET(WIN32_MANIFEST_PROCESSOR_ARCHITECTURE "ia64")
ELSEIF(CPU_arm)
	SET(WIN32_MANIFEST_PROCESSOR_ARCHITECTURE "arm")
ELSEIF(CPU_arm64)
	SET(WIN32_MANIFEST_PROCESSOR_ARCHITECTURE "arm64")
ELSE()
	MESSAGE(FATAL_ERROR "Unsupported CPU architecture, please fix!")
ENDIF()

# Compiler-specific Win32 flags.
IF(MSVC)
	INCLUDE(cmake/platform/win32-msvc.cmake)
ELSE(MSVC)
	INCLUDE(cmake/platform/win32-gcc.cmake)
ENDIF(MSVC)

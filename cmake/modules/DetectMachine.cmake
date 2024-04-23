IF(WIN32)
	SET(LMMS_BUILD_WIN32 1)
ELSEIF(APPLE)
	SET(LMMS_BUILD_APPLE 1)
ELSEIF(${CMAKE_SYSTEM_NAME} MATCHES "OpenBSD")
	SET(LMMS_BUILD_OPENBSD 1)
ELSEIF(${CMAKE_SYSTEM_NAME} MATCHES "FreeBSD")
	SET(LMMS_BUILD_FREEBSD 1)
ELSEIF(HAIKU)
	SET(LMMS_BUILD_HAIKU 1)
ELSE()
	SET(LMMS_BUILD_LINUX 1)
ENDIF(WIN32)

# LMMS_BUILD_MSYS also set in build_winXX.sh
IF(LMMS_BUILD_WIN32 AND CMAKE_COMPILER_IS_GNUCXX AND DEFINED ENV{MSYSCON})
	SET(LMMS_BUILD_MSYS TRUE)
ENDIF()

MESSAGE("PROCESSOR: ${CMAKE_SYSTEM_PROCESSOR}")
SET(LMMS_HOST_X86 FALSE)
SET(LMMS_HOST_X86_64 FALSE)
SET(LMMS_HOST_ARM32 FALSE)
SET(LMMS_HOST_ARM64 FALSE)
SET(LMMS_HOST_RISCV32 FALSE)
SET(LMMS_HOST_RISCV64 FALSE)

IF(NOT DEFINED WIN64 AND CMAKE_SIZEOF_VOID_P EQUAL 8)
	# TODO: This seems a bit presumptous
	SET(WIN64 ON)
ENDIF()

IF(WIN32)
	if(MSVC)
		SET(MSVC_VER ${CMAKE_CXX_COMPILER_VERSION})

		# Detect target architecture
		IF(CMAKE_GENERATOR_PLATFORM)
			STRING(TOLOWER "${CMAKE_GENERATOR_PLATFORM}" MSVC_TARGET_PLATFORM)
		ELSE()
			STRING(TOLOWER "${CMAKE_VS_PLATFORM_NAME_DEFAULT}" MSVC_TARGET_PLATFORM)
		ENDIF()

		IF(MSVC_TARGET_PLATFORM MATCHES "x64")
			SET(IS_X86_64 TRUE)
			SET(LMMS_BUILD_WIN64 TRUE)
		ELSEIF(MSVC_TARGET_PLATFORM MATCHES "win32")
			SET(IS_X86 TRUE)
		ELSEIF(MSVC_TARGET_PLATFORM MATCHES "arm64")
			SET(IS_ARM64 TRUE)
		ELSEIF(MSVC_TARGET_PLATFORM MATCHES "arm")
			SET(IS_ARM32 TRUE)
		ELSEIF(CMAKE_CXX_COMPILER MATCHES "amd64/cl.exe$" OR CMAKE_CXX_COMPILER MATCHES "x64/cl.exe$")
			SET(IS_X86_64 TRUE)
			SET(LMMS_BUILD_WIN64 TRUE)
		ELSEIF(CMAKE_CXX_COMPILER MATCHES "bin/cl.exe$" OR CMAKE_CXX_COMPILER MATCHES "x86/cl.exe$")
			SET(IS_X86 TRUE)
		ELSEIF(CMAKE_CXX_COMPILER MATCHES "arm64/cl.exe$")
			SET(IS_ARM64 TRUE)
		ELSEIF(CMAKE_CXX_COMPILER MATCHES "arm/cl.exe$")
			SET(IS_ARM32 TRUE)
		ELSE()
			MESSAGE(WARNING "Unknown target architecture: ${MSVC_TARGET_PLATFORM}")
		ENDIF()

		IF(MSVC_VER VERSION_GREATER 19.30 OR MSVC_VER VERSION_EQUAL 19.30)
			SET(LMMS_MSVC_GENERATOR "Visual Studio 17 2022")
			SET(LMMS_MSVC_YEAR 2022)
		ELSEIF(MSVC_VER VERSION_GREATER 19.20 OR MSVC_VER VERSION_EQUAL 19.20)
			SET(LMMS_MSVC_GENERATOR "Visual Studio 16 2019")
			SET(LMMS_MSVC_YEAR 2019) # Qt only provides binaries for MSVC 2017, but 2019 is binary compatible
		ELSEIF(MSVC_VER VERSION_GREATER 19.10 OR MSVC_VER VERSION_EQUAL 19.10)
			SET(LMMS_MSVC_GENERATOR "Visual Studio 15 2017")
			SET(LMMS_MSVC_YEAR 2017)
		ELSEIF(MSVC_VER VERSION_GREATER 19.0 OR MSVC_VER VERSION_EQUAL 19.0)
			SET(LMMS_MSVC_GENERATOR "Visual Studio 14 2015")
			SET(LMMS_MSVC_YEAR 2015)
		ELSE()
			MESSAGE(SEND_WARNING "Can't detect MSVC version: ${MSVC_VER}")
		ENDIF()

		unset(MSVC_VER)
	else()
		# Cross-compiled
		# TODO: Handle Windows ARM64 targets
		IF(WIN64)
			SET(IS_X86_64 TRUE)
			SET(LMMS_BUILD_WIN64 TRUE)
		ELSE(WIN64)
			SET(IS_X86 TRUE)
		ENDIF(WIN64)
	endif()
ELSE()
	# Detect target architecture based on compiler target triple e.g. "x86_64-pc-linux"
	execute_process(COMMAND ${CMAKE_C_COMPILER} -dumpmachine ${CMAKE_C_FLAGS} OUTPUT_VARIABLE Machine)
	MESSAGE("Machine: ${Machine}")
	STRING(REGEX MATCH "i.86" IS_X86 "${Machine}")
	STRING(REGEX MATCH "86_64|amd64" IS_X86_64 "${Machine}")
	IF(Machine MATCHES "arm|aarch64")
		IF(Machine MATCHES "arm64|aarch64")
			SET(IS_ARM64 TRUE)
		ELSE()
			SET(IS_ARM32 TRUE)
		ENDIF()
	ELSEIF(Machine MATCHES "rv|riscv")
		IF(Machine MATCHES "rv64|riscv64")
			SET(IS_RISCV64 TRUE)
		ELSE()
			SET(IS_RISCV32 TRUE)
		ENDIF()
	ELSEIF(Machine MATCHES "ppc|powerpc")
		IF(Machine MATCHES "ppc64|powerpc64")
			SET(IS_PPC64 TRUE)
		ELSE()
			SET(IS_PPC32 TRUE)
		ENDIF()
	ENDIF()
ENDIF()

IF(IS_X86)
	MESSAGE("-- Target host is 32 bit, Intel")
	SET(LMMS_HOST_X86 TRUE)
ELSEIF(IS_X86_64)
	MESSAGE("-- Target host is 64 bit, Intel")
	SET(LMMS_HOST_X86_64 TRUE)
ELSEIF(IS_ARM32)
	MESSAGE("-- Target host is 32 bit, ARM")
	SET(LMMS_HOST_ARM32 TRUE)
ELSEIF(IS_ARM64)
	MESSAGE("-- Target host is 64 bit, ARM")
	SET(LMMS_HOST_ARM64 TRUE)
ELSEIF(IS_RISCV32)
	MESSAGE("-- Target host is 32 bit, RISC-V")
	SET(LMMS_HOST_RISCV32 TRUE)
ELSEIF(IS_RISCV64)
	MESSAGE("-- Target host is 64 bit, RISC-V")
	SET(LMMS_HOST_RISCV64 TRUE)
ELSEIF(IS_PPC32)
	MESSAGE("-- Target host is 32 bit, PPC")
	SET(LMMS_HOST_PPC32 TRUE)
ELSEIF(IS_PPC64)
	MESSAGE("-- Target host is 64 bit, PPC")
	SET(LMMS_HOST_PPC64 TRUE)
ELSE()
	MESSAGE("Can't identify target host. Assuming 32 bit platform.")
ENDIF()

IF(CMAKE_INSTALL_LIBDIR)
	SET(LIB_DIR "${CMAKE_INSTALL_LIBDIR}")
ELSE(CMAKE_INSTALL_LIBDIR)
	SET(LIB_DIR lib)
ENDIF(CMAKE_INSTALL_LIBDIR)


IF(LMMS_BUILD_WIN32)
	SET(BIN_DIR .)
	SET(PLUGIN_DIR plugins)
	SET(DATA_DIR data)
	SET(LMMS_DATA_DIR data)
ELSE(LMMS_BUILD_WIN32)
	SET(BIN_DIR bin)
	SET(PLUGIN_DIR ${LIB_DIR}/lmms)
	SET(DATA_DIR share)
	SET(LMMS_DATA_DIR ${DATA_DIR}/lmms)
ENDIF(LMMS_BUILD_WIN32)

IF(LMMS_BUILD_APPLE)
	# Detect Homebrew versus Macports environment
	EXECUTE_PROCESS(COMMAND brew --prefix RESULT_VARIABLE DETECT_HOMEBREW OUTPUT_VARIABLE HOMEBREW_PREFIX ERROR_QUIET OUTPUT_STRIP_TRAILING_WHITESPACE)
	EXECUTE_PROCESS(COMMAND which port RESULT_VARIABLE DETECT_MACPORTS OUTPUT_VARIABLE MACPORTS_PREFIX ERROR_QUIET OUTPUT_STRIP_TRAILING_WHITESPACE)
	IF(${DETECT_HOMEBREW} EQUAL 0)
       		SET(HOMEBREW 1)
        	SET(APPLE_PREFIX "${HOMEBREW_PREFIX}")
	ELSEIF(${DETECT_MACPORTS} EQUAL 0)
        	SET(MACPORTS 1)
        	GET_FILENAME_COMPONENT(MACPORTS_PREFIX ${MACPORTS_PREFIX} DIRECTORY)
		GET_FILENAME_COMPONENT(MACPORTS_PREFIX ${MACPORTS_PREFIX} DIRECTORY)
		SET(APPLE_PREFIX "${MACPORTS_PREFIX}")
        	LINK_DIRECTORIES(${LINK_DIRECTORIES} ${APPLE_PREFIX}/lib)
	ENDIF()

	# Detect OS Version
	EXECUTE_PROCESS(COMMAND sw_vers -productVersion OUTPUT_VARIABLE APPLE_OS_VER ERROR_QUIET OUTPUT_STRIP_TRAILING_WHITESPACE)
	STRING(REGEX REPLACE "\\.[0-9]*$" "" APPLE_OS_VER "${APPLE_OS_VER}")
	SET(CMAKE_MACOSX_RPATH 1)	
ENDIF()

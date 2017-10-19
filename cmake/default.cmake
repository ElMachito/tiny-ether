#default build unix host unix target

# libusys objs
set(UETH_TARGET_OS UNIX)

# libucrypto config
# TODO - add external modules
option(UETH_USE_MBEDTLS "Link with libmbedcrypto.a" ON)
option(UETH_USE_SECP256K1 "Link with libsecp256k1.a" ON)

# TODO depreciate these?
add_definitions(-DURLP_CONFIG_UNIX)
add_definitions(-DUSYS_CONFIG_UNIX)

if(UETH_USE_SECP256K1)
	ExternalProject_Add(secp256k1
		PREFIX ${CMAKE_SOURCE_DIR}/target
		GIT_REPOSITORY https://github.com/thomaskey/secp256k1
		GIT_TAG develop
    		CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=<INSTALL_DIR>
    		           -DCMAKE_POSITION_INDEPENDENT_CODE=${BUILD_SHARED_LIBS}
    		           -DCMAKE_C_COMPILER=${CMAKE_C_COMPILER}
    		           -DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}
    		           ${_only_release_configuration}
    		LOG_CONFIGURE 1
    		BUILD_COMMAND ""
    		${_overwrite_install_command}
    		LOG_INSTALL 1
	)
	# Create imported library
	ExternalProject_Get_Property(secp256k1 INSTALL_DIR)
	add_library(Secp256k1 STATIC IMPORTED)
	set(SECP256K1_LIBRARY ${INSTALL_DIR}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}secp256k1${CMAKE_STATIC_LIBRARY_SUFFIX})
	set(SECP256K1_INCLUDE_DIR ${INSTALL_DIR}/include)
	file(MAKE_DIRECTORY ${SECP256K1_INCLUDE_DIR})  # Must exist.
	set_property(TARGET Secp256k1 PROPERTY IMPORTED_CONFIGURATIONS Release)
	set_property(TARGET Secp256k1 PROPERTY IMPORTED_LOCATION_RELEASE ${SECP256K1_LIBRARY})
	set_property(TARGET Secp256k1 PROPERTY INTERFACE_INCLUDE_DIRECTORIES ${SECP256K1_INCLUDE_DIR})
	add_dependencies(Secp256k1 secp256k1)
	unset(INSTALL_DIR)
endif()

# Unix specific files
if (${UETH_USE_UNIX})
	set(USYS_DIR unix)
endif()

# Common files
set(sources ./async/async_io.c)
set(headers ./async/async_io.h)
list(APPEND sources 
	./${USYS_DIR}/usys_signals.c 
	./${USYS_DIR}/usys_io.c 
	./${USYS_DIR}/usys_log.c 
	./${USYS_DIR}/usys_time.c)
list(APPEND headers 
	./${USYS_DIR}/usys_signals.h 
	./${USYS_DIR}/usys_io.h 
	./${USYS_DIR}/usys_log.h 
	./${USYS_DIR}/usys_time.h 
	./${USYS_DIR}/usys_config.h 
	./${USYS_DIR}/usys_config_unix.h)

# libusys
add_library(usys ${sources} ${headers})
target_include_directories(usys PUBLIC ./${USYS_DIR})
target_include_directories(usys PUBLIC ./async)

#unit test for libusys
add_executable(usys_unit_test test/test.c)

# setup unit test dependencies
target_link_libraries(usys_unit_test usys)
add_dependencies(usys_unit_test usys)

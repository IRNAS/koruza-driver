cmake_minimum_required(VERSION 2.6)

project(koruza-driver C)
add_definitions(-Os -Wall -Werror --std=gnu99 -Wmissing-declarations -g3)

set(COMMON_SOURCES
message.c
frame.c
crc32.c
)

set(DAEMON_SOURCES
serial.c
koruza.c
ubus.c
main.c
)

set(LIBS
ubox
ubus
uci
)

include_directories(${CMAKE_CURRENT_SOURCE_DIR})

if(NOT ONLY_TESTS)
  add_executable(koruza-driver ${COMMON_SOURCES} ${DAEMON_SOURCES})
  target_link_libraries(koruza-driver ${LIBS})

  install(TARGETS koruza-driver
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
  )
endif(NOT ONLY_TESTS)

# Unit tests.
enable_testing()

add_executable(test_tlv ${COMMON_SOURCES} tests/test_tlv.c)
add_test(test_tlv test_tlv)

add_executable(test_frame ${COMMON_SOURCES} tests/test_frame.c)
add_test(test_frame test_frame)

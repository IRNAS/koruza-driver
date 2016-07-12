cmake_minimum_required(VERSION 2.6)

project(koruza-driver C)
add_definitions(-Os -Wall -Werror --std=gnu99 -Wmissing-declarations -g3)

set(COMMON_SOURCES
message.c
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
add_executable(koruza-driver ${COMMON_SOURCES} ${DAEMON_SOURCES})
target_link_libraries(koruza-driver ${LIBS})

install(TARGETS koruza-driver
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
)

# Unit tests.
enable_testing()

add_executable(test_tlv ${COMMON_SOURCES} tests/test_tlv.c)
target_link_libraries(test_tlv ${LIBS})
add_test(test_tlv test_tlv)

cmake_minimum_required(VERSION 2.6)

project(koruza-driver C)
add_definitions(-Os -Wall -Werror --std=gnu99 -Wmissing-declarations -g3)

if(NOT ONLY_TESTS)
  find_path(ubus_include_dir libubus.h)
  include_directories(${ubus_include_dir})

  find_path(ubox_include_dir libubox/usock.h)
  include_directories(${ubox_include_dir})

  find_path(uci_include_dir uci.h)
  include_directories(${uci_include_dir})

  find_library(ubus_library NAMES ubus)
  find_library(ubox_library NAMES ubox)
  find_library(uci_library NAMES uci)
endif(NOT ONLY_TESTS)

set(COMMON_SOURCES
message.c
frame.c
crc32.c
)

set(RPI_WS281X_SOURCES
rpi_ws281x/dma.c
rpi_ws281x/mailbox.c
rpi_ws281x/pwm.c
rpi_ws281x/rpihw.c
rpi_ws281x/ws2811.c
)

set(DAEMON_SOURCES
serial.c
gpio.c
koruza.c
ubus.c
configuration.c
network.c
upgrade.c
main.c
)

set(LIBS
m
${ubox_library}
${ubus_library}
${uci_library}
)

include_directories(${CMAKE_CURRENT_SOURCE_DIR})

if(NOT ONLY_TESTS)
  add_executable(koruza-driver ${COMMON_SOURCES} ${RPI_WS281X_SOURCES} ${DAEMON_SOURCES})
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

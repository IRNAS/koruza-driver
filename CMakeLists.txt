cmake_minimum_required(VERSION 2.6)

project(koruza-driver C)
add_definitions(-Os -Wall -Werror --std=gnu99 -Wmissing-declarations -g3)

set(SOURCES
main.c
message.c
crc32.c
)

set(LIBS
)

add_executable(koruza-driver ${SOURCES})
target_link_libraries(koruza-driver ${LIBS})

install(TARGETS koruza-driver
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
)

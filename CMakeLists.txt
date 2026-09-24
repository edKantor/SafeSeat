cmake_minimum_required(VERSION 3.20.0)

# AvEdan holds our custom boards/drivers as a Zephyr module (see
# AvEdan/zephyr/module.yml); it isn't in NCS's west manifest, so it's added
# explicitly rather than relying on `west update` to have fetched it.
list(APPEND ZEPHYR_EXTRA_MODULES ${CMAKE_CURRENT_SOURCE_DIR}/AvEdan)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(safeseat)

target_sources(app PRIVATE src/main.cpp src/beacon_task.cpp)

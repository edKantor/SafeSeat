cmake_minimum_required(VERSION 3.21)

if(NOT DEFINED AVEDAN_MCU_FAMILY)
    set(AVEDAN_MCU_FAMILY nrf CACHE STRING "MCU family: stm32 or nrf")
endif()
if(NOT DEFINED AVEDAN_BOARD)
    set(AVEDAN_BOARD nrf52_dk CACHE STRING "Board under AvEdan/bsp/boards")
endif()

project(SafeSeat CXX C ASM)

# Same bare-metal rationale as AvEdan/CMakeLists.txt; set here too since this
# app/ target lives outside AvEdan's own directory scope.
add_compile_options(
    $<$<COMPILE_LANGUAGE:CXX>:-fno-exceptions>
    $<$<COMPILE_LANGUAGE:CXX>:-fno-rtti>)

add_subdirectory(AvEdan)

add_executable(safeseat app/main.cpp)
target_link_libraries(safeseat PRIVATE avedan_bsp avedan_drivers)
target_compile_definitions(safeseat PRIVATE AVEDAN_BOARD_NS=bsp::${AVEDAN_BOARD})

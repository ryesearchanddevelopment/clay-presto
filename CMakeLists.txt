cmake_minimum_required(VERSION 3.12)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

set(NAME clay-presto)

# == DO NOT EDIT THE FOLLOWING LINES for the Raspberry Pi Pico VS Code Extension to work ==
if(WIN32)
    set(USERHOME $ENV{USERPROFILE})
else()
    set(USERHOME $ENV{HOME})
endif()
set(sdkVersion 2.1.0)
set(toolchainVersion 13_3_Rel1)
set(picotoolVersion 2.1.0)
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
if (EXISTS ${picoVscode})
    include(${picoVscode})
endif()
# ====================================================================================
set(PICO_PLATFORM rp2350-arm-s)
set(PICO_BOARD pimoroni_pico_plus2_w_rp2350 CACHE STRING "Board type")

# the following enables the system malloc/free to be wrapped
set(SKIP_PICO_MALLOC 1)

# the following enables wrapping in sparkfun_pico builds
set(SFE_PICO_ALLOC_WRAP 1)

# the following enables the system malloc/free to be wrapped during compilation
add_definitions(-DSFE_PICO_ALLOC_WRAP)

set(PIMORONI_PICO_FETCH_FROM_GIT 1)
set(PIMORONI_PICO_FETCH_FROM_GIT_PATH ${CMAKE_CURRENT_LIST_DIR}/../pimoroni-pico)
include(pimoroni_pico_import.cmake)
include(pico_sdk_import.cmake)

# Gooey boilerplate
project(clay-presto C CXX ASM)

# Initialize the SDK
pico_sdk_init()

if (NOT PIMORONI_PRESTO_PATH)
    set(PIMORONI_PRESTO_PATH ../presto/)
endif()
if(NOT IS_ABSOLUTE ${PIMORONI_PRESTO_PATH})
    get_filename_component(
        PIMORONI_PRESTO_PATH
        "${CMAKE_CURRENT_BINARY_DIR}/${PIMORONI_PRESTO_PATH}"
        ABSOLUTE)
endif()
message("PIMORONI_PRESTO_PATH is ${PIMORONI_PRESTO_PATH}")
set(PIMORONI_PRESTO_PATH ${PIMORONI_PRESTO_PATH} CACHE PATH "Path to the Presto libraries" FORCE)
include_directories(${PIMORONI_PRESTO_PATH})
list(APPEND CMAKE_MODULE_PATH ${PIMORONI_PRESTO_PATH})
include(drivers/st7701/st7701_presto)

include_directories(${CMAKE_CURRENT_LIST_DIR}/uzlib/src)

# Add your source files
add_executable(clay-presto
    src/main.cpp # <-- Add source files here!
    src/fileio.cpp
)

add_definitions(-DSFE_RP2350_XIP_CSI_PIN=47)
add_subdirectory(sparkfun_pico)

# Include required libraries
# This assumes `pimoroni-pico` is stored alongside your project
include(common/pimoroni_i2c)
include(common/pimoroni_bus)
include(drivers/fatfs/fatfs)
include(drivers/sdcard/sdcard)
include(libraries/pico_graphics/pico_graphics)
include(libraries/pico_vector/pico_vector)

# Don't forget to link the libraries you need!
target_link_libraries(clay-presto
    st7701_presto
    pico_stdlib
    pico_multicore
    pimoroni_i2c
    sdcard
    fatfs
    hardware_interp
    pico_graphics
    pico_vector
    sparkfun_pico
)

# Configure the SD Card library for Presto
target_compile_definitions(clay-presto PRIVATE
  SDCARD_SPI_BUS=spi0
  SDCARD_PIN_SPI0_CS=39
  SDCARD_PIN_SPI0_SCK=34
  SDCARD_PIN_SPI0_MOSI=35
  SDCARD_PIN_SPI0_MISO=36
  PICO_CLOCK_AJDUST_PERI_CLOCK_WITH_SYS_CLOCK=1
)

# create map/bin/hex file etc.
pico_add_extra_outputs(clay-presto)

# Enable USB UART output only
pico_enable_stdio_uart(clay-presto 0)
pico_enable_stdio_usb(clay-presto 1)

# Set up files for the release packages
install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/clay-presto.uf2
    ${CMAKE_CURRENT_LIST_DIR}/README.md
    DESTINATION .
)

set(CPACK_PACKAGE_FILE_NAME clay-presto)
set(CPACK_INCLUDE_TOPLEVEL_DIRECTORY OFF)
set(CPACK_GENERATOR "ZIP" "TGZ")
include(CPack)
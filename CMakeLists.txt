#
# Title:	        Pico-mposite Makefile
# Description:		Makefile 
# Author:	        Dean Belfield
# Created:	        31/01/2021
# Last Updated:	        19/02/2022
#
# Modinfo:
# 01/02/2022:		Added this header comment, fixed typo in executable filename, added extra target sources
# 19/02/2022:           Added terminal.c

#
# See the official documentation https://www.raspberrypi.com/documentation/microcontrollers/c_sdk.html
# for guidance on setting up the Pico C/C++ SDK.
#

add_executable(pico-mposite)

pico_generate_pio_header(pico-mposite ${CMAKE_CURRENT_LIST_DIR}/cvideo_sync.pio)
pico_generate_pio_header(pico-mposite ${CMAKE_CURRENT_LIST_DIR}/cvideo_data.pio)

target_sources(
        pico-mposite PRIVATE
        main.c
        cvideo.c
        graphics.c
        charset.c
        bitmap.c
        terminal.c
)

target_link_libraries(
        pico-mposite PRIVATE
        pico_stdlib
        pico_mem_ops
        hardware_pio
        hardware_dma
        hardware_irq
)

pico_add_extra_outputs(pico-mposite)

# add url via pico_set_program_url
#
example_auto_set_url(pico-mposite)

# generate .hex file and .pio.h file for the RP2040 datasheet (to make sure
# the datasheet always shows the output of the latest pioasm version)
#
add_custom_target(
        pio_cvideo_sync_datasheet DEPENDS
	Pioasm
	${CMAKE_CURRENT_LIST_DIR}/generated/cvideo_sync.hex
	${CMAKE_CURRENT_LIST_DIR}/generated/cvideo_sync.pio.h
)
add_custom_command(
        OUTPUT ${CMAKE_CURRENT_LIST_DIR}/generated/cvideo_sync.hex
        DEPENDS ${CMAKE_CURRENT_LIST_DIR}/cvideo_sync.pio
        COMMAND Pioasm -o hex ${CMAKE_CURRENT_LIST_DIR}/cvideo_sync.pio ${CMAKE_CURRENT_LIST_DIR}/generated/cvideo_sync.hex
)
add_custom_command(
        OUTPUT ${CMAKE_CURRENT_LIST_DIR}/generated/cvideo_sync.pio.h
        DEPENDS ${CMAKE_CURRENT_LIST_DIR}/cvideo_sync.pio
        COMMAND Pioasm ${CMAKE_CURRENT_LIST_DIR}/cvideo_sync.pio ${CMAKE_CURRENT_LIST_DIR}/generated/cvideo_sync.pio.h
)

add_custom_target(
        pio_cvideo_data_datasheet DEPENDS
	Pioasm
	${CMAKE_CURRENT_LIST_DIR}/generated/cvideo_data.hex
	${CMAKE_CURRENT_LIST_DIR}/generated/cvideo_data.pio.h
)
add_custom_command(
        OUTPUT ${CMAKE_CURRENT_LIST_DIR}/generated/cvideo_data.hex
        DEPENDS ${CMAKE_CURRENT_LIST_DIR}/cvideo_data.pio
        COMMAND Pioasm -o hex ${CMAKE_CURRENT_LIST_DIR}/cvideo_data.pio ${CMAKE_CURRENT_LIST_DIR}/generated/cvideo_data.hex
)
add_custom_command(
        OUTPUT ${CMAKE_CURRENT_LIST_DIR}/generated/cvideo_data.pio.h
        DEPENDS ${CMAKE_CURRENT_LIST_DIR}/cvideo_data.pio
        COMMAND Pioasm ${CMAKE_CURRENT_LIST_DIR}/cvideo_data.pio ${CMAKE_CURRENT_LIST_DIR}/generated/cvideo_data.pio.h
)


add_dependencies(pico-mposite pio_cvideo_sync_datasheet pio_cvideo_data_datasheet)
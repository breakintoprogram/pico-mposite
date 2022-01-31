add_executable(pi-composite)

pico_generate_pio_header(pi-composite ${CMAKE_CURRENT_LIST_DIR}/cvideo_sync.pio)
pico_generate_pio_header(pi-composite ${CMAKE_CURRENT_LIST_DIR}/cvideo_data.pio)

target_sources(
        pi-composite PRIVATE
        cvideo.c
)

target_link_libraries(
        pi-composite PRIVATE
        pico_stdlib
        pico_mem_ops
        hardware_pio
        hardware_dma
        hardware_irq
)

pico_add_extra_outputs(pi-composite)

# add url via pico_set_program_url
#
example_auto_set_url(pi-composite)

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


add_dependencies(pi-composite pio_cvideo_sync_datasheet pio_cvideo_data_datasheet)
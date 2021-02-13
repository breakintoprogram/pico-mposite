add_executable(pio_cvideo)

pico_generate_pio_header(pio_cvideo ${CMAKE_CURRENT_LIST_DIR}/cvideo.pio)

target_sources(pio_cvideo PRIVATE cvideo.c)

target_link_libraries(pio_cvideo PRIVATE pico_stdlib pico_mem_ops hardware_pio hardware_dma hardware_irq)
pico_add_extra_outputs(pio_cvideo)

# add url via pico_set_program_url
example_auto_set_url(pio_cvideo)

# generate .hex file and .pio.h file for the RP2040 datasheet (to make sure
# the datasheet always shows the output of the latest pioasm version)
add_custom_target(pio_cvideo_datasheet DEPENDS
		Pioasm
		${CMAKE_CURRENT_LIST_DIR}/generated/cvideo.hex
		${CMAKE_CURRENT_LIST_DIR}/generated/cvideo.pio.h
		)
add_custom_command(OUTPUT ${CMAKE_CURRENT_LIST_DIR}/generated/cvideo.hex
        DEPENDS ${CMAKE_CURRENT_LIST_DIR}/cvideo.pio
        COMMAND Pioasm -o hex ${CMAKE_CURRENT_LIST_DIR}/cvideo.pio ${CMAKE_CURRENT_LIST_DIR}/generated/cvideo.hex
        )
add_custom_command(OUTPUT ${CMAKE_CURRENT_LIST_DIR}/generated/cvideo.pio.h
        DEPENDS ${CMAKE_CURRENT_LIST_DIR}/cvideo.pio
        COMMAND Pioasm ${CMAKE_CURRENT_LIST_DIR}/cvideo.pio ${CMAKE_CURRENT_LIST_DIR}/generated/cvideo.pio.h
        )

add_dependencies(pio_cvideo pio_cvideo_datasheet)

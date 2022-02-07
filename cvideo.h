//
// Title:	        Pico-mposite Video Output
// Author:	        Dean Belfield
// Created:	        26/01/2021
// Last Updated:	05/02/2022
//
// Modinfo:
// 31/01/2022:      Tweaks to reflect code changes
// 02/02/2022:      Added initialise_cvideo
// 04/02/2022:      Added set_border
// 05/02/2022:      Added support for colour composite board

#pragma once

#define opt_colour  0           // Set to 1 for colour version

#define piofreq_0 5.25f         // Clock frequence of state machine for PIO handling sync
#define piofreq_1  7.0f         // Clock frequency of state machine for PIO handling pixel data

#define sm_sync 0               // State machine number in the PIO for the sync data
#define sm_data 1               // State machine number in the PIO for the pixel data   

#if opt_colour == 0
    #define colour_base 0x10    // Start colour; for monochrome version this relates to black level voltage
    #define colour_max  0x0f    // Last available colour
    #define HSLO        0x0001
    #define HSHI        0x000d
    #define VSLO        HSLO
    #define VSHI        HSHI
    #define BORD        0x8000
    #define gpio_base   0
    #define gpio_count  5
#else
    #define colour_base 0x00
    #define colour_max  0xFF
    #define HSLO        0x0000
    #define HSHI        0x0200
    #define VSLO        0x0000
    #define VSHI        0x0100
    #define BORD        0x8000
    #define gpio_base   0
    #define gpio_count  10
#endif

int initialise_cvideo(void);

void cvideo_configure_pio_dma(PIO pio, uint sm, uint dma_channel, uint transfer_size, size_t buffer_size,  irq_handler_t handler);

void cvideo_pio_handler(void);
void cvideo_dma_handler(void);

void wait_vblank(void);
void set_border(unsigned char colour);
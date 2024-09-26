//
// Title:	        Pico-mposite Video Output
// Author:	        Dean Belfield
// Created:	        26/01/2021
// Last Updated:	26/09/2024
//
// Modinfo:
// 31/01/2022:      Tweaks to reflect code changes
// 02/02/2022:      Added initialise_cvideo
// 04/02/2022:      Added set_border
// 05/02/2022:      Added support for colour composite board
// 20/02/2022:      Bitmap is now dynamically allocated
// 01/03/2022:      Tweaked sync parameters for colour version
// 26/09/2024:		Externed variables

#pragma once

#include "config.h"

#define piofreq_0 5.25f         // Clock frequence of state machine for PIO handling sync
#define piofreq_1_256 7.00f     // Clock frequency of state machine for PIO handling pixel data at various resolutions
#define piofreq_1_320 5.60f
#define piofreq_1_640 2.80f

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
    #define HSLO        0x4200
    #define HSHI        0x4000
    #define VSLO        0x4100
    #define VSHI        0x4000
    #define BORD        0x8000
    #define gpio_base   0
    #define gpio_count  10
#endif

extern unsigned char * bitmap;

extern int width;
extern int height;

int initialise_cvideo(void);
int set_mode(int mode);

void cvideo_configure_pio_dma(PIO pio, uint sm, uint dma_channel, uint transfer_size, size_t buffer_size,  irq_handler_t handler);

void cvideo_pio_handler(void);
void cvideo_dma_handler(void);

void wait_vblank(void);
void set_border(unsigned char colour);

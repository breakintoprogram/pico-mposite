//
// Title:	        Pico-mposite Video Output
// Author:	        Dean Belfield
// Created:	        26/01/2021
// Last Updated:	02/02/2022
//
// Modinfo:
// 31/01/2022:      Tweaks to reflect code changes
// 02/02/2022:      Added initialise_cvideo
// 

#pragma once

#define piofreq_0 5.25f         // Clock frequence of state machine for PIO handling sync
#define piofreq_1  7.0f         // Clock frequency of state machine for PIO handling pixel data

#define sm_sync 0               // State machine number in the PIO for the sync data
#define sm_data 1               // State machine number in the PIO for the pixel data   

int initialise_cvideo(void);

void cvideo_configure_pio_dma(PIO pio, uint sm, uint dma_channel, size_t buffer_size_words, irq_handler_t handler);

void cvideo_pio_handler(void);
void cvideo_dma_handler(void);

void wait_vblank(void);
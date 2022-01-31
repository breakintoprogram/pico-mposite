//
// Title:	        Pico-mposite Video Output
// Author:	        Dean Belfield
// Created:	        26/01/2021
// Last Updated:	31/01/2022
//
// Modinfo:
// 31/01/2022:      Tweaks to reflect code changes
// 

#pragma once

void cvideo_configure_pio_dma(PIO pio, uint sm, uint dma_channel, size_t buffer_size_words, irq_handler_t handler);
void cvideo_pio_handler(void);
void cvideo_dma_handler(void);
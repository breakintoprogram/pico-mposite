#pragma once

void cvideo_configure_pio_dma(PIO pio, uint sm, uint dma_channel, size_t buffer_size_words);
void cvideo_dma_handler(void);

void write_vsync_s(unsigned char *p, int length);
void write_vsync_l(unsigned char *p, int length);

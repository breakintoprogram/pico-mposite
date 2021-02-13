//
// Title:	        Pico-mposite Video Output
// Author:	        Dean Belfield
// Created:	        26/01/2021
// Last Updated:	26/01/2021
//
// Modinfo:
// 

#include "memory.h"

#include "pico/stdlib.h"

#include "hardware/pio.h"
#include "hardware/dma.h"
#include "hardware/irq.h"

#include "cvideo.h"
#include "cvideo.pio.h"     // The assembled PIO code

#define state_machine 0     // The PIO state machine to use
#define width 256           // Bitmap width in pixels
#define height 192          // Bitmap height in pixels
#define hsync_bp1 24        // Length of pulse at 0.0v
#define hsync_bp2 48        // Length of pulse at 0.3v
#define hdots 382           // Data for hsync including back porch
#define piofreq 7.0f        // Clock frequence of state machine
#define border_colour 11    // The border colour

#define pixel_start hsync_bp1 + hsync_bp2 + 18  // Where the pixel data starts in pixel_buffer

uint dma_channel;           // DMA channel for transferring hsync data to PIO
uint vline;                 // Current video line being processed
uint bline;                 // Line in the bitmap to fetch

#include "bitmap.h"         // The demo bitmap

unsigned char vsync_ll[hdots+1];					// buffer for a vsync line with a long/long pulse
unsigned char vsync_ls[hdots+1];					// buffer for a vsync line with a long/short pulse
unsigned char vsync_ss[hdots+1];					// Buffer for a vsync line with a short/short pulse
unsigned char border[hdots+1];						// Buffer for a vsync line for the top and bottom borders
unsigned char pixel_buffer[height][hdots+1];		// Buffer for all the vsync lines for the image

int main() {
    PIO pio = pio0;
    uint offset = pio_add_program(pio, &cvideo_program);	// Load up the PIO program

    dma_channel = dma_claim_unused_channel(true);			// Claim a DMA channel for the hsync transfer
    vline = 1;												// Initialise the video scan line counter to 1
    bline = 0;												// And the index into the bitmap pixel buffer to 0

    write_vsync_l(&vsync_ll[0],        hdots>>1);			// Pre-build a long/long vscan line...
    write_vsync_l(&vsync_ll[hdots>>1], hdots>>1);
    write_vsync_l(&vsync_ls[0],        hdots>>1);			// A long/short vscan line...
    write_vsync_s(&vsync_ls[hdots>>1], hdots>>1);
    write_vsync_s(&vsync_ss[0],        hdots>>1);			// A short/short vscan line
    write_vsync_s(&vsync_ss[hdots>>1], hdots>>1);

    memset(&border[0], border_colour, hdots);				// And fill the border with the border colour

	// This bit pre-builds the bitmap by adding the horizontal border stripes and the hsync pulse
	//
    for(int i = 0; i < height; i++) {								// Loop through the pixel buffer
        memset(&pixel_buffer[i][0], border_colour, hdots);			// First fill the buffer with the border colour
        memset(&pixel_buffer[i][0], 1, hsync_bp1);					// Add the hsync pulse
        memset(&pixel_buffer[i][hsync_bp1], 9, hsync_bp2);
        memset(&pixel_buffer[i][pixel_start], 31, width);
        memcpy(&pixel_buffer[i][pixel_start], &bitmap[i], width);	// Copy the bitmap into the middle
    }

	// Initialise the PIO
	//
    pio_sm_set_enabled(pio, state_machine, false);
    pio_sm_clear_fifos(pio, state_machine);	
    cvideo_initialise_pio(pio, state_machine, offset, 0, 5, piofreq);
    cvideo_configure_pio_dma(pio, state_machine, dma_channel, pixel_buffer[0], hdots+1);
    pio_sm_set_enabled(pio, state_machine, true);

	// And the DMA
	//
    cvideo_dma_handler();
    while (true) {
        tight_loop_contents();
    }
}

// Write out a short vsync pulse
//
void write_vsync_s(unsigned char *p, int length) {
    int pulse_width = length / 16;
    for(int i = 0; i < length; i++) {
        p[i] = i <= pulse_width ? 1 : 13;
    }
}

// Write out a long vsync half-pulse
//
void write_vsync_l(unsigned char *p, int length) {
    int pulse_width = length - (length / 16) - 1;
    for(int i = 0; i < length; i++) {
        p[i] = i >= pulse_width ? 13 : 1;
    }
}

// The hblank interrupt handler
// 
void cvideo_dma_handler(void) {
    void * p_data;
    dma_hw->ints0 = 1u << dma_channel;		// Clear the interrupt request
    switch(vline) {							// What scanline are we on?
        case 1 ... 2:
            p_data = vsync_ll;   			// Do the vertical sync signals first
            break;
        case 3:
            p_data = vsync_ls;
            break;
        case 4 ... 5:
            p_data = vsync_ss;
            break;
        case 6 ... 68:
            p_data = border;				// Our fake top and bottom borders on these scanline
            break;
        case 260 ... 309:
            p_data = border;
            break;
        case 310 ... 312:
            p_data = vsync_ss;				// More vertical sync at the bottom
            break;
        default:
            p_data = pixel_buffer[bline++];	// Otherwise use data in the pixel buffer
            break;
    }

	// Reset the DMA buffer and retrigger the DMA
	//
    dma_channel_set_read_addr(dma_channel, p_data, true);	
    vline++;
    if(vline >= 313) {	// If we've gone past the bottom scanline then
        vline = 1;		// Reset the scanline counter
        bline = 0;		// And the pixel buffer row index counter
    }
}

// Configure the PIO DMA
// Parameters:
// - pio: The PIO to attach this to
// - sm: The state machine number
// - dma_channel: The DMA channel
// - buffer: No longer used
// - buffer_size_words: Number of bytes to transfer
//
void cvideo_configure_pio_dma(PIO pio, uint sm, uint dma_channel, unsigned char *buffer, size_t buffer_size_words) {
    pio_sm_clear_fifos(pio, sm);
    dma_channel_config c = dma_channel_get_default_config(dma_channel);
    channel_config_set_transfer_data_size(&c, DMA_SIZE_8);
    channel_config_set_read_increment(&c, true);
    channel_config_set_dreq(&c, pio_get_dreq(pio, sm, true));
    dma_channel_configure(dma_channel, &c,
        &pio->txf[sm],              // Destination pointer
        NULL,                       // Source pointer
        buffer_size_words,          // Number of transfers
        true                        // Start flag (true = start immediately)
    );
    dma_channel_set_irq0_enabled(dma_channel, true);
    irq_set_exclusive_handler(DMA_IRQ_0, cvideo_dma_handler);
    irq_set_enabled(DMA_IRQ_0, true);
}

//
// Title:	        Pico-mposite Video Output
// Author:	        Dean Belfield
// Created:	        26/01/2021
// Last Updated:	15/02/2021
//
// Modinfo:
// 15/02/2021:      Border buffers now have horizontal sync pulse set correctly
//                  Decreased RAM usage by updating the image buffer scanline on the fly during the horizontal interrupt
//					Fixed logic error in cvideo_dma_handler; initial memcpy done twice
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
unsigned char pixel_buffer[2][hdots+1];	        	// Double-buffer for the pixel data scanlines

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

    // This bit pre-builds the border scanline
    //
    memset(&border[0], border_colour, hdots);				// Fill the border with the border colour
    memset(&border[0], 1, hsync_bp1);				        // Add the hsync pulse
    memset(&border[hsync_bp1], 9, hsync_bp2);

	// This bit pre-builds the pixel buffer scanlines by adding the hsync pulse and left and right horizontal borders 
	//
    for(int i = 0; i < 2; i++) {							// Loop through the pixel buffers
        memset(&pixel_buffer[i][0], border_colour, hdots);	// First fill the buffer with the border colour
        memset(&pixel_buffer[i][0], 1, hsync_bp1);			// Add the hsync pulse
        memset(&pixel_buffer[i][hsync_bp1], 9, hsync_bp2);
        memset(&pixel_buffer[i][pixel_start], 31, width);
    }

	// Initialise the PIO
	//
    pio_sm_set_enabled(pio, state_machine, false);                      // Disable the PIO state machine
    pio_sm_clear_fifos(pio, state_machine);	                            // Clear the PIO FIFO buffers
    cvideo_initialise_pio(pio, state_machine, offset, 0, 5, piofreq);   // Initialise the PIO (function in cvideo.pio)
    cvideo_configure_pio_dma(pio, state_machine, dma_channel, hdots+1); // Hook up the DMA channel to the state machine
    pio_sm_set_enabled(pio, state_machine, true);                       // Enable the PIO state machine

	// And kick everything off
	//
    cvideo_dma_handler();       // Call the DMA handler as a one-off to initialise it
    while (true) {              // And then just loop doing nothing
        tight_loop_contents();
    }
}

// Write out a short vsync pulse
// Parameters:
// - p: Pointer to the buffer to store this sync data
// - length: The buffer size
//
void write_vsync_s(unsigned char *p, int length) {
    int pulse_width = length / 16;
    for(int i = 0; i < length; i++) {
        p[i] = i <= pulse_width ? 1 : 13;
    }
}

// Write out a long vsync half-pulse
// Parameters:
// - p: Pointer to the buffer to store this sync data
// - length: The buffer size
//
void write_vsync_l(unsigned char *p, int length) {
    int pulse_width = length - (length / 16) - 1;
    for(int i = 0; i < length; i++) {
        p[i] = i >= pulse_width ? 13 : 1;
    }
}

// The hblank interrupt handler
// This is triggered by the instruction irq set 0 in the PIO code (cvideo.pio)
// 
void cvideo_dma_handler(void) {

    // Switch condition on the vertical scanline number (vline)
    // Each statement does a dma_channel_set_read_addr to point the PIO to the next data to output
    //
    switch(vline) {

        // First deal with the vertical sync scanlines
        // Also on scanline 3, preload the first pixel buffer scanline
        //
        case 1 ... 2: 
            dma_channel_set_read_addr(dma_channel, vsync_ll, true);
            break;
        case 3:
            dma_channel_set_read_addr(dma_channel, vsync_ls, true);
            memcpy(&pixel_buffer[bline & 1][pixel_start], &bitmap[bline], width);
            break;
        case 4 ... 5:
        case 310 ... 312:
            dma_channel_set_read_addr(dma_channel, vsync_ss, true);
            break;

        // Then the border scanlines
        //
        case 6 ... 68:
        case 260 ... 309:
            dma_channel_set_read_addr(dma_channel, border, true);
            break;

        // Now point the dma at the first buffer for the pixel data,
        // and preload the data for the next scanline
        // 
        default:
            dma_channel_set_read_addr(dma_channel, pixel_buffer[bline++ & 1], true);    // Set the DMA to read from one of the pixel_buffers
            memcpy(&pixel_buffer[bline & 1][pixel_start], &bitmap[bline], width);       // And memcpy the next scanline into the other pixel buffer
            break;
    }

    // Increment and wrap the counters
    //
    if(vline++ >= 312) {    // If we've gone past the bottom scanline then
        vline = 1;		    // Reset the scanline counter
        bline = 0;		    // And the pixel buffer row index counter
    }

    // Finally, clear the interrupt request ready for the next horizontal sync interrupt
    //
    dma_hw->ints0 = 1u << dma_channel;		
}

// Configure the PIO DMA
// Parameters:
// - pio: The PIO to attach this to
// - sm: The state machine number
// - dma_channel: The DMA channel
// - buffer_size_words: Number of bytes to transfer
//
void cvideo_configure_pio_dma(PIO pio, uint sm, uint dma_channel, size_t buffer_size_words) {
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

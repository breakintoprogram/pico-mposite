//
// Title:	        Pico-mposite Video Output
// Description:		The composite video stuff
// Author:	        Dean Belfield
// Created:	        26/01/2021
// Last Updated:	04/02/2022
//
// Modinfo:
// 15/02/2021:      Border buffers now have horizontal sync pulse set correctly
//                  Decreased RAM usage by updating the image buffer scanline on the fly during the horizontal interrupt
//					Fixed logic error in cvideo_dma_handler; initial memcpy done twice
// 31/01/2022:      Refactored to use less memory
//					Split the video generation into two state machines; sync and data
// 01/02/2022:      Added a handful of graphics primitives
// 02/02/2022:      Split main loop out into main.c
// 04/02/2022:      Added set_border
// 

#include <stdlib.h>

#include "memory.h"
#include "pico/stdlib.h"

#include "hardware/pio.h"
#include "hardware/dma.h"
#include "hardware/irq.h"   

#include "charset.h"            // The character set

#include "cvideo.h"
#include "graphics.h"
#include "cvideo_sync.pio.h"    // The assembled PIO code
#include "cvideo_data.pio.h"    

PIO pio_0;                      // The PIO that this uses

uint dma_channel_0;             // DMA channel for transferring sync data to PIO
uint dma_channel_1;             // DMA channel for transferring pixel data data to PIO
uint vline;                     // Current PAL(ish) video line being processed
uint bline;                     // Line in the bitmap to fetch

uint vblank_count;              // Vblank counter

/*
 * The sync tables consist of 32 entries, each one corresponding to a 2us slice of the 64us
 * horizontal sync. The value 0x00 is reserved as a control byte for the horizontal sync;
 * cvideo_sync will not write to the GPIO for that block of 0x00's.
 */

// Horizontal sync with gap for pixel data
//
unsigned char hsync[32] = {
    0x01, 0x0d, 0x0d, 0x10, 0x10, 0x10, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x10, 0x10, 0x10,
};

// Horizontal sync for top and bottom borders
//
unsigned char border[32] = {
    0x01, 0x0d, 0x0d, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 
    0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 
};

// Vertical sync (long/long)
//
unsigned char vsync_ll[32] = {
    0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x0d,
    0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x0d,
};

// Vertical sync (short/short)
//
unsigned char vsync_ss[32] = {
    0x01, 0x0d, 0x0d, 0x0d, 0x0d, 0x0d, 0x0d, 0x0d, 0x0d, 0x0d, 0x0d, 0x0d, 0x0d, 0x0d, 0x0d, 0x0d,
    0x01, 0x0d, 0x0d, 0x0d, 0x0d, 0x0d, 0x0d, 0x0d, 0x0d, 0x0d, 0x0d, 0x0d, 0x0d, 0x0d, 0x0d, 0x0d,
};

// Vertical sync (long/short)
//
unsigned char vsync_ls[32] = {
    0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x0d,
    0x01, 0x0d, 0x0d, 0x0d, 0x0d, 0x0d, 0x0d, 0x0d, 0x0d, 0x0d, 0x0d, 0x0d, 0x0d, 0x0d, 0x0d, 0x0d,
};

/*
 * The main routine sets up the whole shebang
 */
int initialise_cvideo() { 
    pio_0 = pio0;	// Assign the PIO

    // Load up the PIO programs
    //
    uint offset_0 = pio_add_program(pio_0, &cvideo_sync_program);
    uint offset_1 = pio_add_program(pio_0, &cvideo_data_program);

    dma_channel_0 = dma_claim_unused_channel(true);	// Claim a DMA channel for the sync
    dma_channel_1 = dma_claim_unused_channel(true);	// And one for the pixel data

    vline = 1;          // Initialise the video scan line counter to 1
    bline = 0;	        // And the index into the bitmap pixel buffer to 0
    vblank_count = 0;   // And the vblank counter

	// Initialise the first PIO (video sync)
	//
    pio_sm_set_enabled(pio_0, sm_sync, false);	// Disable the PIO state machine
    pio_sm_clear_fifos(pio_0, sm_sync);			// Clear the PIO FIFO buffers
    cvideo_sync_initialise_pio(					// Initialise the PIO (function in cvideo.pio)
		pio_0,									// The PIO to attach this state machine to
		sm_sync,								// The state machine number
		offset_0,								// And offset
		0,										// Start pin in the GPIO
		5,										// Number of pins
		piofreq_0								// State machine clock frequency
	);	
    cvideo_configure_pio_dma(					// Configure the DMA
        pio_0,									// The PIO to attach this DMA to
        sm_sync,								// The state machine number
        dma_channel_0,							// The DMA channel
        32,										// Number of bytes to transfer
        cvideo_dma_handler						// The DMA handler
    );
    pio_sm_set_enabled(pio_0, sm_sync, true);	// Enable the PIO state machine

	// Initialise the second PIO (pixel data)
	//
    pio_sm_set_enabled(pio_0, sm_data, false);	// As above...
    pio_sm_clear_fifos(pio_0, sm_data);
    cvideo_data_initialise_pio(					// .. but a different state machine
		pio_0,
		sm_data,
		offset_1,
		0,
		5,
		piofreq_1
	);
   
    // Initialise the DMA
    //
    cvideo_configure_pio_dma(
        pio_0,	
        sm_data,
        dma_channel_1,							// On DMA channel 1
        256,									// This time there is 256 bytes of data (pixels)
        NULL									// But there is no DMA interrupt for the pixl data
    ); 
    pio_sm_set_enabled(pio_0, sm_data, true);	// Enable the PIO state machine

    irq_set_exclusive_handler(					// Set up the PIO IRQ handler
		PIO0_IRQ_0,								// The IRQ #
		cvideo_pio_handler						// And handler routine
	); 
    irq_set_enabled(PIO0_IRQ_0, true);			// Enable it
    pio0_hw->inte0 = PIO_IRQ0_INTE_SM0_BITS;	// Just for IRQ 0 (triggered by irq set 0 in PIO)

    set_border(0x10);                           // Set the border colour
    cls(0x10);                                  // Clear the screen

	// And kick everything off
	//
    cvideo_pio_handler();                       // Call the handlers as a one-off to initialise both DMAs
    cvideo_dma_handler();       

    return 0;
}

// Set the border colour
//
void set_border(unsigned char colour) {
    if(colour < 0x10 || colour > 0x1f) {
        return;
    }
    for(int i = 3; i <32; i++) {
        if(hsync[i] >= 0x10) {
            hsync[i] = colour;
        }
        border[i] = colour;
    }
}

// Wait for vblank
//
void wait_vblank(void) {
    uint c = vblank_count;
    while(c == vblank_count) {
        sleep_us(64);
    }
}

// The PIO interrupt handler
// This sets up the DMA for cvideo_data with pixel data and is triggered by the irq set 0
// instruction at the end of the PIO
//
void cvideo_pio_handler(void) {
    dma_channel_set_read_addr(dma_channel_1, bitmap[bline++], true);	// Line up the next block of pixels
    hw_set_bits(&pio0->irq, 1u);										// Reset the IRQ
}

// The DMA interrupt handler
// This feeds the state machine cvideo_sync with data for the PAL(ish) video signal
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
            dma_channel_set_read_addr(dma_channel_0, vsync_ll, true);
            break;
        case 3:
            dma_channel_set_read_addr(dma_channel_0, vsync_ls, true);
            break;
        case 4 ... 5:
        case 310 ... 312:
            dma_channel_set_read_addr(dma_channel_0, vsync_ss, true);
            break;

        // Then the border scanlines
        //
        case 6 ... 68:
        case 261 ... 309:
            dma_channel_set_read_addr(dma_channel_0, border, true);
            break;

        // Now point the dma at the first buffer for the pixel data,
        // and preload the data for the next scanline
        // 
        default:
            dma_channel_set_read_addr(dma_channel_0, hsync, true);
            break;
    }

    // Increment and wrap the counters
    //
    if(vline++ >= 312) {    // If we've gone past the bottom scanline then
        vline = 1;		    // Reset the scanline counter
        bline = 0;		    // And the pixel buffer row index counter
        vblank_count++;
    }

    // Finally, clear the interrupt request ready for the next horizontal sync interrupt
    //
    dma_hw->ints0 = 1u << dma_channel_0;	
}

// Configure the PIO DMA
// Parameters:
// - pio: The PIO to attach this to
// - sm: The state machine number
// - dma_channel: The DMA channel
// - buffer_size_words: Number of bytes to transfer
// - handler: Address of the interrupt handler, or NULL for no interrupts
//
void cvideo_configure_pio_dma(PIO pio, uint sm, uint dma_channel, size_t buffer_size_words, irq_handler_t handler) {
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
    if(handler != NULL) {
        dma_channel_set_irq0_enabled(dma_channel, true);
        irq_set_exclusive_handler(DMA_IRQ_0, handler);
        irq_set_enabled(DMA_IRQ_0, true);
    }
}
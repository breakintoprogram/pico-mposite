//
// Title:	        Pico-mposite Video Output
// Description:		The composite video stuff
// Author:	        Dean Belfield
// Created:	        26/01/2021
// Last Updated:	25/02/2022
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
// 05/02/2022:      Added support for colour, fixed bug in video generation
// 20/02/2022:      Bitmap is now dynamically allocated; added two higher resolution video modes
// 25/02/2022:      Lengthened HSYNC to 12us

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
uint offset_0;                  // Program offsets
uint offset_1;

uint dma_channel_0;             // DMA channel for transferring sync data to PIO
uint dma_channel_1;             // DMA channel for transferring pixel data data to PIO
uint vline;                     // Current PAL(ish) video line being processed
uint bline;                     // Line in the bitmap to fetch

uint vblank_count;              // Vblank counter

unsigned char * bitmap;         // Bitmap buffer

int width = 256;                // Bitmap dimensions             
int height = 256;

/*
 * The sync tables consist of 32 entries, each one corresponding to a 2us slice of the 64us
 * horizontal sync. The value 0x00 is reserved as a control byte for the horizontal sync;
 * cvideo_sync will not write to the GPIO for that block of 0x00's.
 * 
 * All sync pulses are active low
 */

// Horizontal sync with gap for pixel data
//
unsigned short hsync[32] = {
    HSLO, HSLO, HSHI, HSHI, HSHI, HSHI, BORD, BORD, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, BORD, BORD, BORD,
};

// Horizontal sync for top and bottom borders
//
unsigned short border[32] = {
    HSLO, HSLO, HSHI, HSHI, HSHI, HSHI, BORD, BORD, BORD, BORD, BORD, BORD, BORD, BORD, BORD, BORD, 
    BORD, BORD, BORD, BORD, BORD, BORD, BORD, BORD, BORD, BORD, BORD, BORD, BORD, BORD, BORD, BORD, 
};

// Vertical sync (long/long)
//
unsigned short vsync_ll[32] = {
    VSLO, VSLO, VSLO, VSLO, VSLO, VSLO, VSLO, VSLO, VSLO, VSLO, VSLO, VSLO, VSLO, VSLO, VSLO, VSHI, // Long sync pulse
    VSLO, VSLO, VSLO, VSLO, VSLO, VSLO, VSLO, VSLO, VSLO, VSLO, VSLO, VSLO, VSLO, VSLO, VSLO, VSHI, // Long sync pulse
};

// Vertical sync (short/short)
//
unsigned short vsync_ss[32] = {
    VSLO, VSHI, VSHI, VSHI, VSHI, VSHI, VSHI, VSHI, VSHI, VSHI, VSHI, VSHI, VSHI, VSHI, VSHI, VSHI, // Short sync pulse
    VSLO, VSHI, VSHI, VSHI, VSHI, VSHI, VSHI, VSHI, VSHI, VSHI, VSHI, VSHI, VSHI, VSHI, VSHI, VSHI, // Short sync pulse
};

// Vertical sync (long/short)
//
unsigned short vsync_ls[32] = {
    VSLO, VSLO, VSLO, VSLO, VSLO, VSLO, VSLO, VSLO, VSLO, VSLO, VSLO, VSLO, VSLO, VSLO, VSLO, VSHI, // Long sync pulse
    VSLO, VSHI, VSHI, VSHI, VSHI, VSHI, VSHI, VSHI, VSHI, VSHI, VSHI, VSHI, VSHI, VSHI, VSHI, VSHI, // Short sync pulse
};

/*
 * The main routine sets up the whole shebang
 */
int initialise_cvideo(void) { 
    pio_0 = pio0;	                    // Assign the PIO

    // Load up the PIO programs
    //
    offset_0 = pio_add_program(pio_0, &cvideo_sync_program);
    offset_1 = pio_add_program(pio_0, &cvideo_data_program);

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
		gpio_base,								// Start pin in the GPIO
		gpio_count,								// Number of pins
		piofreq_0								// State machine clock frequency
	);	
    cvideo_configure_pio_dma(					// Configure the DMA for Sync
        pio_0,									// The PIO to attach this DMA to
        sm_sync,								// The state machine number
        dma_channel_0,							// The DMA channel
        DMA_SIZE_16,                            // Size of each transfer
        32,										// Number of bytes to transfer
        cvideo_dma_handler						// The DMA handler
    );
    pio_sm_set_enabled(pio_0, sm_sync, true);	// Enable the PIO state machine

    bitmap = malloc(width * height);            // Allocate the bitmap memory

	// Initialise the second PIO (pixel data)
	//
    cvideo_data_initialise_pio(					
		pio_0,
		sm_data,
		offset_1,
		gpio_base,
		gpio_count,
		piofreq_1_256
	);

    // Initialise the DMA for data (ie pixels)
    //
    cvideo_configure_pio_dma(
        pio_0,	
        sm_data,
        dma_channel_1,							// On DMA channel 1
        DMA_SIZE_8,                             // Size of each transfer
        width,									// The bitmap width
        NULL									// But there is no DMA interrupt for the pixel data
    ); 
    pio_sm_set_enabled(pio_0, sm_data, true);	// Enable the PIO state machine

    irq_set_exclusive_handler(					// Set up the PIO IRQ handler
		PIO0_IRQ_0,								// The IRQ #
		cvideo_pio_handler						// And handler routine
	); 
    pio0_hw->inte0 = PIO_IRQ0_INTE_SM0_BITS;	// Just for IRQ 0 (triggered by irq set 0 in PIO)
    irq_set_enabled(PIO0_IRQ_0, true);			// Enable it    

    set_border(0);                              // Set the border colour
    cls(0);                                     // Clear the screen      

    return 0;
}

// Set the graphics mode
// mode - The graphics mode (0 = 256x192, 1 = 320 x 192, 2 = 640 x 192)
//
int set_mode(int mode) {
    double dfreq;

    wait_vblank();

    switch(mode) {                              // Get the video mode
        case 1: 
            width = 320;                        // Set screen width and
            dfreq = piofreq_1_320;              // pixel dot frequency accordingly
            break;
        case 2: 
            width = 640;                
            dfreq = piofreq_1_640;
            break;
        default:
            width = 256;
            dfreq = piofreq_1_256;
            break;            

    }

    if(bitmap != NULL) {
        free(bitmap);
    }
    bitmap = malloc(width * height);            // Allocate the bitmap memory

    cvideo_configure_pio_dma(                   // Reconfigure the DMA
        pio_0,	
        sm_data,
        dma_channel_1,							// On DMA channel 1
        DMA_SIZE_8,                             // Size of each transfer
        width,									// The bitmap width
        NULL									// But there is no DMA interrupt for the pixel data
    ); 

    pio_0->sm[sm_data].clkdiv = (uint32_t) (dfreq * (1 << 16));

    return 0;
}

// Set the border colour
// - colour: Border colour
//
void set_border(unsigned char colour) {
    if(colour > colour_max) {
        return;
    }
    unsigned short c = BORD | (colour_base + colour);
    
    for(int i = 6; i <32; i++) {                // Skip the first three hsync values
        if(hsync[i] & BORD) {                   // If the border bit is set in the hsync
            hsync[i] = c;                       // Then write out the new colour (with the BORD bit set)
        }
        border[i] = c;                          // We can just write the values out to the border table
    }
}

// Wait for vblank
//
void wait_vblank(void) {
    uint c = vblank_count;                      // Get the current vblank count
    while(c == vblank_count) {                  // Wait until it changes
        sleep_us(4);                            // Need to sleep for a minimum of 4us
    }
}

// The PIO interrupt handler
// This sets up the DMA for cvideo_data with pixel data and is triggered by the irq set 0
// instruction at the end of the PIO
//
void cvideo_pio_handler(void) {
    if(bline >= height) {
        bline = 0;
    }
    dma_channel_set_read_addr(dma_channel_1, &bitmap[width * bline++], true);   // Line up the next block of pixels
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
        case 6 ... 36:
        case 293 ... 309:
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
// - transfer_size: Size of each DMA bus transfer (DMA_SIZE_8, DMA_SIZE_16 or DMA_SIZE_32)
// - buffer_size_words: Number of bytes to transfer
// - handler: Address of the interrupt handler, or NULL for no interrupts
//
void cvideo_configure_pio_dma(PIO pio, uint sm, uint dma_channel, uint transfer_size, size_t buffer_size, irq_handler_t handler) {
    dma_channel_config c = dma_channel_get_default_config(dma_channel);
    channel_config_set_transfer_data_size(&c, transfer_size);
    channel_config_set_read_increment(&c, true);
    channel_config_set_dreq(&c, pio_get_dreq(pio, sm, true));
    dma_channel_configure(dma_channel, &c,
        &pio->txf[sm],              // Destination pointer
        NULL,                       // Source pointer
        buffer_size,                // Size of buffer
        true                        // Start flag (true = start immediately)
    );
    if(handler != NULL) {
        dma_channel_set_irq0_enabled(dma_channel, true);
        irq_set_exclusive_handler(DMA_IRQ_0, handler);
        irq_set_enabled(DMA_IRQ_0, true);
    }
}
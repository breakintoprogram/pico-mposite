//
// Title:	        Pico-mposite Terminal Emulation
// Description:		Simple terminal emulation
// Author:	        Dean Belfield
// Created:	        19/02/2022
// Last Updated:	03/03/2022
//
// Modinfo:
// 03/03/2022:      Added colour

#include "pico/stdlib.h"

#include "hardware/pio.h"
#include "hardware/dma.h"
#include "hardware/irq.h"  
#include "hardware/uart.h" 

#include "cvideo.h"
#include "graphics.h"

#include "terminal.h"

int terminal_x;
int terminal_y;

void initialise_terminal(void) {
    uart_init(uart0, 115200);
    gpio_set_function(12, GPIO_FUNC_UART);  // TX
    gpio_set_function(13, GPIO_FUNC_UART);  // RX
}

// Handle carriage returns
//
void cr(void) {
    terminal_x = 0;
    terminal_y += 8;
    if(terminal_y >= height) {
        terminal_y -= 8;
        scroll_up(15, 8);
    }
}

// Advance one character position
//
void fs(void) {
    terminal_x += 8;
    if(terminal_x >= width) {
        cr();
    }
}

// Backspace
//
void bs(void) {
    terminal_x -= 8;
    if(terminal_x < 0) {
        terminal_x = 0;
    }
}

// The terminal loop
//
void terminal(void) {
    terminal_x = 0;
    terminal_y = 0;
    while(true) {
        print_char(terminal_x, terminal_y, '_', col_terminal_cursor, col_terminal_bg);
        char c = uart_getc(uart0);          // Get the character from the UART (blocking)
        if(c >= 32) {                       // Output printable characters
            print_char(terminal_x, terminal_y, c, col_terminal_fg, col_terminal_bg);
            fs();
        }
        else {                              // Else deal with the important control characters
            print_char(terminal_x, terminal_y, ' ', col_terminal_fg, col_terminal_bg);  
            switch(c) {
                case 0x08:  // Backspace
                    bs();
                    break;
                case 0x0D:  // CR
                    cr();
                    break;
                case 0x03:  // Ctrl+C       // Quit the terminal loop on these codes
                case 0x1B:  // ESC
                    return;
            }
        }
    }
}
//
// Title:	        Pico-mposite Terminal Emulation
// Author:	        Dean Belfield
// Created:	        19/02/2022
// Last Updated:	03/03/2022
//
// Modinfo:
// 03/03/2022:      Added colour

#pragma once

#include "config.h"

#if opt_colour == 0
    #define col_terminal_bg     15
    #define col_terminal_fg     0
    #define col_terminal_border 7
    #define col_terminal_cursor 15
#else
    #define col_terminal_bg     rgb(0,0,0)
    #define col_terminal_fg     rgb(7,7,0)
    #define col_terminal_border rgb(0,0,2)
    #define col_terminal_cursor rgb(7,7,7)
#endif 

void initialise_terminal(void);
void terminal(void);
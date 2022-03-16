//
// Title:	        Pico-mposite Video Output
// Author:	        Dean Belfield
// Created:	        26/01/2021
// Last Updated:	01/03/2022
//
// Modinfo:
// 20/02/2022:      Added demo_terminal
// 01/03/2022:      Added colour to the demos

#pragma once

#include "config.h"

#if opt_colour == 0
    #define col_black   0x00
    #define col_grey    0x0C
    #define col_white   0x0F
#else
    #define col_black   rgb(0,0,0)
    #define col_grey    rgb(6,6,6)
    #define col_white   rgb(7,7,7)
    #define col_red     rgb(7,0,0)
    #define col_green   rgb(0,7,0)
    #define col_blue    rgb(0,0,7)
    #define col_yellow  rgb(7,7,0)
    #define col_magenta rgb(7,0,7)
    #define col_cyan    rgb(0,7,7)
#endif

int main(void);

void demo_splash(void);
void demo_spinny_cube(void);
void demo_mandlebrot(void);
void demo_terminal(void);

void render_spinny_cube(int xo, int yo, double the, double psi, double phi, bool filled);
void render_mandlebrot(void);
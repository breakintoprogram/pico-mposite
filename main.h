//
// Title:	        Pico-mposite Video Output
// Author:	        Dean Belfield
// Created:	        26/01/2021
// Last Updated:	02/02/2022
//
// Modinfo:
// 

#pragma once

int main(void);

void demo_splash(void);
void demo_spinny_cube(void);
void demo_mandlebrot(void);

void render_spinny_cube(int xo, int yo, double the, double psi, double phi, int colour);
void render_mandlebrot(void);
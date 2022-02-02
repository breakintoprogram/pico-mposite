//
// Title:	        Pico-mposite Video Output
// Description:		A hacked-together composite video output for the Raspberry Pi Pico
// Author:	        Dean Belfield
// Created:	        02/02/2021
// Last Updated:	02/02/2022
// 

#include <stdlib.h>
#include <math.h>

#include "memory.h"
#include "pico/stdlib.h"

#include "hardware/pio.h"
#include "hardware/dma.h"
#include "hardware/irq.h"   

#include "bitmap.h"
#include "graphics.h"
#include "cvideo.h"

#include "main.h"

// Cube corner points
//
int shape_pts[8][8] = {
    { -20,  20,  20 },
    {  20,  20,  20 },
    { -20, -20,  20 },
    {  20, -20,  20 },
    { -20,  20, -20 },
    {  20,  20, -20 },
    { -20, -20, -20 },
    {  20, -20, -20 },
};

// Cube polygons (lines between corners)
//
int shape[6][4] = {
    { 0,1,3,2 },
    { 6,7,5,4 },
    { 1,5,7,3 },
    { 2,6,4,0 },
    { 2,3,7,6 },
    { 0,4,5,1 },
};

// The main loop
//
int main() {
    initialise_cvideo();    // Initialise the composite video stuff
    //
    // And then just loop doing your thing
    //
    while(true) {
        demo_splash();
        demo_spinny_cube();
        demo_mandlebrot();
    }
}

void demo_splash() {
    memcpy(bitmap, sample_bitmap, height * width);

    print_string(60, 8, "Pico-mposite v1.0", 0x10, 0x1f);
    print_string(64, 24, "By Dean Belfield", 0x10, 0x1f);
    print_string(24, 180, "www.breakintoprogram.co.uk", 0x10, 0x1f);

    sleep_ms(10000);
}

// Demo: Spinning 3D cube
//
void demo_spinny_cube() {
    double the = 0;
    double psi = 0;
    double phi = 0;

    for(int i = 0; i < 1000; i++) {
        wait_vblank();
        cls(0x1f);
        render_spinny_cube(0, 0, the, psi, phi, 0x10);
        print_string(0, 180, "Pico-mposite Graphics Primitives", 0x1f, 0x10);
        the += 0.01;
        psi += 0.03;
        phi -= 0.02;
    }
}

// Demo: Mandlebrot set
//
void demo_mandlebrot() {
    cls(0x10);
    render_mandlebrot();
    print_string(16, 180, "Pico-mposite Mandlebrot Demo", 0x10, 0x1f);
    sleep_ms(10000);
}

// Draw a 3D cube
// xo: X position in view
// yo: Y position in view
// the, psi, phi: Rotation angles
// colour: Pixel colour
//
void render_spinny_cube(int xo, int yo, double the, double psi, double phi, int colour) {
    int i;
    double x, y, z, xx, yy, zz;
    int a[8], b[8];
    int xd =0, yd = 0;
    int x1, y1, x2, y2, x3, y3, x4, y4;
    double sd = 512, od = 256;

    for(i = 0; i < 8 ; i++) {
        xx = shape_pts[i][0];
        yy = shape_pts[i][1];
        zz = shape_pts[i][2];

         y = yy * cos(phi) - zz * sin(phi);
        zz = yy * sin(phi) + zz * cos(phi);
         x = xx * cos(the) - zz * sin(the);
        zz = xx * sin(the) + zz * cos(the);
        xx =  x * cos(psi) -  y * sin(psi);
        yy =  x * sin(psi) +  y * cos(psi);

        xx += xo + xd;
        yy += yo + yd;

        a[i] = 128 + xx * sd / (od - zz);
        b[i] =  96 + yy * sd / (od - zz);
    }

    for(i = 0; i < 6; i++) {
        x1 = a[shape[i][0]];
        x2 = a[shape[i][1]];
        x3 = a[shape[i][2]];
        x4 = a[shape[i][3]];
        y1 = b[shape[i][0]];
        y2 = b[shape[i][1]];
        y3 = b[shape[i][2]];
        y4 = b[shape[i][3]];

        if(x1 * (y2 - y3) + x2 * (y3 - y1) + x3 * (y1 - y2) <= 0) {
            draw_line(x1, y1, x2, y2, colour);
            draw_line(x2, y2, x3, y3, colour);
            draw_line(x3, y3, x4, y4, colour);
            draw_line(x4, y4, x1, y1, colour);
        }
    }
}

// Draw a Mandlebrot
//
void render_mandlebrot(void) {
    int k = 0;
    float i , j , r , x , y = -96;
    while(y++<95) {
        for(x = 0; x < 256; x++) {
            plot(x, y + 96, 0x10+(k&0x0f));
            for(i = k = r = 0; j = r * r - i * i - 2 + x / 100, i = 2 * r * i + y / 70, j * j + i * i < 11 && k++ < 111; r = j);
        }
    }
}
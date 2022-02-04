//
// Title:	        Pico-mposite Graphics Primitives
// Description:		A hacked-together composite video output for the Raspberry Pi Pico
// Author:	        Dean Belfield
// Created:	        01/02/2021
// Last Updated:	03/02/2022
//
// Modinfo:
// 03/02/2022:      Fixed bug in print_char, typos in comments

#include <math.h>

#include "memory.h"
#include "charset.h"            // The character set

#include "graphics.h"

unsigned char bitmap[height][width];

// Clear the screen
// - c Colour (0x10 to 0x1f)
//
void cls(unsigned char c) {
    memset(bitmap, c, height * width);
}

// Print a character
// - x: X position on screen (pixels)
// - y: Y position on screen (pixels)
// - c: Character to print (ASCII 32 to 127)
// - bc: Background colour (0x10 to 0x1f)
// - fc: Foreground colour (0x10 to 0x1f)
//
void print_char(int x, int y, int c, unsigned char bc, unsigned char fc) {
    int char_index;

    if(c >= 32 && c < 128) {
        char_index = (c - 32) * 8;
        for(int row = 0; row < 8; row++) {
            unsigned char data = charset[char_index + row];
            for(int bit = 0; bit < 8; bit ++) {
                bitmap[y + row][x + 7 - bit] = data & 1 << bit ? fc : bc;
            }
        }
    }
}

// Print a string
// - x: X position on screen (pixels)
// - y: Y position on screen (pixels)
// - s: Zero terminated string
// - bc: Background colour (0x10 to 0x1f)
// - fc: Foreground colour (0x10 to 0x1f)
//
void print_string(int x, int y, char *s, unsigned char bc, unsigned char fc) {
    for(int i = 0; i < strlen(s); i++) {
        print_char(x + i * 8, y, s[i], bc, fc);
    }
}

// Plot a point
// - x: X position on screen
// - y: Y position on screen
// - c: Pixel colour (0x10 to 0x1f)
//
void plot(int x, int y, unsigned char c) {
    if(x >= 0 && x < width && y >= 0 && y < height) {
        bitmap[y][x] = c;
    }
}

// Draw a line
// - x1, y1: Coordinates of start point
// - x2, y2: Coordinates of last point
// - c: Pixel colour (0x10 to 0x1f)
//
void draw_line(int x1, int y1, int x2, int y2, unsigned char c) {
    int dx, dy, sx, sy, e, xp, yp, i;
    
    dx = x2 - x1;
    dy = y2 - y1;

    sx = (dx > 0) - (dx < 0);
    sy = (dy > 0) - (dy < 0);

    dx *= sx;
    dy *= sy;

    if(dx == 0 && dy == 0) {
        plot(xp, yp, c);
        return;
    }

    xp = x1;
    yp = y1;

    if(dx > dy) {
        e = dx / 2;
        dy -= dx;
        for(i = 0; i < dx; i++) {
            plot(xp, yp, c);
            e += dy;
            if(e > 0) {
                yp += sy;
            }
            else {
                e += dx;
            }
            xp += sx;
        }
    }
    else {
        e = dy / 2;
        dx -= dy;
        for(i = 0; i < dy; i++) {
            plot(xp, yp, c);
            e += dx;
            if(e > 0) {
                xp += sx;
            }
            else {
                e += dy;
            }
            yp += sy;
        }
    }
}

// Draw a circle
// - x: X Coordinate 
// - y: Y Coordinate
// - r: Radius
// - c: Pixel colour (0x10 to 0x1f)
//
void draw_circle(int x, int y, int r, unsigned char c) {
    int xp = 0;
    int yp = r;
    int d = 3 - 2 * r;
    while (yp >= xp) {
        plot(x + xp, y + yp, c);
        plot(x + xp, y - yp, c);   
        plot(x - xp, y + yp, c);   
        plot(x - xp, y - yp, c);           
        plot(x + yp, y + xp, c);
        plot(x + yp, y - xp, c);   
        plot(x - yp, y + xp, c);   
        plot(x - yp, y - xp, c);           
        xp++;
        if (d > 0) {
            yp--;
            d += 4 * (xp - yp) + 10;
        }
        else {
            d += 4 * xp + 6;
        }
    }
}
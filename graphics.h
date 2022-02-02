//
// Title:	        Pico-mposite Graphics Primitives
// Author:	        Dean Belfield
// Created:	        01/02/2022
// Last Updated:	01/02/2022
//
// Modinfo:
// 

#pragma once

#define width 256               // Bitmap width in pixels
#define height 192              // Bitmap height in pixels

unsigned char bitmap[height][width];

void cls(unsigned char c);

void print_char(int x, int y, int c, unsigned char bc, unsigned char fc);
void print_string(int x, int y, char *s, unsigned char bc, unsigned char fc);

void plot(int x, int y, unsigned char c);
void draw_line(int x1, int y1, int x2, int y2, unsigned char c);
void draw_circle(int x, int y, int r, unsigned char c);

//
// Title:	        Pico-mposite Graphics Primitives
// Author:	        Dean Belfield
// Created:	        01/02/2022
// Last Updated:	02/03/2022
//
// Modinfo:
// 07/02/2022:      Added support for filled primitives
// 20/02/2022:      Added scroll_up, bitmap now initialised in cvideo.c
// 02/03/2022:      Added blit

#pragma once

#include <stdbool.h>

#define rgb(r,g,b) (((b&6)<<5)|(g<<3)|r)

struct Line {
    int  dx, dy, sx, sy, e, xp, yp, h;
    bool quad;
};

void cls(unsigned char c);
void scroll_up(unsigned char c, int rows);

void print_char(int x, int y, int c, unsigned char bc, unsigned char fc);
void print_string(int x, int y, char *s, unsigned char bc, unsigned char fc);

void plot(int x, int y, unsigned char c);
void draw_line(int x1, int y1, int x2, int y2, unsigned char c);
void draw_horizontal_line(int y1, int x1, int x2, int c);
void draw_circle(int x, int y, int r, unsigned char c, bool filled);
void draw_polygon(int x1, int y1, int x2, int y2, int x3, int y3, int x4, int y4, unsigned char c, bool filled);
void draw_rect(int x1, int y1, int x2, int y2, unsigned char c, bool filled);
void draw_triangle(int x1, int y1, int x2, int y2, int x3, int y3, unsigned char c, bool filled);

void swap(int *a, int *b);
void init_line(struct Line *line, int x1, int y1, int x2, int y2);
void step_line(struct Line *line);

void blit(const void * data, int sx, int sy, int sw, int sh, int dx, int dy);

#include <SDL.h>
#include <stdio.h>
#include <string.h>

/*

 Copyright (c) 2021 Nobody Industries

 Permission to use, copy, modify, and distribute this software for any
 purpose with or without fee is hereby granted, provided that the above
 copyright notice and this permission notice appear in all copies.

 THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 WITH REGARD TO THIS SOFTWARE.

*/

#pragma mark DEFINITIONS

#define APP_NAME "Grid"
#define WINDOW_WIDTH 640
#define WINDOW_HEIGHT 480

#define FALSE 0
#define TRUE 1

#define CELL_SIZE 20
#define CELL_PAD 2
#define ICON_SIZE 40
#define ICON_PAD 5
#define FPS 30
#define N_CELLS_X (WINDOW_WIDTH / CELL_SIZE)
#define N_CELLS_Y ((WINDOW_HEIGHT - ICON_SIZE) / CELL_SIZE)
#define N_BUTTONS 4
#define N_SPEEDS 5

#pragma mark TYPES

typedef unsigned char bool;
typedef unsigned char byte;
typedef enum {
	WHITE = 0,
	BLACK = 1,
} color;
typedef enum {
	BUTTON_ON = 0,
	BUTTON_OFF = 1,
	BUTTON_SPEED_UP = 2,
	BUTTON_SPEED_DOWN = 3,
	BUTTON_CLEAR = 4,
} icon;
typedef struct _button {
	int x, y;
	int pad;
	int scale;
	bool state;
	icon icn_true;
	icon icn_false;
	void (*on_click)(struct _button *button);
} button;

#pragma mark GLOBAL CONSTANTS

const Uint32 color_values[] = {0xFFFFFF, 0x000000};
const byte icons[][8] = {
	{0x00, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x00}, /* on */
	{0x00, 0x3C, 0x7E, 0x66, 0x66, 0x7E, 0x3C, 0x00}, /* off */
	{0x00, 0x48, 0x6C, 0x7E, 0x7E, 0x6C, 0x48, 0x00}, /* speed up */
	{0x00, 0x12, 0x36, 0x7E, 0x7E, 0x36, 0x12, 0x00}, /* speed down */
	{0x00, 0x60, 0x70, 0x38, 0x1C, 0x0E, 0x06, 0x00}, /* clear */
};
const int speeds[] = {60, 30, 15, 10, 5};

#pragma mark GLOBAL VARIABLES

SDL_Window *window;
SDL_Renderer *renderer;
SDL_Texture *texture;
Uint32 *pixels;
bool *cells, *ncells;
bool running = FALSE;
button *buttons;
int speed_idx;

#pragma mark UTILS

void
error(const char *msg, const char *err)
{
	printf("ERROR: %s [%s]", msg, err);
}

int
arrayIdx(int x, int y, int width)
{
	return x + (y * width);
}

bool
point_in_rectangle(int px, int py, int rx, int ry, int rw, int rh)
{
	if((px > rx) && (px < rx + rw) && (py > ry) && (py < ry + rh))
		return TRUE;
	return FALSE;
}

#pragma mark PROGRAM LOGIC

int
count_neighbors_on(int x, int y)
{
	int count = 0;
	int nx, ny;
	for(int dx = -1; dx <= 1; dx++)
		for(int dy = -1; dy <= 1; dy++) {
			if(dx != 0 || dy != 0) {
				nx = (x + dx) >= N_CELLS_X ? 0 : (x + dx);
				nx = nx < 0 ? N_CELLS_X - 1 : nx;
				ny = (y + dy) >= N_CELLS_Y ? 0 : (y + dy);
				ny = ny < 0 ? N_CELLS_Y - 1 : ny;
				if(cells[arrayIdx(nx, ny, N_CELLS_X)] == TRUE)
					count++;
			}
		}
	return count;
}

void
update_cells()
{
	memset(ncells, FALSE, N_CELLS_X * N_CELLS_Y * sizeof(bool));
	for(int y = 0; y < N_CELLS_Y; y++) {
		for(int x = 0; x < N_CELLS_X; x++) {
			int n = count_neighbors_on(x, y);
			int idx = arrayIdx(x, y, N_CELLS_X);
			if(cells[idx] == TRUE) {
				if(n < 2)
					ncells[idx] = FALSE;
				else if(n <= 3)
					ncells[idx] = TRUE;
				else
					ncells[idx] = FALSE;
			} else {
				if(n == 3)
					ncells[idx] = TRUE;
			}
		}
	}
	memcpy(cells, ncells, N_CELLS_X * N_CELLS_Y * sizeof(bool));
}

#pragma mark UI EVENTS

void
play_on_click(button *button)
{
	if(button->state == TRUE) {
		button->state = FALSE;
		running = FALSE;
	} else {
		button->state = TRUE;
		running = TRUE;
	}
}

void
speed_up_on_click(button *button)
{
	if(speed_idx + 1 < N_SPEEDS)
		speed_idx++;
}

void
speed_down_on_click(button *button)
{
	if(speed_idx - 1 >= 0)
		speed_idx--;
}

void
clear_on_click(button *button)
{
	if(running == FALSE) {
		memset(cells, FALSE, N_CELLS_X * N_CELLS_Y * sizeof(bool));
		memset(ncells, FALSE, N_CELLS_X * N_CELLS_Y * sizeof(bool));
	}
}

#pragma mark INITIALIZATION & DESTRUCTION

void
init_app()
{
	// Initialization of global variables
	speed_idx = 2;
	// SDL & Memory allocation
	if(SDL_Init(SDL_INIT_VIDEO) < 0) {
		error("SDL_Init Failed", SDL_GetError());
		exit(1);
	}
	window = SDL_CreateWindow(
		APP_NAME,
		SDL_WINDOWPOS_UNDEFINED,
		SDL_WINDOWPOS_UNDEFINED,
		WINDOW_WIDTH,
		WINDOW_HEIGHT,
		SDL_WINDOW_SHOWN);
	if(window == NULL) {
		error("SDL_CreateWindow Failed", SDL_GetError());
		goto error_window;
	}
	renderer = SDL_CreateRenderer(window, -1, 0);
	if(renderer == NULL) {
		error("SDL_CreateRenderer Failed", SDL_GetError());
		goto error_renderer;
	}
	texture = SDL_CreateTexture(
		renderer,
		SDL_PIXELFORMAT_RGB888,
		SDL_TEXTUREACCESS_STATIC,
		WINDOW_WIDTH,
		WINDOW_HEIGHT);
	if(texture == NULL) {
		error("SDL_CreateTexture Failed", SDL_GetError());
		goto error_texture;
	}
	pixels = (Uint32 *)calloc(WINDOW_HEIGHT * WINDOW_WIDTH, sizeof(Uint32));
	if(pixels == NULL) {
		error("Allocation error", "pixels - init");
		goto error_pixels;
	}
	cells = (bool *)calloc(N_CELLS_X * N_CELLS_Y, sizeof(bool));
	if(cells == NULL) {
		error("Allocation error", "cells - init");
		goto error_cells;
	}
	buttons = (button *)calloc(N_BUTTONS, sizeof(button));
	if(buttons == NULL) {
		error("Allocation error", "buttons - init");
		goto error_buttons;
	}
	ncells = (bool *)calloc(N_CELLS_X * N_CELLS_Y, sizeof(bool));
	if(ncells == NULL) {
		error("Allocation error", "cells - update");
		goto error_ncells;
	}
	return;
	// Error handling
error_ncells:
	free(buttons);
error_buttons:
	free(cells);
error_cells:
	free(pixels);
error_pixels:
	SDL_DestroyTexture(texture);
error_texture:
	SDL_DestroyRenderer(renderer);
error_renderer:
	SDL_DestroyWindow(window);
error_window:
	SDL_Quit();
	exit(1);
}

void
init_ui()
{
	buttons[0].x = 0;
	buttons[0].y = WINDOW_HEIGHT - ICON_SIZE;
	buttons[0].pad = ICON_PAD;
	buttons[0].scale = 4;
	buttons[0].state = running;
	buttons[0].icn_true = BUTTON_OFF;
	buttons[0].icn_false = BUTTON_ON;
	buttons[0].on_click = play_on_click;
	buttons[1].x = ICON_SIZE;
	buttons[1].y = WINDOW_HEIGHT - ICON_SIZE;
	buttons[1].pad = ICON_PAD;
	buttons[1].scale = 4;
	buttons[1].state = TRUE;
	buttons[1].icn_true = BUTTON_SPEED_DOWN;
	buttons[1].icn_false = BUTTON_SPEED_DOWN;
	buttons[1].on_click = speed_down_on_click;
	buttons[2].x = ICON_SIZE * 2;
	buttons[2].y = WINDOW_HEIGHT - ICON_SIZE;
	buttons[2].pad = ICON_PAD;
	buttons[2].scale = 4;
	buttons[2].state = TRUE;
	buttons[2].icn_true = BUTTON_SPEED_UP;
	buttons[2].icn_false = BUTTON_SPEED_UP;
	buttons[2].on_click = speed_up_on_click;
	buttons[3].x = ICON_SIZE * 3;
	buttons[3].y = WINDOW_HEIGHT - ICON_SIZE;
	buttons[3].pad = ICON_PAD;
	buttons[3].scale = 4;
	buttons[3].state = TRUE;
	buttons[3].icn_true = BUTTON_CLEAR;
	buttons[3].icn_false = BUTTON_CLEAR;
	buttons[3].on_click = clear_on_click;
}

void
destroy()
{
	free(ncells);
	free(buttons);
	free(cells);
	free(pixels);
	SDL_DestroyTexture(texture);
	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);
	SDL_Quit();
}

#pragma mark DRAW

void
clear(color c)
{
	for(int idx = 0; idx < WINDOW_HEIGHT * WINDOW_WIDTH; idx++)
		pixels[idx] = color_values[c];
}

void
putpixel(int x, int y, color c)
{
	if(x >= 0 && x < WINDOW_WIDTH && y >= 0 && y < WINDOW_HEIGHT)
		pixels[arrayIdx(x, y, WINDOW_WIDTH)] = color_values[c];
}

void
draw_rect(int x, int y, int w, int h, bool filled, color c)
{
	for(int cy = y; cy < y + h; cy++)
		for(int cx = x; cx < x + w; cx++) {
			if((filled == FALSE && (cx == x || cy == y || cx == (x + w - 1) ||
									cy == (y + h - 1))) ||
			   (filled == TRUE))
				putpixel(cx, cy, c);
		}
}

void
draw_grid()
{
	int idx = 0;
	for(int cy = 0; cy < WINDOW_HEIGHT - ICON_SIZE; cy += CELL_SIZE)
		for(int cx = 0; cx < WINDOW_WIDTH; cx += CELL_SIZE, idx++)
			draw_rect(
				cx + CELL_PAD,
				cy + CELL_PAD,
				CELL_SIZE - CELL_PAD,
				CELL_SIZE - CELL_PAD,
				cells[idx],
				WHITE);
}

void
draw_button_icon(
	int x,
	int y,
	int pad,
	int scale,
	icon icn,
	color color_foreground,
	color color_background)
{
	int v, h, cx, cy;
	byte is_on;
	for(v = 0; v < 8; v++)
		for(h = 0; h < 8; h++) {
			is_on = (icons[icn][v] >> (7 - h)) & 0x01;
			cx = x + pad + (h * scale);
			cy = y + pad + (v * scale);
			for(int ny = 0; ny < scale; ny++)
				for(int nx = 0; nx < scale; nx++)
					putpixel(
						cx + nx,
						cy + ny,
						is_on ? color_foreground : color_background);
		}
}

void
draw_ui()
{
	for(int i = 0; i < N_BUTTONS; i++) {
		button cButton = buttons[i];
		icon icn = cButton.state == TRUE ? cButton.icn_true : cButton.icn_false;
		draw_button_icon(
			cButton.x,
			cButton.y,
			cButton.pad,
			cButton.scale,
			icn,
			BLACK,
			WHITE);
	}
}

void
redraw()
{
	clear(BLACK);
	draw_grid();
	draw_ui();
	SDL_UpdateTexture(texture, NULL, pixels, WINDOW_WIDTH * sizeof(Uint32));
	SDL_RenderClear(renderer);
	SDL_RenderCopy(renderer, texture, NULL, NULL);
	SDL_RenderPresent(renderer);
}

#pragma mark SDL

void
on_mouse_event(SDL_Event *evt)
{
	int idx;
	switch(evt->type) {
	case SDL_MOUSEMOTION:
	case SDL_MOUSEBUTTONUP: break;
	case SDL_MOUSEBUTTONDOWN:
		// Grid check
		if(running == FALSE) {
			idx = 0;
			for(int cy = 0; cy < WINDOW_HEIGHT - ICON_SIZE; cy += CELL_SIZE)
				for(int cx = 0; cx < WINDOW_WIDTH; cx += CELL_SIZE, idx++)
					if(point_in_rectangle(
						   evt->button.x,
						   evt->button.y,
						   cx + CELL_PAD,
						   cy + CELL_PAD,
						   CELL_SIZE - CELL_PAD,
						   CELL_SIZE - CELL_PAD)) {
						cells[idx] = cells[idx] == TRUE ? FALSE : TRUE;
					}
		}
		// Button check
		for(int i = 0; i < N_BUTTONS; i++) {
			if(point_in_rectangle(
				   evt->button.x,
				   evt->button.y,
				   buttons[i].x,
				   buttons[i].y,
				   ICON_SIZE,
				   ICON_SIZE)) {
				buttons[i].on_click(&buttons[i]);
			}
		}
		break;
	}
}

#pragma mark MAIN PROGRAM

int
main()
{
	int tick_next = 0;
	int frame_next_cells = 0;
	SDL_Event event;
	init_app();
	init_ui();
	redraw();
	while(1) {
		int tick = SDL_GetTicks();
		if(tick < tick_next)
			SDL_Delay(tick_next - tick);
		tick_next = tick + (1000 / FPS);
		if(running) {
			frame_next_cells++;
			if(frame_next_cells >= speeds[speed_idx]) {
				update_cells();
				frame_next_cells = 0;
			}
		} else {
			frame_next_cells = speeds[speed_idx] - 1;
		}
		redraw();
		while(SDL_PollEvent(&event) != 0) {
			switch(event.type) {
			case SDL_QUIT: destroy(); return 0;
			case SDL_MOUSEBUTTONUP:
			case SDL_MOUSEBUTTONDOWN:
			case SDL_MOUSEMOTION: on_mouse_event(&event); break;
			}
		}
	}
	destroy();
	return 0;
}

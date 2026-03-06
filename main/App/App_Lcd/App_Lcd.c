/*
 * Author: xbai
 * Date  : 2026/03/05
 */

#include "Bsp_Lcd.h"
#include "Bsp/Bsp_Lcd/Bsp_Lcd.h"
#include <math.h>
#include <stdlib.h>

#define LCD_W 240
#define LCD_H 240

#define COLOR_SPACE       0x0000
#define COLOR_ORBIT       0x2104
#define COLOR_STAR_DIM    0x7BEF
#define COLOR_STAR_BRIGHT 0xFFFF

#define COLOR_EARTH      0x07FF
#define COLOR_MARS       0xF800
#define COLOR_VENUS      0xFD20
#define COLOR_MERCURY    0xC618
#define COLOR_JUPITER    0xFFE0
#define COLOR_SATURN     0xF7E0

#define COLOR_SCANLINE   0x18C3

static uint16_t framebuffer[LCD_W * LCD_H] __attribute__((aligned(4)));

typedef struct
{
    float radius;
    float angle;
    float speed;
    uint8_t size;
    uint16_t color;
}Planet;

typedef struct
{
    int x;
    int y;
    uint8_t brightness;
} star_t;

#define STAR_COUNT 60
star_t stars[STAR_COUNT];

Planet planets[] =
{
    {20,0,0.08,2,COLOR_MERCURY}, // Mercury
    {35,0,0.06,3,COLOR_VENUS},   // Venus
    {50,0,0.05,3,COLOR_EARTH},   // Earth
    {65,0,0.04,2,COLOR_MARS},    // Mars
    {80,0,0.02,5,COLOR_JUPITER}, // Jupiter
    {95,0,0.015,4,COLOR_SATURN}  // Saturn
};

int comet_x = -20;
int comet_y = 30;
float saturn_angle = 240;
float star_rotation = 0.0f;
float sun_phase = 0.0f;

static void draw_stars(void);
static void draw_sun(int cx,int cy);
static void draw_orbits(int cx,int cy);
static void draw_planets(int cx,int cy);
static void draw_saturn(int cx,int cy,int r,float angle);
static void draw_comet(void);
static void draw_scanline(void);
static void gfx_clear(uint16_t color);
static void gfx_draw_pixel(int x, int y, uint16_t color);
static void gfx_draw_circle(int cx, int cy, int r, uint16_t color);
static void gfx_fill_circle(int cx, int cy, int r, uint16_t color);
static void gfx_draw_line(int x0, int y0, int x1, int y1, uint16_t color);
static void gfx_present(void);

void stars_init(void)
{
    for(int i = 0; i < STAR_COUNT; i++)
    {
        stars[i].x = rand() % LCD_W;
        stars[i].y = rand() % LCD_H;
        stars[i].brightness = rand() % 2;
    }
}

static void draw_stars(void)
{
    int cx = 120;
    int cy = 120;

    for(int i = 0; i < STAR_COUNT; i++)
    {
        float dx = stars[i].x - cx;
        float dy = stars[i].y - cy;

        float rx = dx * cosf(star_rotation) - dy * sinf(star_rotation);
        float ry = dx * sinf(star_rotation) + dy * cosf(star_rotation);

        int x = cx + rx;
        int y = cy + ry;

        uint16_t color = stars[i].brightness ? 0xFFFF : 0x7BEF;

        gfx_draw_pixel(x,y,color);

        stars[i].brightness ^= 1;
    }
}

static void draw_sun(int cx,int cy)
{
    int pulse = 2 * sinf(sun_phase);

    gfx_fill_circle(cx,cy,18+pulse,0xFFE8);
    gfx_fill_circle(cx,cy,14+pulse,0xFFE0);
    gfx_fill_circle(cx,cy,10+pulse,0xFFD0);
    gfx_fill_circle(cx,cy,6+pulse,0xFFFF);
}

static void draw_orbits(int cx,int cy)
{
    for(int i = 0; i < 6; i++)
    {
        gfx_draw_circle(cx, cy, planets[i].radius, COLOR_ORBIT);
    }
}

static void draw_planets(int cx,int cy)
{
    for(int i = 0; i < 6; i++)
    {
        if(i != 5)
        {
            planets[i].angle += planets[i].speed;

            int x = cx + planets[i].radius * cosf(planets[i].angle);
            int y = cy + planets[i].radius * sinf(planets[i].angle);

            gfx_fill_circle(x, y, planets[i].size, planets[i].color);
        }
        else
        {
            draw_saturn(cx,cy,100,saturn_angle);
        }
    }
}

static void draw_saturn(int cx,int cy,int r,float angle)
{
    int x = cx + r*cosf(angle);
    int y = cy + r*sinf(angle);

    gfx_fill_circle(x,y,4,0xFFE0);

    gfx_draw_line(x-6,y,x+6,y,0xC618);
}

static void draw_comet(void)
{
    for(int i = 0; i < 6; i++)
    {
        gfx_fill_circle(comet_x-i*3, comet_y, 1, 0x7BEF);
    }

    gfx_fill_circle(comet_x, comet_y, 2, 0xFFFF);

    comet_x += 2;

    if(comet_x > 260)
    {
        comet_x = -20;
        comet_y = rand()%200;
    }
}

static void draw_scanline(void)
{
    static int y = 0;

    gfx_draw_line(0,y,240,y,0x2104);

    y+=2;
    if(y>240) y=0;
}

static void gfx_clear(uint16_t color)
{
    for(int i = 0; i < LCD_W * LCD_H; i++)
    {
        framebuffer[i] = color;
    }
}

static void gfx_draw_pixel(int x, int y, uint16_t color)
{
    if(x < 0 || x >= LCD_W)
    {
        return;
    }

    if(y < 0 || y >= LCD_H)
    {
        return;
    }

    framebuffer[y * LCD_W + x] = color;
}

static void gfx_draw_circle(int cx, int cy, int r, uint16_t color)
{
    for (int i = 0; i < 360; i++)
    {
        float rad = i * 3.14159f / 180.0f;

        int x = cx + r * cosf(rad);
        int y = cy + r * sinf(rad);

        gfx_draw_pixel(x, y, color);
    }
}

static void gfx_fill_circle(int cx, int cy, int r, uint16_t color)
{
    for(int y = -r; y <= r; y++)
    {
        for(int x = -r; x <= r; x++)
        {
            if(x*x + y*y <= r*r)
            {
                gfx_draw_pixel(cx + x, cy + y, color);
            }
        }
    }
}

static void gfx_draw_line(int x0, int y0, int x1, int y1, uint16_t color)
{
    int dx = abs(x1 - x0);
    int dy = -abs(y1 - y0);

    int sx = x0 < x1 ? 1 : -1;
    int sy = y0 < y1 ? 1 : -1;

    int err = dx + dy;

    while (1)
    {
        gfx_draw_pixel(x0, y0, color);

        if (x0 == x1 && y0 == y1)
            break;

        int e2 = 2 * err;

        if (e2 >= dy)
        {
            err += dy;
            x0 += sx;
        }

        if (e2 <= dx)
        {
            err += dx;
            y0 += sy;
        }
    }
}

static void gfx_present(void)
{
    esp_lcd_panel_draw_bitmap(panel_handle, 0, 0, 240, 240, framebuffer);
}

void App_Solar_Render(void)
{
    gfx_clear(0x0000);
    int cx = 120;
    int cy = 120;

    draw_stars();
    draw_orbits(cx, cy);
    draw_sun(cx, cy);
    draw_planets(cx, cy);
    draw_comet();
    draw_scanline();
    saturn_angle += 0.006;
    star_rotation += 0.0005f;
    sun_phase += 0.05f;

    gfx_present();
}
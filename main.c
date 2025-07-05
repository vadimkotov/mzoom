#include <stdbool.h>
#include <stdio.h>
#include <math.h>

#include "raylib.h"

#define SCREEN_WIDTH 800
#define SCREEN_HEIGHT 600
#define MAX_ITERATIONS 100
#define ZOOM_FACTOR 0.8

typedef long double real_t;

real_t mandelbrot(real_t cr, real_t ci, int max_iterations) {
  /* Mandelbrot set formula:
   * z(n+1) = z(n)**2 + c, where z(0) = 0
   */

  real_t zr = 0.0L;
  real_t zi = 0.0L;
  
  for (int i = 0; i < max_iterations; ++i) {
    real_t zr_new = zr * zr - zi * zi + cr;
    zi = 2 * zr * zi + ci;
    zr = zr_new;

    real_t zabs_squared = zr * zr + zi * zi;
    // Instead of taking sqrt(z) and comparing with 2.0,
    // we work on z**2 and compare with 4.0.
    if (zabs_squared > 4.0L) {
      /* nu is an approximation of the Green's function, which
       * reflects how fast the iteration escapes to infinity.
       */
      real_t nu = (real_t)i  + 1.0L - log2l(log2l(zabs_squared));
      return nu;
    }
  }
  return -1.0L;
}

int main(void) {
  InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "MZOOM");

  SetTargetFPS(30);

  /* Mandelbrot lives in [-2, 2]/[-2, 2] square, so we need
   * to map screen coordinates to is. However, for a prettier
   * more centered image it is recommended to use:
   * - [-2.0, 1.0] for the real part
   * - [-1.5, 1.5] for the imaginary
   */
  real_t center_point_real = 0.0L; // 0.74364388703L;
  real_t center_point_imag = 0.0L; // 0.13182590421L;

  real_t width = 3.0L;
  real_t height = width * ((real_t)SCREEN_HEIGHT / SCREEN_WIDTH);

  real_t real_min = center_point_real - width * 0.5L;
  real_t imag_min = center_point_imag - height * 0.5L;

  real_t scalex = width / (real_t)SCREEN_WIDTH;
  real_t scaley = height / (real_t)SCREEN_HEIGHT;

  Texture2D texture = LoadTextureFromImage(GenImageColor(SCREEN_WIDTH, SCREEN_HEIGHT, BLACK));
  Color buffer[SCREEN_WIDTH * SCREEN_HEIGHT * sizeof(Color)] = {};

  while (!WindowShouldClose()) {
    
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
      width *= ZOOM_FACTOR;
      height *= ZOOM_FACTOR;
      
      Vector2 mouse_pos = GetMousePosition();

      real_t mouse_real = real_min + scalex * (mouse_pos.x + 0.5L);
      real_t mouse_imag = imag_min + scalex * ((real_t)(SCREEN_HEIGHT - mouse_pos.y - 1) + 0.5L);

      center_point_real = mouse_real;
      center_point_imag = mouse_imag;
      

      real_min = center_point_real - width * 0.5L;
      imag_min = center_point_imag - height * 0.5L;

      scalex = width / (real_t)SCREEN_WIDTH;
      scaley = height / (real_t)SCREEN_HEIGHT;
    }

    int max_iterations = 64 + 4 * log10l(1.0L / width);
    
    BeginDrawing();
    ClearBackground(BLACK);
    
    for (int y = 0; y < SCREEN_HEIGHT; ++y) {
      real_t imag = scaley * ((real_t)(SCREEN_HEIGHT-y-1) + 0.5L) + imag_min;

      for (int x = 0; x < SCREEN_WIDTH; ++x) {
        real_t real = scalex * ((real_t)x + 0.5L) + real_min;

        real_t nu = mandelbrot(real, imag, max_iterations);

        if (nu > -1.0L) {
          int color = nu * 10.0L;
          buffer[y * SCREEN_WIDTH + x] = ColorFromHSV(color, 0.8f, 0.8f);
        } else {
          buffer[y * SCREEN_WIDTH + x] = BLACK;
        }
      }
    }

    UpdateTexture(texture, buffer);
    DrawTexture(texture, 0, 0, WHITE);

    EndDrawing();
  }
  CloseWindow();
  return 0;
}

#include <complex.h>
#include <stdbool.h>
#include <stdio.h>
#include <math.h>

#include "raylib.h"

#define SCREEN_WIDTH 800
#define SCREEN_HEIGHT 600
#define MAX_ITERATIONS 100

float mandelbrot(float real, float imag) {
  /* Mandelbrot set formula:
   * z(n+1) = z(n)**2 + c, where z(0) = 0
   */
  float complex z = 0; 
  float complex c = real + I * imag;
  for (int i = 0; i < MAX_ITERATIONS; ++i) {
    z = cpow(z, 2.0f) + c;
    float zabs = cabsf(z);
    if (zabs > 2.0f) {
      /* nu is an approximation of the Green's function, which
       * reflects how fast the iteration escapes to infinity.
       */
      float nu = (float)i  + 1.0f - log2f(log2f(zabs));
      return nu;
    }
  }
  return -1.0f;
}

int main(void) {
  InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "MZOOM");

  SetTargetFPS(60);

  while (!WindowShouldClose()) {
    BeginDrawing();
    ClearBackground(BLACK);

    for (int x = 0; x < SCREEN_WIDTH; ++x) {
      for (int y = 0; y < SCREEN_HEIGHT; ++y) {
        /* Mandelbrot lives in [-2, 2]/[-2, 2] square, so we need
         * to map screen coordinates to is. However, for a prettier
         * more centered image it is recommended to use:
         * - [-2.0, 1.0] for the real part
         * - [-1.5, 1.5] for the imaginary
         */

        float real = 3.0f / (float)SCREEN_WIDTH * (float)x - 2.0f;
        float imag = 3.0f / (float)SCREEN_HEIGHT * (float)(SCREEN_HEIGHT-y) - 1.5f;
        float nu = mandelbrot(real, imag);
        if (nu > -1.0f) {
          int color = nu * 10.0f;
          DrawPixel(x, y, ColorFromHSV(color, 0.8f, 0.8f));
        }
      }
    }

    EndDrawing();
  }
  CloseWindow();
  return 0;
}

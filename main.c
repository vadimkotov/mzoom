#include <stdatomic.h>
#include <stdbool.h>
#include <stdio.h>
#include <threads.h>
#include <math.h>

#include "raylib.h"

#define SCREEN_WIDTH 800
#define SCREEN_HEIGHT 600
#define TEXTURE_BUFSIZE SCREEN_WIDTH*SCREEN_HEIGHT*sizeof(Color)
#define MAX_ITERATIONS 100
#define ZOOM_FACTOR 0.8

typedef long double real_t;

typedef struct {
  Color* front;
  Color* back;
  real_t width;
  real_t height;
  real_t center_real;
  real_t center_imag;
  real_t real_min;
  real_t imag_min;
  real_t scalex;
  real_t scaley;
  atomic_bool dirty;
  atomic_bool ready;
  bool quit;
  mtx_t swap_lock;
} State;

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

int worker(void* arg) {
  State* state = arg;
  while(!state->quit) {
    if (!atomic_load(&state->dirty)) {
      thrd_yield();
      continue;
    }

    int max_iterations = 64 + 4 * log10l(1.0L / state->width);

    for (int y = 0; y < SCREEN_HEIGHT; ++y) {
      real_t imag = state->scaley * ((real_t)(SCREEN_HEIGHT-y-1) + 0.5L) + state->imag_min;

      for (int x = 0; x < SCREEN_WIDTH; ++x) {
        real_t real = state->scalex * ((real_t)x + 0.5L) + state->real_min;

        real_t nu = mandelbrot(real, imag, max_iterations);

        if (nu > -1.0L) {
          int color = nu * 10.0L;
          state->back[y * SCREEN_WIDTH + x] = ColorFromHSV(color, 0.8f, 0.8f);
        } else {
          state->back[y * SCREEN_WIDTH + x] = BLACK;
        }
      }
    }
    // TODO: should this be here?
    atomic_store(&state->ready, false);
    mtx_lock(&state->swap_lock);
    // Swap buffers
    Color* tmp = state->front;
    state->front = state->back;
    state->back = tmp;
    mtx_unlock(&state->swap_lock);

    atomic_store(&state->ready, true);
    atomic_store(&state->dirty, false);
  }
  printf("[WORKER] Done\n");
  return 0;
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
  Texture2D texture = LoadTextureFromImage(GenImageColor(SCREEN_WIDTH, SCREEN_HEIGHT, BLACK));

  real_t width = 3.0L;
  real_t height = width * ((real_t)SCREEN_HEIGHT / SCREEN_WIDTH);
  real_t center_real = -0.5L;
  real_t center_imag = 0.0L;

  State state = {
    .width = width,
    .height = height,
    .center_real = center_real,
    .center_imag = center_imag,
    .real_min = center_real - width * 0.5L,
    .imag_min = center_imag - height * 0.5L,
    .scalex = width / (real_t)SCREEN_WIDTH,
    .scaley = height / (real_t)SCREEN_HEIGHT,
    .front = MemAlloc(TEXTURE_BUFSIZE),
    .back =  MemAlloc(TEXTURE_BUFSIZE),
    .dirty = ATOMIC_VAR_INIT(true),
    .ready = ATOMIC_VAR_INIT(false),
    .quit = false,
  };

  // TODO: check for failure?
  mtx_init(&state.swap_lock, mtx_plain);

  thrd_t thr;
  thrd_create(&thr, worker, &state);

  while (!WindowShouldClose()) {

    bool view_changed = false;

    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
      state.width *= ZOOM_FACTOR;
      state.height *= ZOOM_FACTOR;
      
      Vector2 mouse_pos = GetMousePosition();
      real_t mouse_real = state.real_min + state.scalex * (mouse_pos.x + 0.5L);
      real_t mouse_imag = state.imag_min + state.scalex * ((real_t)(SCREEN_HEIGHT - mouse_pos.y - 1) + 0.5L);

      state.center_real = mouse_real;
      state.center_imag = mouse_imag;

      state.real_min = state.center_real - state.width * 0.5L;
      state.imag_min = state.center_imag - state.height * 0.5L;

      state.scalex = state.width / (real_t)SCREEN_WIDTH;
      state.scaley = state.height / (real_t)SCREEN_HEIGHT;

      view_changed = true;
    }

    if (view_changed && !atomic_load(&state.dirty)) {
      atomic_store(&state.dirty, true);
    }

    if (atomic_load(&state.ready)) {
      mtx_lock(&state.swap_lock);
      UpdateTexture(texture, state.front);
      mtx_unlock(&state.swap_lock);
      atomic_store(&state.ready, false);
    }
    
    BeginDrawing();
    ClearBackground(BLACK);
    DrawTexture(texture, 0, 0, WHITE);
    EndDrawing();
  }

  state.quit = true;
  thrd_join(thr, NULL);
  CloseWindow();
  return 0;
}

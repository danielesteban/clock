#include "led-matrix.h"
#include "transformer.h"
#include "graphics.h"

#include <unistd.h>
#include <math.h>
#include <stdio.h>
#include <signal.h>
#include <time.h>

#include "HSVtoRGB.hpp"

using namespace rgb_matrix;

volatile bool interrupt_received = false;
static void InterruptHandler(int signo) {
  interrupt_received = true;
}

static void draw(RGBMatrix *matrix, Font *font) {
  float fR, fG, fB, fH = 0, fS = 1.0, fV = 0.5;
  int width = matrix->width();
  int height = matrix->height();
  int centerX = width / 2;
  int centerY = height / 2;
  FrameCanvas *canvas = matrix->CreateFrameCanvas();
  clock_t lastTicks = clock();
  unsigned int acc = 0;
  while(1) {
    if (interrupt_received) return;
    clock_t ticks = clock();
    int delta = ticks - lastTicks;
    lastTicks = ticks;
    time_t rawtime;
    time(&rawtime);
    struct tm * timeinfo = localtime (&rawtime);
    char text[9];
    sprintf(text, "%02d:%02d:%02d", timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec);
    canvas->Clear();
    float angle = ((float) acc / 10000.0) * (M_PI / 180.0);
    acc += delta;
    int x = (float) centerX + (cos(angle) * 8.0);
    int y = (float) centerY + (sin(angle) * 4.0);
    for (float radius = 1; radius <= centerX + 8; radius += 1.0) {
      float rH = fH + ((centerX + 8 - radius) * 3.0);
      while (rH > 360.0) rH -= 360.0;
      HSVtoRGB(fR, fG, fB, rH, fS, fV);
      Color color = Color(fR * 255, fG * 255, fB * 255);
      DrawLine(canvas, x - radius, y + radius - 1, x + radius - 1, y + radius - 1, color);
      DrawLine(canvas, x - radius, y - radius, x + radius - 1, y - radius, color);
      DrawLine(canvas, x - radius, y + radius - 1, x - radius, y - radius, color);
      DrawLine(canvas, x + radius - 1, y + radius - 1, x + radius - 1, y - radius, color);
    }
    for (float radius = 1; radius <= centerX + 8; radius += 1.0) {
     float rH = fH + ((centerX + 8 - radius) * 3.0);
     while (rH > 360.0) rH -= 360.0;
     HSVtoRGB(fR, fG, fB, rH, fS, fV);
     DrawCircle(canvas, x, y, radius, Color(fR * 255, fG * 255, fB * 255));
    }
    if((fH += 1.0) > 360.0) fH = 0;

    DrawText(canvas, *font, 4, font->baseline() + 9, Color(0, 0, 0), text);

    canvas = matrix->SwapOnVSync(canvas);
    // usleep(std::max(0, 33333 - (int) (clock() - ticks)));
  }
}

int main(int argc, char *argv[]) {
  RGBMatrix::Options defaults;
  defaults.hardware_mapping = "regular-pi1";
  defaults.rows = 32;
  defaults.chain_length = 2;
  defaults.brightness = 70;
  // defaults.pwm_lsb_nanoseconds = 600;
  RuntimeOptions runtime;
  runtime.gpio_slowdown = 0;
  RGBMatrix *matrix = CreateMatrixFromFlags(&argc, &argv, &defaults, &runtime);
  if (matrix == NULL) return 1;
  matrix->ApplyStaticTransformer(RotateTransformer(180));

  Font font;
  if (!font.LoadFont("matrix/fonts/7x14B.bdf")) {
    fprintf(stderr, "Couldn't load font\n");
    return 1;
  }

  signal(SIGTERM, InterruptHandler);
  signal(SIGINT, InterruptHandler);

  srand(time(NULL));
  draw(matrix, &font);
  matrix->Clear();
  delete matrix;

  return 0;
}

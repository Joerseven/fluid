#include "raylib.h"
#include "solver.h"
#include "raymath.h"
#include <algorithm>
#include <array>

constexpr int N = 100;
constexpr int fl_array_size = (N + 2) * (N + 2);

typedef std::unique_ptr<std::array<float, fl_array_size>> float_array_ptr;

constexpr int fl_index(int x, int y) {
  return ((x) + (N + 2) * (y));
}

Color floatToColor(float number) {
  if (number > 1) number = 1;
  return { (unsigned char)(0 * 255), (unsigned char)(0 * 255), (unsigned char)(number * 255), (unsigned char)255};
}

std::ostream& operator<<(std::ostream& stream, const Color& color) {
  stream << (int)color.r << ", " << (int)color.g << ", " << (int)color.b << ", " << (int)color.a;
  return stream;
}

void add_liquid(int x, int y, float* density) {
  for (int i=0; i<fl_array_size; i++) {
    Vector2 position = { (float)(i % N), (float)(i / N) };
    if (Vector2Distance(position, {(float)x,(float)y}) < 10) {
      density[i] += 0.1f;
    }
  }
}

void add_source(const float_array_ptr& dest, const float_array_ptr& src, float dt) {
  for (int i = 0; i < fl_array_size; i++) {
    (*dest)[i] += dt * (*src)[i];
  }
}

void set_boundary(int width, int b, const float_array_ptr& data) {
  for (int i = 1; i <= width; i++) {
    (*data)[fl_index(0, i)] = b == 1 ? -(*data)[fl_index(1, i)] : (*data)[fl_index(1, i)];
    (*data)[fl_index(width + 1, i)] = b == 1 ? -(*data)[fl_index(width, i)] : (*data)[fl_index(width, i)];
    (*data)[fl_index(i, 0)] = b == 2 ? -(*data)[fl_index(i, 1)] : (*data)[fl_index(i, 1)];
    (*data)[fl_index(i, width + 1)] = b == 2 ? -(*data)[fl_index(i, width)] : (*data)[fl_index(i, width)];
  }
  (*data)[fl_index(0, 0)] = 0.5f * ((*data)[fl_index(1, 0)] + (*data)[fl_index(0, 1)]);
  (*data)[fl_index(0, width + 1)] = 0.5f * ((*data)[fl_index(1, width + 1)] + (*data)[fl_index(0, width)]);
  (*data)[fl_index(width + 1, 0)] = 0.5f * ((*data)[fl_index(width, 0)] + (*data)[fl_index(width + 1, 1)]);
  (*data)[fl_index(width + 1, width + 1)] = 0.5f * ((*data)[fl_index(1, 0)] + (*data)[fl_index(0, 1)]);
}

void diffuse(int b, const float_array_ptr& dest, const float_array_ptr& src, float diff, float dt) {

  float a = dt * diff * N * N;

  for (int k = 0;k < 20;k++) {
    for (int i = 1;i <= N;i++) {
      for (int j = 1; j <= N; j++) {
        (*dest)[fl_index(i, j)] = ((*src)[fl_index(i, j)] + a * ((*dest)[fl_index(i - 1, j)] + (*dest)[fl_index(i + 1, j)] + (*dest)[fl_index(i, j - 1)] + (*dest)[fl_index(i, j + 1)])) / (1 + 4 * a);
      }
    }
    set_boundary(N, b, dest);
  }
}

void advection(int b, const float_array_ptr& den, const float_array_ptr& prev_den, const float_array_ptr& u, const float_array_ptr& v, float dt) {
  int i0, j0, i1, j1;
  float s0, t0, s1, t1, dt0;
  dt0 = dt * N;
  for (int i = 1; i <= N; i++) {
    for (int j = 1; j <= N; j++) {
      float x = i - dt0 * (*u)[fl_index(i, j)];
      float y = j - dt0 * (*v)[fl_index(i, j)];
      if (x < 0.5) x = 0.5; 
      if (x > N + 0.5) x = N + 0.5;
      i0 = (int)x;
      i1 = i0 + 1;
      if (y < 0.5) y = 0.5;
      if (y > N + 0.5) y = N + 0.5;
      j0 = (int)y;
      j1 = j0 + 1;
      s1 = x - i0;
      s0 = 1 - s1;
      t1 = y - j0;
      t0 = 1 - t1;
      (*den)[fl_index(i, j)] = s0 * (t0 * (*prev_den)[fl_index(i0, j0)] + t1 * (*prev_den)[fl_index(i0, j1)]) + s1 * (t0 * (*prev_den)[fl_index(i1, j0)] + t1 * (*prev_den)[fl_index(i1, j1)]);
    }
  }
  set_boundary(N, b, den);
}

void project(const float_array_ptr& u, const float_array_ptr& v, const float_array_ptr& p, const float_array_ptr& div) {
  float h = 1.0 / N;
  for (int i = 1; i <= N; i++) {
    for (int j = 1; j <= N;j++) {
      (*div)[fl_index(i, j)] = -0.5 * h * ((*u)[fl_index(i + 1, j)] - (*u)[fl_index(i - 1, j)] + (*v)[fl_index(i, j + 1)] - (*v)[fl_index(i, j - 1)]);
      (*p)[fl_index(i, j)] = 0;
    }
  }
  set_boundary(N, 0, div);
  set_boundary(N, 0, p);

  for (int k = 0; k < 20; k++) {
    for (int i = 1; i <= N;i++) {
      for (int j = 1; j <= N;j++) {
        (*p)[fl_index(i, j)] = ((*div)[fl_index(i, j)] + (*p)[fl_index(i - 1, j)] + (*p)[fl_index(i + 1, j)] + (*p)[fl_index(i, j - 1)] + (*p)[fl_index(i, j + 1)]) / 4.0;
      }
    }
    set_boundary(N, 0, p);
  }

  for (int i = 1; i <= N; i++) {
    for (int j = 1;j <= N;j++) {
      (*u)[fl_index(i, j)] -= 0.5 * ((*p)[fl_index(i + 1, j)] - (*p)[fl_index(i - 1, j)]) / h;
      (*v)[fl_index(i, j)] -= 0.5 * ((*p)[fl_index(i, j + 1)] - (*p)[fl_index(i, j - 1)]) / h;
    }
  }

  set_boundary(N, 1, u);
  set_boundary(N, 2, v);
}

void density_step(const float_array_ptr& dest, const float_array_ptr& src, const float_array_ptr& u, const float_array_ptr& v, float diff, float dt) {
  add_source(dest, src, dt);
  std::swap(*dest, *src);
  diffuse(0, dest, src, diff, dt);
  std::swap(*dest, *src);
  advection(0, dest, src, u, v, dt);
}

void velocity_step(const float_array_ptr& u, const float_array_ptr& v, const float_array_ptr& prev_u, const float_array_ptr& prev_v, float visc, float dt) {
  add_source(u, prev_u, dt);
  add_source(v, prev_v, dt);
  std::swap(*prev_u, *u);
  diffuse(1, u, prev_u, visc, dt);
  std::swap(*prev_v, *v);
  diffuse(2, v, prev_v, visc, dt);
  project(u, v, prev_u, prev_v);
  std::swap(*prev_u, *u);
  std::swap(*prev_v, *v);
  advection(1, u, prev_u, prev_u, prev_v, dt);
  advection(2, v, prev_v, prev_u, prev_v, dt);
  project(u, v, prev_u, prev_v);
}

void add_liquid_point(const float_array_ptr& dest, int x, int y, int radius) {
  for (int i = x - radius; i < x + radius; i++) {
    for (int j = y - radius; j < y + radius; j++) {
      (*dest)[fl_index(i, j)] += 1000.0f;
    }
  }
}

int main (int argc, char *argv[]) {

  float_array_ptr vely = std::make_unique<std::array<float, fl_array_size>>();
  float_array_ptr velx = std::make_unique<std::array<float, fl_array_size>>();
  float_array_ptr prev_velx = std::make_unique<std::array<float, fl_array_size>>();
  float_array_ptr prev_vely = std::make_unique < std::array<float, fl_array_size>>();
  float_array_ptr density = std::make_unique<std::array<float, fl_array_size>>();
  float_array_ptr prev_density = std::make_unique<std::array<float, fl_array_size>>();
  float_array_ptr contents = std::make_unique<std::array<float, fl_array_size>>();
  contents->fill(0.0);
  density->fill(0.0);
  prev_velx->fill(0.0);
  prev_vely->fill(0.0);

  auto s = Solver();

  InitWindow(N+2, N+2, "Fluid Window!");
  SetTargetFPS(60);

  while (!WindowShouldClose()) {

    float dt = GetFrameTime();

    //if (IsMouseButtonDown(MOUSE_LEFT_BUTTON)) {
      //add_liquid(GetMouseX(), GetMouseY(), density->data());
    //}
    if (IsMouseButtonDown(MOUSE_LEFT_BUTTON)) {
      add_liquid_point(prev_density, 50, 50, 1);
    }

    if (IsMouseButtonDown(MOUSE_RIGHT_BUTTON)) {
      add_liquid_point(prev_velx, 50, 50, 1);
    }
    
    velocity_step(velx, vely, prev_velx, prev_vely, 0, dt);
    density_step(density, prev_density, velx, vely, 0.001f, dt);

    prev_velx->fill(0.0);
    prev_vely->fill(0.0);
    prev_density->fill(0.0);

    BeginDrawing();
    ClearBackground(RAYWHITE);

    for (int k = 0; k < fl_array_size; k++) {
      int x = k / (N + 2);
      int y = k % (N + 2);
      if (x != 0 && x != N + 1 && y != 0 && y != N + 1) {
        float color = (*density)[fl_index(x, y)];
        DrawPixel(x, y, floatToColor(color));
      }
      else {
        DrawPixel(x, y, GREEN);
      }
    }

    EndDrawing();
  }

  CloseWindow();
  return 0;
}

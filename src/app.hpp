#include <vector>
#include <cstdint>
#include <climits>
#include <algorithm>
#include <cmath>
#include <SDL2/SDL.h>

struct Rgb {
    float rgb[3];

    Rgb() {
        rgb[0] = 0.0;
        rgb[1] = 0.0;
        rgb[2] = 0.0;
    }

    Rgb(float r, float g, float b) {
      rgb[0] = r;
      rgb[1] = g;
      rgb[2] = b;
    }

    float r() {
        return rgb[0];
    }
    float g() {
        return rgb[1];
    }
    float b() {
        return rgb[2];
    }
};

struct Point {
    float x, y;

    static bool cmp_yx(Point &a, Point &b) {
        return (a.y < b.y || (a.y == b.y && a.x < b.x));
    }
};

struct Triangle {
    Point points[3];
    Rgb colors[3];

    Triangle(Point p0, Point p1, Point p2, Rgb c0, Rgb c1, Rgb c2) {
        points[0] = p0;
        points[1] = p1;
        points[2] = p2;
        colors[0] = c0;
        colors[1] = c1;
        colors[2] = c2;
    }

    SDL_Rect bounding_box() {
        float x0 = points[0].x;
        float x1 = points[1].x;
        float x2 = points[2].x;
        float y0 = points[0].y;
        float y1 = points[1].y;
        float y2 = points[2].y;

        if (x0 > x1) std::swap(x0, x1);
        if (x0 > x2) std::swap(x0, x2);
        if (x1 > x2) std::swap(x1, x2);
        if (y0 > y1) std::swap(y0, y1);
        if (y0 > y2) std::swap(y0, y2);
        if (y1 > y2) std::swap(y1, y2);

        return { x0, y0, x2-x0, y2-y0 };
    }
};

enum AppState {
    AppState_CreatePoint,
    AppState_Edit,
};

const Rgb COLOR_CYCLE[] = {
    {1.0, 0.0, 0.0},
    {0.0, 1.0, 0.0},
    {0.0, 0.0, 1.0},
    {1.0, 1.0, 0.0},
    {0.0, 1.0, 1.0},
    {1.0, 0.0, 1.0},
};
#define COLOR_CYCLE_SIZE (sizeof(COLOR_CYCLE) / sizeof(COLOR_CYCLE[0]))


struct App {
    AppState state;
    std::vector<Triangle> triangles;
    size_t selected;
    Point buffered_points[2];
    int num_buffered_points;
    int color_cycle;
    Rgb edge_color;
    SDL_Texture *filled;
    bool is_filled;

    App(SDL_Texture *texture) {
        state = AppState_CreatePoint;
        selected = SIZE_MAX;
        num_buffered_points = 0;
        color_cycle = 0;
        edge_color = Rgb(0.25, 0.25, 0.25);
        filled = texture;
        is_filled = false;
    }

    // TODO: there's still some detail
    size_t get_tri_by_point(Point point) {
        #define DIST_THRESH 30
        float min_dist = INFINITY;
        size_t min_dist_index = SIZE_MAX;
        for (size_t i = 0; i < triangles.size(); i++) {
            for (size_t j = 0; j < 3; j++) {
                Point p0 = triangles[i].points[j];
                Point p1 = triangles[i].points[(j+1) % 3];

                if ((p0.x <= point.x && point.x <= p1.x) || (p0.x <= point.x && point.x <= p1.x)
                ||  (p0.y <= point.y && point.y <= p1.y) || (p0.y <= point.y && point.y <= p1.y))
                {
                    float span = sqrtf((p0.x - p1.x)*(p0.x - p1.x) + (p0.y - p1.y)*(p0.y - p1.y));
                    float abx = fabs((p0.x - p1.x)*(p0.y - point.y) - (p0.y - p1.y)*(p0.x - point.x));
                    float dist = abx / span;
                    if (dist < DIST_THRESH && dist < min_dist) {
                        min_dist = dist;
                        min_dist_index = i;
                    }
                }
            }
        }
        return min_dist_index;
    }

    void on_click(int x, int y) {
        Point point = { x, y };
        switch (state) {
        case AppState_CreatePoint:
            if (num_buffered_points == 2) {
                Point p0 = buffered_points[0];
                Point p1 = buffered_points[1];
                Point p2 = point;

                Rgb c0 = COLOR_CYCLE[color_cycle++];
                if (color_cycle == COLOR_CYCLE_SIZE) color_cycle = 0;
                Rgb c1 = COLOR_CYCLE[color_cycle++];
                if (color_cycle == COLOR_CYCLE_SIZE) color_cycle = 0;
                Rgb c2 = COLOR_CYCLE[color_cycle++];
                if (color_cycle == COLOR_CYCLE_SIZE) color_cycle = 0;

                selected = triangles.size();
                triangles.push_back(Triangle(p0, p1, p2, c0, c1, c2));
                num_buffered_points = 0;
            } else {
                buffered_points[num_buffered_points++] = point;
            }
            break;
        case AppState_Edit:
            size_t search = get_tri_by_point(point);
            if (search != SIZE_MAX) {
                selected = search;
            }
            break;
        }
    }

    void draw(SDL_Renderer *renderer, int mousex, int mousey) {
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);

        SDL_SetRenderDrawColor(renderer, 50, 0, 0, 255);
        if (selected != SIZE_MAX) {
            SDL_Rect rect = triangles[selected].bounding_box();
            SDL_RenderDrawRect(renderer, &rect);
        }

        SDL_SetRenderDrawColor(renderer, edge_color.r()*255,
            edge_color.g()*255, edge_color.b()*255, 255);
        for (size_t i = 0; i < triangles.size(); i++) {
            for (int j = 0; j < 3; j++) {
                Point p0 = triangles[i].points[j];
                Point p1 = triangles[i].points[j==2 ? 0 : j+1];
                SDL_RenderDrawLine(renderer, p0.x, p0.y, p1.x, p1.y);
            }
        }
        
        if (is_filled) {
            SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
            SDL_RenderCopy(renderer, filled, NULL, NULL);
        }

        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
        if (num_buffered_points == 1) {
            float x0 = buffered_points[0].x;
            float y0 = buffered_points[0].y;
            SDL_RenderDrawLine(renderer, x0, y0, mousex, mousey);
        } else if (num_buffered_points == 2) {
            float x0 = buffered_points[0].x;
            float y0 = buffered_points[0].y;
            float x1 = buffered_points[1].x;
            float y1 = buffered_points[1].y;
            SDL_RenderDrawLine(renderer, x0, y0, x1, y1);
            SDL_RenderDrawLine(renderer, x0, y0, mousex, mousey);
            SDL_RenderDrawLine(renderer, x1, y1, mousex, mousey);
        }
    }
};


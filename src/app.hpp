#include <SDL2/SDL_blendmode.h>
#include <SDL2/SDL_render.h>
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

        return { int(x0), int(y0), int(x2-x0), int(y2-y0) };
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
    size_t selected, which_filled;
    Point buffered_points[2];
    int num_buffered_points;
    int color_cycle;
    Rgb edge_color;
    SDL_Texture *filled;
    bool is_filled;
    std::vector<std::pair<Point, Rgb>> intersections;

    // std::vector<std::pair<Point, Point>> edge_dists;

    App(SDL_Texture *texture) {
        state = AppState_CreatePoint;
        selected = SIZE_MAX;
        num_buffered_points = 0;
        color_cycle = 0;
        edge_color = Rgb(0.25, 0.25, 0.25);
        filled = texture;
        is_filled = false;
    }

    size_t get_tri_by_point(Point p2) {
        // edge_dists.clear();
        #define DIST_THRESH 40.0f
        float min_dist = INFINITY;
        size_t min_dist_index = SIZE_MAX;
        for (size_t i = 0; i < triangles.size(); i++) {
            for (size_t j = 0; j < 3; j++) {
                Point p0 = triangles[i].points[j];
                Point p1 = triangles[i].points[(j+1) % 3];
                // reta do segmento P0 P1
                float a1 = p1.y - p0.y;
                float b1 = p0.x - p1.x;
                float c1 = a1*p0.x + b1*p0.y;
                // reta do segmento perpendicular que passa por `point`
                float a2 = p1.x - p0.x;
                float b2 = p1.y - p0.y;
                float c2 = a2*p2.x + b2*p2.y;
                // intersecao das retas
                float ix = (c1*b2 - b1*c2) / (a1*b2 - b1*a2);
                float iy = (a1*c2 - c1*a2) / (a1*b2 - b1*a2);
                // ignora se intersecao nao estiver no segmento
                auto [x0, x1] = std::minmax(p0.x, p1.x);
                auto [y0, y1] = std::minmax(p0.y, p1.y);
                if (!((x0 <= ix && ix <= x1) || (y0 <= iy && iy <= y1))) {
                    continue;
                }
                // ignora se distancia for muito grande
                float dist = sqrt((p2.x - ix)*(p2.x - ix) + (p2.y - iy)*(p2.y - iy));
                if (dist > DIST_THRESH) {
                    continue;
                }
                // edge_dists.push_back({p2, {ix, iy}});
                if (dist < min_dist) {
                    min_dist = dist;
                    min_dist_index = i;
                }
            }
        }
        return min_dist_index;
    }

    void fill_selected(SDL_Renderer *renderer) {
        if (selected == SIZE_MAX) {
            return;
        }
        is_filled = true;
        which_filled = selected;
        intersections.clear();
        for (size_t i = 0; i < 3; i++) {
            Point p0 = triangles[selected].points[i];
            Point p1 = triangles[selected].points[i == 2 ? 0 : i + 1];
            Rgb c0 = triangles[selected].colors[i];
            Rgb c1 = triangles[selected].colors[i == 2 ? 0 : i + 1];
            // ignora segmentos horizontais 
            if (p0.y == p1.y) {
                continue;
            }
            // trabalha de cima para baixo
            if (p0.y > p1.y) {
                std::swap(p0, p1);
                std::swap(c0, c1);
            }
            float dy = p1.y - p0.y;
            float dx = (p1.x - p0.x) / dy;
            float dr = (c1.r() - c0.r()) / dy;
            float dg = (c1.g() - c0.g()) / dy;
            float db = (c1.b() - c0.b()) / dy;
            float x = p0.x;
            float r = c0.r();
            float g = c0.g();
            float b = c0.b();
            for (float y = p0.y; y < p1.y; y += 1.0) {
                intersections.push_back({{x, y}, {r, g, b}});
                x += dx;
                r += dr;
                g += dg;
                b += db;
            }
        }
        auto compare = [](const std::pair<Point, Rgb>& a, const std::pair<Point, Rgb>& b) {
            return (a.first.y < b.first.y) || (a.first.y == b.first.y && a.first.x < b.first.x);
        };
        std::sort(intersections.begin(), intersections.end(), compare);

        SDL_SetRenderTarget(renderer, filled);
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
        SDL_SetTextureBlendMode(filled, SDL_BLENDMODE_NONE);
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
        SDL_RenderFillRect(renderer, NULL);
        for (size_t i = 0; i < intersections.size(); i += 2) {
            Point p0 = intersections[i].first;
            Point p1 = intersections[i + 1].first;
            Rgb c0 = intersections[i].second;
            Rgb c1 = intersections[i + 1].second;
            float dx = p1.x - p0.x;
            float dr = (p0.x == p1.x) ? 0.0 : (c1.r() - c0.r()) / dx;
            float dg = (p0.x == p1.x) ? 0.0 : (c1.g() - c0.g()) / dx;
            float db = (p0.x == p1.x) ? 0.0 : (c1.b() - c0.b()) / dx;
            float r = c0.r();
            float g = c0.g();
            float b = c0.b();

            int y = p0.y;
            for (int x = p0.x; x <= p1.x; x++) {
                SDL_SetRenderDrawColor(renderer, r*255, g*255, b*255, 255);
                SDL_RenderDrawPoint(renderer, x, y);
                r += dr;
                g += dg;
                b += db;
            }
        }
        SDL_SetRenderTarget(renderer, NULL);
    }

    void remove_selected() {
        if (selected != SIZE_MAX) {
            triangles.erase(triangles.begin() + selected);
            if (selected == which_filled) {
                which_filled = SIZE_MAX;
                is_filled = false;
            }
            selected = SIZE_MAX;
        }
    }

    void on_click(int x, int y) {
        Point point = { float(x), float(y) };
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
                selected = SIZE_MAX;
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
            SDL_SetTextureBlendMode(filled, SDL_BLENDMODE_BLEND);
            SDL_RenderCopy(renderer, filled, NULL, NULL);
        }

        SDL_SetRenderDrawColor(renderer, edge_color.r()*255, edge_color.b()*255, edge_color.g()*255, 255);
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

        SDL_SetRenderDrawColor(renderer, edge_color.r()*255, edge_color.b()*255, edge_color.g()*255, 255);
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
        // for (auto seg : edge_dists) {
        //     SDL_RenderDrawLine(renderer, seg.first.x, seg.first.y, seg.second.x, seg.second.y);
        // }
    }
};


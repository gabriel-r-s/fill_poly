#include <vector>
#include <cstdint>
#include <climits>
#include <algorithm>
#include <SDL2/SDL.h>

#define MIN_DIST 16
#define AUTOFILL_INTERVAL 30

enum AppState {
    AppState_CreatePoint,
    AppState_Edit,
    AppState_Dragging
};

struct ColorPoint {
    int16_t x, y;
    float color[3];

    float r() {
        return color[0];
    }
    float g() {
        return color[1];
    }
    float b() {
        return color[2];
    }

    static bool cmp_y_x(ColorPoint &a, ColorPoint &b) {
        return (a.y < b.y) || (a.y == b.y && a.x < b.x);
    }
};

#define COLOR_CYCLE_SIZE 6
const float COLOR_CYCLE[COLOR_CYCLE_SIZE][3] = {
    {1.0, 0.0, 0.0},
    {0.0, 1.0, 0.0},
    {0.0, 0.0, 1.0},
    {1.0, 1.0, 0.0},
    {0.0, 1.0, 1.0},
    {1.0, 0.0, 1.0},
};

struct App {
    std::vector<ColorPoint> points;
    size_t nearest_point; 
    size_t selected_point;
    size_t color_cycle;
    bool autofill;
    int editing;
    std::vector<ColorPoint> filled;
    std::vector<ColorPoint> intersections;
    AppState state;
    
    App() {
        nearest_point = -1;
        selected_point = 0;
        color_cycle = 0;
        autofill = false;
        editing = 0;
        filled = {};
        state = AppState_CreatePoint;
    }

    void clear() {
        color_cycle = 0;
        points.clear();
        filled.clear();
    }

    void must_refill() {
        filled.clear();
        editing = 10;
    }

    size_t find_nearest_point(int x, int y) {
        size_t nearest = -1;
        int min_dx = INT_MAX;
        int min_dy = INT_MAX;
        for (size_t i = 0; i < points.size(); i++) {
            int dx = abs(points[i].x - x);
            int dy = abs(points[i].y - y);
            if (dx < MIN_DIST && dy < MIN_DIST && dx < min_dx && dy < min_dy) {
                nearest = i;
                min_dx = dx;
                min_dy = dy;
            }
        }
        return nearest;
    }

    void on_click(int x, int y) {
        ColorPoint point;
        switch (state) {
        case AppState_CreatePoint:   
            point.x = x;
            point.y = y;
            std::copy(COLOR_CYCLE[color_cycle], COLOR_CYCLE[color_cycle]+3, point.color);
            points.push_back(point);
            color_cycle = (color_cycle + 1) % COLOR_CYCLE_SIZE;
            filled.clear();
            break;
        case AppState_Edit:
            nearest_point = find_nearest_point(x, y);
            if (nearest_point != -1) {
                selected_point = nearest_point;
                state = AppState_Dragging;
            }
            break;
        case AppState_Dragging:
            break;
        }
    }
    
    void on_move(int x, int y) {
        switch (state) {
        case AppState_CreatePoint:
            break;
        case AppState_Edit:
            nearest_point = find_nearest_point(x, y);
            break;
        case AppState_Dragging:
            must_refill();
            points[selected_point].x = x;
            points[selected_point].y = y;
            break;
        }
    }

    void on_release(int x, int y) {
        switch (state) {
        case AppState_CreatePoint:
            break;
        case AppState_Edit:
            break;
        case AppState_Dragging:
            state = AppState_Edit;
            break;
        }
    }

    void fill() {
        filled.clear();
        intersections.clear();
        for (size_t i = 0; i < points.size(); i++) {
            size_t j = i + 1;
            if (j == points.size()) {
                j = 0;
            }
            ColorPoint p0 = points[i];
            ColorPoint p1 = points[j];
            if (p0.y == p1.y) {
                continue;
            }
            if (p0.y > p1.y) {
                std::swap(p0, p1);
            }
            float dx = float(p1.x - p0.x) / float(p1.y - p0.y);
            float dr = float(p1.r() - p0.r()) / float(p1.y - p0.y);
            float dg = float(p1.g() - p0.g()) / float(p1.y - p0.y);
            float db = float(p1.b() - p0.b()) / float(p1.y - p0.y);
            float x = p0.x;
            float r = p0.r();
            float g = p0.g();
            float b = p0.b();
            for (int16_t y = p0.y; y < p1.y; y++) {
                intersections.push_back({ int16_t(x), y, {r, g, b} });
                x += dx;
                r += dr;
                g += dg;
                b += db;
            }
        }
        std::sort(intersections.begin(), intersections.end(), ColorPoint::cmp_y_x);
        for (size_t i = 0; i < intersections.size(); i += 2) {
            size_t j = i + 1;
            if (j == intersections.size()) {
                j = i;
            }
            ColorPoint p0 = intersections[i];
            ColorPoint p1 = intersections[j];
            float r = p0.r();
            float g = p0.g();
            float b = p0.b();
            float dr = 0.0;
            float dg = 0.0;
            float db = 0.0;
            if (p0.x != p1.x) {
                dr = (p1.r() - p0.r()) / (p1.x - p0.x);
                dg = (p1.g() - p0.g()) / (p1.x - p0.x);
                db = (p1.b() - p0.b()) / (p1.x - p0.x);
            }
            int16_t y = p0.y;
            for (int16_t x = p0.x; x <= p1.x; x++) {
                filled.push_back({x, y, {r, g, b}});
                r += dr;
                g += dg;
                b += db;
            }
        }
    }

    void draw(SDL_Renderer *renderer) {
        if (autofill && editing == 0 && state != AppState_Dragging) {
            fill();
        }
        editing -= (editing > 0);
        if (filled.empty()) {
            // draw edges only
            SDL_SetRenderDrawColor(renderer, 255, 255, 255, 24);
            for (size_t i = 0; i < points.size(); i++) {
                size_t j = i + 1;
                if (j == points.size()) {
                    j = 0;
                }
                SDL_RenderDrawLine(renderer, points[i].x, points[i].y, points[j].x, points[j].y);
            }
        } else {
            // draw filled polygon TODO: cache this in an SDL_Texture
            // this tends to get slow on large polygons
            for (ColorPoint point : filled) {
                SDL_SetRenderDrawColor(renderer, point.r()*255, point.g()*255, point.b()*255, 0);
                SDL_RenderDrawPoint(renderer, point.x, point.y);
            }
        }
        // draw a square around the nearest vertex
        if (state == AppState_Edit && nearest_point != -1) {
            ColorPoint point = points[nearest_point];
            SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
            SDL_SetRenderDrawColor(renderer, 255, 255, 255, 128);
            SDL_Rect square = {
                .x = point.x - MIN_DIST / 2,
                .y = point.y - MIN_DIST / 2,
                .w = MIN_DIST,
                .h = MIN_DIST,
            };
            SDL_RenderFillRect(renderer, &square);
            SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
        }
    }
};

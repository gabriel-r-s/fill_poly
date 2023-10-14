#include <vector>
#include <cstdint>
#include <climits>
#include <algorithm>

#define MIN_DIST 16
#define AUTOFILL_INTERVAL 30

enum AppMode {
    AppMode_CreatePoint,
    AppMode_Edit,
    AppMode_Dragging
};

struct ColorPoint {
    int x, y;
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
};

#define COLOR_CYCLE_SIZE 6
const float COLOR_CYCLE[COLOR_CYCLE_SIZE][3] = {
    {1.0, 0.0, 0.0},
    {0.0, 1.0, 0.0},
    {0.0, 0.0, 1.0},
    {1.0, 1.0, 0.0},
    {0.0, 1.0, 1.0},
    {1.0, 0.0, 1.0}, };
struct App {
    std::vector<ColorPoint> points;
    size_t nearest_point; 
    size_t selected_point;
    size_t color_cycle;
    bool autofill;
    std::vector<ColorPoint> filled;
    std::vector<ColorPoint> intersections;
    AppMode mode;
    
    App() {
        nearest_point = -1;
        selected_point = 0;
        color_cycle = 0;
        filled = {};
        autofill = false;
        mode = AppMode_CreatePoint;
    }

    void clear() {
        color_cycle = 0;
        points.clear();
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
        switch (mode) {
        case AppMode_CreatePoint:   
            point.x = x;
            point.y = y;
            std::copy(COLOR_CYCLE[color_cycle], COLOR_CYCLE[color_cycle]+3, point.color);
            points.push_back(point);
            color_cycle = (color_cycle + 1) % COLOR_CYCLE_SIZE;
            filled.clear();
            break;
        case AppMode_Edit:
            if (nearest_point != -1) {
                selected_point = nearest_point;
                mode = AppMode_Dragging;
            }
            break;
        case AppMode_Dragging:
            break;
        }
    }
    
    void on_move(int x, int y) {
        switch (mode) {
        case AppMode_CreatePoint:
            break;
        case AppMode_Edit:
            nearest_point = find_nearest_point(x, y);
            break;
        case AppMode_Dragging:
            filled.clear();
            if (selected_point != -1) {
                points[selected_point].x = x;
                points[selected_point].y = y;
            }
            break;
        }
    }

    void on_release(int x, int y) {
        switch (mode) {
        case AppMode_CreatePoint:
            break;
        case AppMode_Edit:
            break;
        case AppMode_Dragging:
            mode = AppMode_Edit;
            break;
        }
    }

    void fill() {
        filled.clear();
        intersections.clear();
        for (size_t i = 0; i < points.size(); i++) {
            size_t j = i + 1;
            if (j == points.size()) j = 0;
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
            for (int y = p0.y; y < p1.y; y++) {
                intersections.push_back({ int(x), y, {r, g, b} });
                x += dx;
                r += dr;
                g += dg;
                b += db;
            }
        }
        auto comp = [](const ColorPoint& a, const ColorPoint& b) {
            return (a.y < b.y || (a.y == b.y && a.x < b.x));
        };
        std::sort(intersections.begin(), intersections.end(), comp);
        for (size_t i = 0; i < intersections.size(); i += 2) {
            ColorPoint p0 = intersections[i];
            ColorPoint p1 = intersections[i + 1];
            float r = p0.r();
            float g = p0.g();
            float b = p0.b();
            float dr = 0.0, dg = 0.0, db = 0.0;
            if (p0.x != p1.x) {
                dr = (p1.r() - p0.r()) / (p1.x - p0.x);
                dg = (p1.g() - p0.g()) / (p1.x - p0.x);
                db = (p1.b() - p0.b()) / (p1.x - p0.x);
            }
            for (int x = p0.x; x <= p1.x; x++) {
                filled.push_back({x, p0.y, {r, g, b}});
                r += dr;
                g += dg;
                b += db;
            }
        }
    }

    void draw(SDL_Renderer *renderer) {
        if (autofill && filled.size() == 0 && mode != AppMode_Dragging) {
            fill();
        }
        if (filled.size() == 0) {
            SDL_SetRenderDrawColor(renderer, 255, 255, 255, 24);
            for (size_t i = 0; i < points.size(); i++) {
                size_t j = i + 1;
                if (j == points.size()) j = 0;
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
        if (mode == AppMode_Edit && nearest_point != -1) {
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

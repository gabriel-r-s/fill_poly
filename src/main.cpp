#include "sdlg.hpp"
#include "app.hpp"
#include <cstdio>
#include <vector>

#define WIDTH 1360
#define HEIGHT 768

int main(int, char**) {
    SDL_Window *window;
    SDL_Renderer *renderer;
    setup(&window, &renderer, WIDTH, HEIGHT);
    ImGuiIO& io = ImGui::GetIO();
    SDL_Event event;
    SDL_Texture *texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, WIDTH, HEIGHT);
    App app(texture);

    int mousex = 0, mousey = 0;
    for (;;) {
        while (SDL_PollEvent(&event)) {
            ImGui_ImplSDL2_ProcessEvent(&event);
                if (quit_event(event, window)) {
                cleanup(window, renderer);
                return 0;
            }
            switch (event.type) {
            case SDL_MOUSEBUTTONDOWN:
                if (io.WantCaptureMouse) break;
                app.on_click(event.button.x, event.button.y);
                break;
            case SDL_MOUSEMOTION:
                mousex = event.button.x;
                mousey = event.button.y;
                break;
            default:
                break;
            }
        }

        imgui_new_frame();
        ImGui::Begin("Menu");
        if (ImGui::Button("Quit")) {
            cleanup(window, renderer);
            return 0;
        }

        ImGui::Text("Edge Color:");
        ImGui::Text("\t");
        ImGui::SameLine();
        ImGui::ColorEdit3("##edge_color", app.edge_color.rgb, ImGuiColorEditFlags_NoInputs);
        ImGui::Text("Mode:");
        ImGui::Text("\t");
        ImGui::SameLine();
        if (ImGui::RadioButton("Create Points", app.state == AppState_CreatePoint)) {
            app.state = AppState_CreatePoint;
        }
        ImGui::SameLine();
        if (ImGui::RadioButton("Edit", app.state == AppState_Edit)) {
            app.state = AppState_Edit;
        }
        if (app.selected != SIZE_MAX) {
            Triangle &tri = app.triangles[app.selected];
            ImGui::Text("Selected Triangle: ");

            ImGui::Text("\t%4.0f %4.0f", tri.points[0].x, tri.points[0].y);
            ImGui::SameLine();
            ImGui::ColorEdit3("##color0", tri.colors[0].rgb, ImGuiColorEditFlags_NoInputs);

            ImGui::Text("\t%4.0f %4.0f", tri.points[1].x, tri.points[1].y);
            ImGui::SameLine();
            ImGui::ColorEdit3("##color1", tri.colors[1].rgb, ImGuiColorEditFlags_NoInputs);

            ImGui::Text("\t%4.0f %4.0f", tri.points[2].x, tri.points[2].y);
            ImGui::SameLine();
            ImGui::ColorEdit3("##color2", tri.colors[2].rgb, ImGuiColorEditFlags_NoInputs);

            ImGui::Text("\t");
            ImGui::SameLine();
            if (ImGui::Button("Fill Selected")) {
                // app.fill();
            }
        }
        ImGui::Text("dbginfo:");
        ImGui::Text("\tmse %4d %4d", mousex, mousey);
        ImGui::NewLine();
        for (Triangle tri : app.triangles) {
            ImGui::Text("\ttri %4.0f %4.0f", tri.points[0].x, tri.points[0].y);
            ImGui::Text("\ttri %4.0f %4.0f", tri.points[1].x, tri.points[1].y);
            ImGui::Text("\ttri %4.0f %4.0f", tri.points[2].x, tri.points[2].y);
            ImGui::NewLine();
        }
        for (int i = 0; i < app.num_buffered_points; i++) {
            ImGui::Text("\tbuf %4.0f %4.0f", app.buffered_points[i].x, app.buffered_points[i].y);
        }

        ImGui::End();
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
        SDL_RenderClear(renderer);
        app.draw(renderer, mousex, mousey);
        imgui_render();
        SDL_RenderPresent(renderer);
    }
    cleanup(window, renderer);
    return 0;
}


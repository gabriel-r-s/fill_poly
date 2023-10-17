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
    App app;

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
            case SDL_MOUSEBUTTONUP:
                if (io.WantCaptureMouse) break;
                app.on_release(event.button.x, event.button.y);
                break;
            case SDL_MOUSEMOTION:
                if (io.WantCaptureMouse) break;
                app.on_move(event.button.x, event.button.y);
                break;
            case SDL_KEYDOWN:
                if (event.key.keysym.scancode == SDL_SCANCODE_ESCAPE) {
                    cleanup(window, renderer);
                    return 0;
                }
                if (io.WantCaptureKeyboard) break;
                switch (event.key.keysym.scancode) {
                case SDL_SCANCODE_C:
                    app.state = AppState_CreatePoint;
                    break;
                case SDL_SCANCODE_E:
                    app.state = AppState_Edit;
                    break;
                case SDL_SCANCODE_F:
                    app.fill();
                    break;
                case SDL_SCANCODE_A:
                    app.autofill = !app.autofill;
                    break;
                case SDL_SCANCODE_R:
                    app.clear();
                    break;
                default:
                    break;
                }
                break;
            }
        }

        imgui_new_frame();
        ImGui::Begin("Menu");

        if (ImGui::Button("Fill (F)")) {
            app.fill();
        }
        ImGui::SameLine();
        if (ImGui::Button("Clear (R)")) {
            app.clear();
        }
        ImGui::SameLine();
        if (ImGui::Button("Quit (Esc)")) {
            cleanup(window, renderer);
            return 0;
        }
        ImGui::Checkbox("Autofill (A)", &app.autofill);

        ImGui::Text("Mode:");
        if (ImGui::RadioButton("Create Points (C)", app.state == AppState_CreatePoint)) {
            app.state = AppState_CreatePoint;
        }
        ImGui::SameLine();
        if (ImGui::RadioButton("Edit (E)", app.state == AppState_Edit || app.state == AppState_Dragging)) {
            app.state = AppState_Edit;
        }

        ImGui::Text("Points: ");
        int id_stack = 0;
        for (ColorPoint &point : app.points) {
            ImGui::PushID(id_stack++);
            int x = point.x;
            if (ImGui::InputInt("x", &x)) {
                point.x = x;
            }
            ImGui::SameLine();
            int y = point.y;
            if (ImGui::InputInt("y", &y)) {
                point.y = y;
            }
            if (ImGui::ColorEdit3("##Color", point.color, 0)) {
                app.filled.clear();
                app.color_changing = 1;
            }
            ImGui::PopID();
        }
        ImGui::End();

        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
        SDL_RenderClear(renderer);
        app.draw(renderer);
        imgui_render();
        SDL_RenderPresent(renderer);
    }
    cleanup(window, renderer);
    return 0;
}

#include "sdlg.h"
#include "app.h"
#include <cstdio>
#include <vector>

#define WIDTH 1360
#define HEIGHT 768

int main(int, char**) {
    SDL_Window *window;
    SDL_Renderer *renderer;
    setup(&window, &renderer, WIDTH, HEIGHT);
    ImGuiIO& io = ImGui::GetIO();
    
    App app;

    bool done = false;
    while (!done) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            ImGui_ImplSDL2_ProcessEvent(&event);
            done = quit_event(event, window);

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
                if (event.key.keysym.scancode == SDL_SCANCODE_ESCAPE) done = true;
                if (io.WantCaptureKeyboard) break;
                switch (event.key.keysym.scancode) {
                case SDL_SCANCODE_ESCAPE:
                    done = true;
                    break;
                case SDL_SCANCODE_C:
                    app.mode = AppMode_CreatePoint;
                    break;
                case SDL_SCANCODE_E:
                    app.mode = AppMode_Edit;
                    break;
                case SDL_SCANCODE_F:
                    app.fill();
                    break;
                case SDL_SCANCODE_A:
                    app.autofill = !app.autofill;
                    break;
                case SDL_SCANCODE_R:
                    app = App();
                    break;
                default:
                    break;
                }
                break;
            }
        }

        imgui_new_frame();
        {
            ImGui::Begin("Menu");

            ImGui::Text("Mode:");
            if (ImGui::RadioButton("Create Points (C)", app.mode == AppMode_CreatePoint)) {
                app.mode = AppMode_CreatePoint;
            }
            ImGui::SameLine();
            if (ImGui::RadioButton("Edit (E)", app.mode == AppMode_Edit || app.mode == AppMode_Dragging)) {
                app.mode = AppMode_Edit;
            }

            ImGui::Text("Points: ");
            // for (ColorPoint &point : app.points) {
            for (size_t i = 0; i < app.points.size(); i++) {
                ImGui::PushID(i);
                if (ImGui::ColorEdit3("##id", &app.points[i].color[0], 0)) {
                    app.filled.clear();
                }
                ImGui::PopID();
                ImGui::SameLine();
                ImGui::Text("(x:%d y:%d)", app.points[i].x, app.points[i].y);
            }
            if (ImGui::Button("Fill (F)")) {
                app.fill();
            }
            ImGui::SameLine();
            ImGui::Checkbox("Autofill (A)", &app.autofill);
            if (ImGui::Button("Clear (R)")) {
                app = App();
            }
            if (ImGui::Button("Quit (Esc)")) {
                done = true;
            }
            
            ImGui::End();
        }
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
        SDL_RenderClear(renderer);
        app.draw(renderer);
        imgui_render();
        SDL_RenderPresent(renderer);
    }
    cleanup(window, renderer);
    return 0;
}

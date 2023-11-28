#include "sdlg.hpp"
#include "app.hpp"
#include <cstdio>
#include <vector>


int main(int, char**) {
    SDL_Window *window;
    SDL_Renderer *renderer;
    setup(&window, &renderer);
    ImGuiIO& io = ImGui::GetIO();
    SDL_Event event;
    int width, height;
    SDL_GetRendererOutputSize(renderer, &width, &height);
    SDL_Texture *texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, width, height);
    App app(texture);


    int mousex = 0, mousey = 0;
    for (;;) {
        while (SDL_PollEvent(&event)) {
            ImGui_ImplSDL2_ProcessEvent(&event);
                if (quit_event(event, window)) {
                cleanup(window, renderer, texture);
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
            cleanup(window, renderer, texture);
            return 0;
        }

        ImGui::Checkbox("Draw Edges", &app.draw_edges);
        if (app.draw_edges) {
            ImGui::Text("Edge Color:");
            ImGui::Text("\t");
            ImGui::SameLine();
            ImGui::ColorEdit3("##edge_color", app.edge_color.rgb, ImGuiColorEditFlags_NoInputs);
        }
        ImGui::Text("Mode:");
        ImGui::Text("\t");
        ImGui::SameLine();
        if (ImGui::RadioButton("Create Points", app.state == AppState_CreatePoint)) {
            app.state = AppState_CreatePoint;
        }
        ImGui::SameLine();
        if (ImGui::RadioButton("Edit", app.state == AppState_Edit)) {
            app.state = AppState_Edit;
            app.num_buffered_points = 0;
        }
        if (app.selected != SIZE_MAX) {
            #define COLOR_EDIT_TIMEOUT 10
            Triangle &tri = app.triangles[app.selected];
            ImGui::Text("Selected Triangle: ");

            ImGui::Text("\t%4.0f %4.0f", tri.points[0].x, tri.points[0].y);
            ImGui::SameLine();
            if (ImGui::ColorEdit3("##color0", tri.colors[0].rgb, ImGuiColorEditFlags_NoInputs)) {
                app.must_redraw = COLOR_EDIT_TIMEOUT;
            }

            ImGui::Text("\t%4.0f %4.0f", tri.points[1].x, tri.points[1].y);
            ImGui::SameLine();
            if (ImGui::ColorEdit3("##color1", tri.colors[1].rgb, ImGuiColorEditFlags_NoInputs)) {
                app.must_redraw = COLOR_EDIT_TIMEOUT;
            }

            ImGui::Text("\t%4.0f %4.0f", tri.points[2].x, tri.points[2].y);
            ImGui::SameLine();
            if (ImGui::ColorEdit3("##color2", tri.colors[2].rgb, ImGuiColorEditFlags_NoInputs)) {
                app.must_redraw = COLOR_EDIT_TIMEOUT;
            }

            ImGui::Text("\t");
            ImGui::SameLine();
            if (ImGui::Button("Up") && app.selected+1 != app.triangles.size()) {
                std::swap(app.triangles[app.selected], app.triangles[app.selected+1]);
                app.selected++;
                app.must_redraw = 1;
            }
            ImGui::SameLine();
            if (ImGui::Button("Down") && app.selected != 0) {
                std::swap(app.triangles[app.selected-1], app.triangles[app.selected]);
                app.selected--;
                app.must_redraw = 1;
            }
            ImGui::SameLine();
            if (ImGui::Button("Delete")) {
                app.remove_selected();
            }
        }
        // ImGui::Text("dbginfo:");
        // ImGui::Text("\tmse %4d %4d", mousex, mousey);
        // ImGui::NewLine();
        // for (Triangle tri : app.triangles) {
        //     ImGui::Text("\ttri %4.0f %4.0f", tri.points[0].x, tri.points[0].y);
        //     ImGui::Text("\ttri %4.0f %4.0f", tri.points[1].x, tri.points[1].y);
        //     ImGui::Text("\ttri %4.0f %4.0f", tri.points[2].x, tri.points[2].y);
        //     ImGui::NewLine();
        // }
        // for (int i = 0; i < app.num_buffered_points; i++) {
        //     float x = app.buffered_points[i].x;
        //     float y = app.buffered_points[i].y;
        //     ImGui::Text("\tbuf %4.0f %4.0f", x, y);
        // }

        ImGui::End();
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
        SDL_RenderClear(renderer);
        app.draw(renderer, mousex, mousey);
        imgui_render();
        SDL_RenderPresent(renderer);
    }
    cleanup(window, renderer, texture);
    return 0;
}


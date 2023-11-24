# fill_poly
Implementation of a scan-line polygon filling algorithm, with color interpolation along the edges and scan-lines. Using SDL2 Software Rendering and dear imgui in C++. <br>

To compile and run:
- Have [`SDL2`](https://wiki.libsdl.org/SDL2/Installation) and [`xmake`](https://xmake.io)  installed
- Tested with SDL 2.26.3, behavior may vary on other versions (ex. 2.0.20 - current version in Ubuntu repos)
- Run: `xmake` and confirm dependency installations
- Run: `xmake r` to execute

How to use:
- Click anywhere to begin creating triangles
- Select `Edit` mode and click a triangle to delete or modify its colors
- Press `Quit` to exit

#include "application.h"
#include "asset/asset_manager.h"
#include "scene/game_scene.h"
#include <raylib.h>
#include <stdio.h>
#include <stdlib.h>

#if defined(PLATFORM_WEB)
#include <emscripten/emscripten.h>
#include <emscripten/html5.h>

#include <stdio.h>
EM_BOOL uievent_callback(int eventType, const EmscriptenUiEvent *e, void *userData) {
    SetWindowSize(e->windowInnerWidth, e->windowInnerHeight);
    return 0;
}
#endif

AssetManager *am;

void app_init() {
#if defined(PLATFORM_WEB)
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    emscripten_set_resize_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, 0, 1, uievent_callback);
#endif
    InitWindow(WIDTH, HEIGHT, "Web Mario");
    SetTargetFPS(60);

    am = malloc(sizeof(AssetManager));
    am_load_assets(am);

    game_scene_init(am->assets);
}

void app_run() {
#if defined(PLATFORM_WEB)
    emscripten_set_main_loop(game_scene_update, 0, 1);
#else
    while (!WindowShouldClose()) {
        game_scene_update();
    }
#endif
}

void app_destroy() {
    game_scene_destroy();
    am_destroy_assets(am);
    free(am);
    CloseWindow();
}

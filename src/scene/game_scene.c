#include "scene/game_scene.h"
#include "component/canimation.h"
#include "component/cbbox.h"
#include "component/ctexture.h"
#include "ds/linkedlist.h"
#include "entity/entity_manager.h"
#include "level/level_parser.h"
#include "physics/aabb_detection.h"
#include "raylib.h"
#include "scene/scene.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

void s_collision_detection();
void s_collision_resolution();
void s_camera_movement();
void s_player_movement();
void s_player_animation();
void s_integrate(float dt);
void s_input();
void s_reset_game();
void s_animation_update();
void s_render();
void s_render_grid();
void s_render_camera_trap();
void s_render_bbox();

Texture2D *textures;
GameScene *game_scene;
Entity    *player;
int        cell_size      = 128;
int        physics_debug  = 0;
int        box_trap_start = 700;
int        box_trap_range = 400;

void game_scene_update() {
    em_update(&game_scene->em);
    s_animation_update();
    s_input();
    s_player_movement();
    s_player_animation();
    s_integrate(GetFrameTime());
    s_camera_movement();
    s_collision_detection();

    BeginDrawing();
    ClearBackground((Color){30, 200, 240, 255});

    BeginMode2D(game_scene->camera);

    if (physics_debug) {
        s_render_grid();
        s_render_camera_trap();
        s_render_bbox();
    } else {
        s_render();
    }

    EndMode2D();
    DrawFPS(0, 0);
    EndDrawing();
}

GameScene *game_scene_init(Texture2D *t) {
    textures = t;

    game_scene             = malloc(sizeof(GameScene));
    game_scene->scene_type = GAME_SCENE;
    em_init(&game_scene->em);

    // camera setup
    game_scene->camera.offset   = (Vector2){0.0, 0.0};
    game_scene->camera.target   = (Vector2){0.0, 0.0};
    game_scene->camera.rotation = 0.0;
    game_scene->camera.zoom     = 1.0;

    // spawn level
    level_parser("gamedata/level.data", &game_scene->em, &cell_size);

    // spawn player
    player                               = em_create_entity(&game_scene->em);
    player->transform.position.x         = 300.0;
    player->transform.position.y         = 300.0;
    player->transform.velocity.x         = 0.0;
    player->transform.velocity.y         = 300.0;
    player->texture.id                   = 1;
    player->shape.half_box               = (Vector2){cell_size / 2.0, cell_size / 2.0};
    player->bbox.half_box                = (Vector2){cell_size / 2.5, cell_size / 2.5};
    player->bbox.enabled                 = 1;
    player->animation.animation_enabled  = 1;
    player->animation.animation_duration = 12;

    return game_scene;
}

void game_scene_destroy() {
    em_destroy(&game_scene->em);
}

void s_collision_detection() {

    // check out of screen
    float threshold = 0.0;
    if (player->transform.position.y < threshold) {
        player->transform.velocity.y *= -1.0;
    } else if (player->transform.position.y > HEIGHT) {
        s_reset_game();
    }

    // check player collision against very objects
    char collide_anything = 0;
    for (Node *i = game_scene->em.entities.head; i != 0; i = i->next) {
        Entity *e = (Entity *)i->data;
        if (e == 0)
            continue;

        if (e->is_alive == 0 || e->bbox.enabled == 0 || e == player)
            continue;

        Vector2 collider_pos      = e->transform.position;
        Vector2 collider_half_box = e->bbox.half_box;

        AABBResult *result          = &player->bbox.collision_result;
        Vector2     player_pos      = player->transform.position;
        Vector2     player_half_box = player->bbox.half_box;

        int collision = AABB_detection(player_pos, player_half_box, collider_pos, collider_half_box, &result->collision_axes, &result->overlapped_shape);
        if (collision == 1.0) {

            Vector2 player_prev_pos = player->transform.prev_position;
            AABB_detection(player_prev_pos, player_half_box, collider_pos, collider_half_box, &result->prev_collision_axes, NULL);

            if (result->prev_collision_axes.y == 0) {
                collide_anything = 1;
            }

            s_collision_resolution();
        } else {
            collide_anything &= 1;
        }
    }

    player->state.on_ground = collide_anything;
}

void s_collision_resolution() {
    if (!player)
        return;

    Vector2 collision_axes = player->bbox.collision_result.collision_axes;

    Vector2   *player_pos       = &player->transform.position;
    Vector2   *player_vec       = &player->transform.velocity;
    AABBResult collision_result = player->bbox.collision_result;

    Vector2 player_vec_dir;
    player_vec_dir.x = player_vec->x > 0.0 ? 1.0 : -1.0;
    player_vec_dir.y = player_vec->y > 0.0 ? 1.0 : -1.0;

    if (collision_axes.x * collision_axes.y == 1.0) {

        if (collision_result.prev_collision_axes.y == 1.0) {
            player_pos->x -= collision_result.overlapped_shape.x * player_vec_dir.x;
        } else if (collision_result.prev_collision_axes.x == 1.0) {
            player_pos->y -= collision_result.overlapped_shape.y * player_vec_dir.y;

            if (player_vec_dir.y == -1.0) {
                // player falling down when hitting their head
                player_vec->y *= -0.8;
            }
        }
    }
}

void s_camera_movement() {
    if (player == 0)
        return;

    float *x_pos = &player->transform.position.x;
    float  x_vec = player->transform.velocity.x;

    if (*x_pos < box_trap_start && x_vec <= 0) {
        box_trap_start = *x_pos;
    } else if (*x_pos > box_trap_start + box_trap_range && x_vec > 0) {
        box_trap_start = (int)(*x_pos - box_trap_range);
    }

    game_scene->camera.target.x = box_trap_start;
    game_scene->camera.offset.x = 700;
#ifdef PLATFORM_WEB
    game_scene->camera.offset.y = (GetRenderHeight() - HEIGHT) + 62;
#endif
}

void s_player_movement() {
    char     input = player->input.input;
    Vector2 *vec   = &player->transform.velocity;
    if (input & (1 << LEFT_KEY_BIT)) {
        vec->x = -350.0;
    } else if (input & (1 << RIGHT_KEY_BIT)) {
        vec->x = 350.0;
    } else {
        vec->x = 0.0;
    }

    if (input & (1 << UP_KEY_BIT) && player->state.on_ground == 1) {
        vec->y = -1500.0;
    } else {
        vec->y += 2500.0 * GetFrameTime();
        vec->y = fmin(1000.0, vec->y);
    }
}

void s_player_animation() {
    char input = player->input.input;
    if (input & (1 << LEFT_KEY_BIT)) {
        player->texture.id = 1;
        player->state.dir  = -1;
    } else if (input & (1 << RIGHT_KEY_BIT)) {
        player->texture.id = 1;
        player->state.dir  = 1;
    } else {
        player->texture.id = 0;
    }

    if (player->state.on_ground == 0) {
        player->texture.id = 2;
    }
}

void s_integrate(float dt) {
    for (Node *i = game_scene->em.entities.head; i != 0; i = i->next) {
        Entity *e = (Entity *)i->data;
        if (e == 0)
            continue;

        if (e->is_alive == 0)
            continue;

        Vector2  vec = e->transform.velocity;
        Vector2 *pos = &e->transform.position;

        e->transform.prev_position = *pos;

        pos->x += vec.x * dt;
        pos->y += vec.y * dt;
    }
}

void s_input() {
    if (IsKeyPressed(KEY_ONE)) {
        physics_debug ^= 1;
    }
    if (IsKeyPressed(KEY_TWO)) {
        s_reset_game();
    }

    char mouse_movement_right = 0;
    char mouse_movement_left  = 0;
    if (IsMouseButtonDown(MOUSE_LEFT_BUTTON)) {
        mouse_movement_right = GetMouseX() >= GetRenderWidth() * 2 / 3;
        mouse_movement_left  = GetMouseX() < GetRenderWidth() / 3;
    }

    if (IsKeyDown(KEY_A) || IsKeyDown(KEY_LEFT) || mouse_movement_left) {
        player->input.input |= 1 << LEFT_KEY_BIT;
    } else {
        player->input.input &= ~(1 << LEFT_KEY_BIT);
    }
    if (IsKeyDown(KEY_D) || IsKeyDown(KEY_RIGHT) || mouse_movement_right) {
        player->input.input |= 1 << RIGHT_KEY_BIT;
    } else {
        player->input.input &= ~(1 << RIGHT_KEY_BIT);
    }
    if (IsKeyPressed(KEY_W) || IsKeyPressed(KEY_UP) || IsGestureDetected(GESTURE_SWIPE_UP)) {
        player->input.input |= (1 << UP_KEY_BIT);
    } else {
        player->input.input &= ~(1 << UP_KEY_BIT);
    }
}

void s_reset_game() {
    box_trap_start              = 700;
    game_scene->camera.target.x = box_trap_start;
    player->transform.position  = (Vector2){300.0, 300.0};
}

void s_animation_update() {
    for (Node *i = game_scene->em.entities.head; i != 0; i = i->next) {
        Entity *e = (Entity *)i->data;
        if (e == 0)
            continue;
        if (e->is_alive == 0 || e->animation.animation_enabled == 0)
            continue;

        CAnimation *animation = &e->animation;
        animation->animation_live_frame += 1;
        animation->current_animation_frame = animation->animation_live_frame / animation->animation_duration;
    }
}

void s_render() {
    for (Node *i = game_scene->em.entities.head; i != 0; i = i->next) {
        Entity *e = (Entity *)i->data;
        if (e == 0)
            continue;

        if (e->is_alive == 0)
            continue;

        Vector2 pos          = e->transform.position;
        int     texture_id   = e->texture.id;
        float   shape_size_w = e->shape.half_box.x;
        float   shape_size_h = e->shape.half_box.y;

        if (texture_id == NO_TEXTURE) {
            DrawCircle(pos.x, pos.y, shape_size_w, RED);
        } else {
            Rectangle source;
            source.x      = 0.0;
            source.y      = 0.0;
            source.width  = textures[texture_id].animation_width * e->state.dir;
            source.height = textures[texture_id].animation_height;

            if (e->animation.animation_enabled)
                source.x = e->animation.current_animation_frame % textures[texture_id].animation_frame_count_x * textures[texture_id].animation_width;

            Rectangle dest;
            dest.x      = pos.x;
            dest.y      = pos.y;
            dest.width  = shape_size_w * 2.0;
            dest.height = shape_size_h * 2.0;
            DrawTexturePro(textures[texture_id], source, dest, e->shape.half_box, 0.0, WHITE);
        }
    }
}

void s_render_grid() {
    int count_x = WIDTH / cell_size;
    int count_y = HEIGHT / cell_size;
    int y_offet = HEIGHT % cell_size;

    for (int i = 1; i < count_x; i += 1) {
        DrawLine(i * cell_size, 0, i * cell_size, HEIGHT, RED);
    }

    for (int i = 0; i <= count_y; i += 1) {
        DrawLine(0, i * cell_size - y_offet, WIDTH, i * cell_size - y_offet, RED);
    }
}

void s_render_camera_trap() {
    DrawLineEx((Vector2){box_trap_start, 0}, (Vector2){box_trap_start, HEIGHT}, 10, GREEN);
    DrawLineEx((Vector2){box_trap_start + box_trap_range, 0}, (Vector2){box_trap_start + box_trap_range, HEIGHT}, 10, GREEN);
}

void s_render_bbox() {
    for (Node *i = game_scene->em.entities.head; i != 0; i = i->next) {
        Entity *e = i->data;
        if (e == 0)
            continue;
        if (e->is_alive == 0 || e->bbox.enabled == 0)
            continue;

        Vector2 pos      = e->transform.position;
        Vector2 half_box = e->bbox.half_box;

        Rectangle rec;
        rec.x      = pos.x;
        rec.y      = pos.y;
        rec.width  = half_box.x * 2.0;
        rec.height = half_box.y * 2.0;

        DrawRectanglePro(rec, half_box, 0.0, RED);
        DrawCircle(pos.x, pos.y, 15.0, BLACK);
    }
}

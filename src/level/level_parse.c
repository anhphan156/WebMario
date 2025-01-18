#include "component/ctexture.h"
#include "entity/entity_manager.h"
#include "level/level_parser.h"
#include "raylib.h"
#include <stdio.h>
#include <stdlib.h>

void level_parser(const char *level_name, EntityManager *em, int *cell_size) {
    FILE *stream = fopen(level_name, "r");
    if (!stream) {
        perror("failed to open level file");
        return;
    }

    char   *line_buffer = 0;
    size_t  len         = 0;
    ssize_t read        = 0;

    read = getline(&line_buffer, &len, stream);
    if (read == -1) {
        perror("getline");
        exit(1);
    }
    *cell_size           = atoi(line_buffer);
    float half_cell_size = *cell_size / 2.0;
    int   y_offset       = HEIGHT % *cell_size * 2;

    int   texture_id = NO_TEXTURE, x = 0, y = 0, collision_enabled = 0, animation_enabled = 0, animation_duration = 24;
    float draw_w = 1.0, draw_h = 1.0, bb_w = 1.0, bb_h = 1.0;
    while ((read = getline(&line_buffer, &len, stream)) != -1) {
        sscanf(line_buffer, "%d %d %d %f %f %f %f %d %d %d", &texture_id, &x, &y, &draw_w, &draw_h, &bb_w, &bb_h, &collision_enabled, &animation_enabled, &animation_duration);

        Entity *e                       = em_create_entity(em);
        e->transform.position.x         = *cell_size * x + half_cell_size;
        e->transform.position.y         = HEIGHT - y * *cell_size - y_offset + half_cell_size;
        e->shape.half_box               = (Vector2){half_cell_size * draw_w, half_cell_size * draw_h};
        e->bbox.half_box                = (Vector2){half_cell_size * bb_w, half_cell_size * bb_h};
        e->bbox.enabled                 = collision_enabled;
        e->texture.id                   = texture_id;
        e->animation.animation_enabled  = animation_enabled;
        e->animation.animation_duration = animation_duration;
    }

    free(line_buffer);
    fclose(stream);
}

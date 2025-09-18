#include "so_long_runner.h"

void update_player(t_game *game, long current_time)
{
    if (current_time - game->last_auto_step >= AUTO_STEP_MS)
    {
        if (game->player.y > 0 && game->map[game->player.y - 1][game->player.x] != '1' &&
            game->player.y - 1 > game->wall_level)
        {
            game->player.y--;
            game->moves++;
            printf("Moves: %d (Auto-step)\n", game->moves);

            if (game->map[game->player.y][game->player.x] == 'C')
                handle_collectible(game, game->player.x, game->player.y);
            else if (game->map[game->player.y][game->player.x] == 'E' &&
                     game->collectibles_taken == game->collectibles_total)
            {
                game->victory = 1;
                printf("[WIN] You've reached the exit!\n");
            }
        }
        game->last_auto_step = current_time;
    }

    if (game->player.y <= game->wall_level)
    {
        game->game_over = 1;
        printf("[MORTO] The wall caught you!\n");
    }
}

void update_army(t_game *game)
{
    int display_count = (game->army_count > MAX_SOLDIERS_DISPLAY) ?
                        MAX_SOLDIERS_DISPLAY : game->army_count;

    float center_x = game->player.x * TILE_SIZE + 20;
    float center_y = game->player.y * TILE_SIZE + 20;

    for (int i = 0; i < display_count; i++)
    {
        double radius = 30 + (i / 10) * 15;
        double angle = (2.0 * M_PI * i) / display_count + (get_time_ms() / 1000.0);

        game->soldiers[i].target_x = center_x + cos(angle) * radius;
        game->soldiers[i].target_y = center_y + sin(angle) * radius;

        game->soldiers[i].x += (game->soldiers[i].target_x - game->soldiers[i].x) * 0.1;
        game->soldiers[i].y += (game->soldiers[i].target_y - game->soldiers[i].y) * 0.1;
    }
}

void update_projectiles(t_game *game)
{
    for (int i = 0; i < MAX_PROJECTILES; i++)
    {
        if (!game->projectiles[i].active)
            continue;

        game->projectiles[i].x += game->projectiles[i].dx;
        game->projectiles[i].y += game->projectiles[i].dy;

        if (game->projectiles[i].y < 0 || game->projectiles[i].y > WIN_HEIGHT ||
            game->projectiles[i].x < 0 || game->projectiles[i].x > WIN_WIDTH)
        {
            game->projectiles[i].active = 0;
        }
    }
}

void update_enemies(t_game *game)
{
    for (int i = 0; i < MAX_ENEMIES; i++)
    {
        if (!game->enemies[i].active)
            continue;

        t_enemy *enemy = &game->enemies[i];
        enemy->move_timer++;

        if (enemy->move_timer >= 10)
        {
            enemy->move_timer = 0;

            if (enemy->type == 0)
            {
                float new_x = enemy->x + enemy->dx * TILE_SIZE;
                int grid_x = (int)(new_x / TILE_SIZE);
                int grid_y = (int)(enemy->y / TILE_SIZE);

                if (grid_x <= 0 || grid_x >= game->map_width - 1 ||
                    game->map[grid_y][grid_x] == '1')
                    enemy->dx = -enemy->dx;
                else
                    enemy->x = new_x;
            }
            else if (enemy->type == 1)
            {
                int player_x = game->player.x * TILE_SIZE + 20;
                int player_y = game->player.y * TILE_SIZE + 20;

                if (abs(enemy->x - player_x) < 3 * TILE_SIZE &&
                    abs(enemy->y - player_y) < 3 * TILE_SIZE)
                {
                    if (enemy->x < player_x && enemy->x + TILE_SIZE < WIN_WIDTH)
                        enemy->x += 2;
                    else if (enemy->x > player_x && enemy->x > 0)
                        enemy->x -= 2;

                    if (enemy->y < player_y && enemy->y + TILE_SIZE < WIN_HEIGHT)
                        enemy->y += 2;
                    else if (enemy->y > player_y && enemy->y > 0)
                        enemy->y -= 2;
                }
            }
            else if (enemy->type == 2)
            {
                enemy->y += enemy->dy * 5;
                if (enemy->y > WIN_HEIGHT)
                    enemy->active = 0;
            }
        }
    }
}

void update_wall(t_game *game, long current_time)
{
    if (current_time - game->last_rise >= RISE_MS)
    {
        if (game->wall_level > 0)
        {
            for (int x = 0; x < game->map_width; x++)
                game->map[game->wall_level][x] = '1';

            game->wall_level--;
            printf("Wall rising! Level: %d\n", game->wall_level);
        }
        game->last_rise = current_time;
    }
}

void handle_collectible(t_game *game, int x, int y)
{
    int gate_index = 0;
    for (int cy = 0; cy < game->map_height; cy++)
    {
        for (int cx = 0; cx < game->map_width; cx++)
        {
            if (game->map[cy][cx] == 'C')
            {
                if (cx == x && cy == y)
                {
                    t_math_gate *gate = &game->math_gates[gate_index];
                    if (!gate->collected)
                    {
                        int old_army = game->army_count;

                        if (gate->operation == '+')
                            game->army_count += gate->value;
                        else if (gate->operation == '*')
                            game->army_count *= gate->value;
                        else if (gate->operation == '-')
                        {
                            game->army_count -= gate->value;
                            if (game->army_count < 0)
                                game->army_count = 0;
                        }

                        printf("Army: %d %c%d = %d\n", old_army, gate->operation,
                               gate->value, game->army_count);

                        gate->collected = 1;
                        game->map[y][x] = '0';
                        game->collectibles_taken++;

                        if (game->army_count <= 0)
                        {
                            game->game_over = 1;
                            printf("[MORTO] Your army was completely destroyed!\n");
                        }
                    }
                    return;
                }
                gate_index++;
            }
        }
    }
}

void shoot_projectiles(t_game *game)
{
    if (game->army_count <= 0)
        return;

    int shots = game->army_count / 10 + 1;
    if (shots > 7)
        shots = 7;

    for (int s = 0; s < shots; s++)
    {
        for (int i = 0; i < MAX_PROJECTILES; i++)
        {
            if (!game->projectiles[i].active)
            {
                game->projectiles[i].x = game->player.x * TILE_SIZE + 20;
                game->projectiles[i].y = game->player.y * TILE_SIZE + 20;

                float spread = (shots > 1) ? (s - shots/2.0) * 0.3 : 0;
                game->projectiles[i].dx = spread;
                game->projectiles[i].dy = -8;
                game->projectiles[i].active = 1;
                break;
            }
        }
    }
}

void check_collisions(t_game *game)
{
    for (int p = 0; p < MAX_PROJECTILES; p++)
    {
        if (!game->projectiles[p].active)
            continue;

        for (int e = 0; e < MAX_ENEMIES; e++)
        {
            if (!game->enemies[e].active)
                continue;

            float dx = game->projectiles[p].x - game->enemies[e].x;
            float dy = game->projectiles[p].y - game->enemies[e].y;
            float distance = sqrt(dx*dx + dy*dy);

            if (distance < 20)
            {
                game->enemies[e].hp--;
                game->projectiles[p].active = 0;

                if (game->enemies[e].hp <= 0)
                    game->enemies[e].active = 0;
                break;
            }
        }
    }

    for (int e = 0; e < MAX_ENEMIES; e++)
    {
        if (!game->enemies[e].active)
            continue;

        float dx = (game->player.x * TILE_SIZE + 20) - game->enemies[e].x;
        float dy = (game->player.y * TILE_SIZE + 20) - game->enemies[e].y;
        float distance = sqrt(dx*dx + dy*dy);

        if (distance < 25)
        {
            if (game->army_count > 0)
            {
                game->army_count -= 5;
                game->enemies[e].active = 0;
                printf("Enemy hit! Army reduced to: %d\n", game->army_count);

                if (game->army_count <= 0)
                {
                    game->game_over = 1;
                    printf("[MORTO] Your army was destroyed!\n");
                }
            }
            else
            {
                game->game_over = 1;
                printf("[MORTO] Enemy killed you!\n");
            }
        }
    }
}
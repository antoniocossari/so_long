#include "so_long_runner.h"

void put_pixel(void *mlx, void *win, int x, int y, int color)
{
    mlx_pixel_put(mlx, win, x, y, color);
}

void draw_rectangle(t_game *game, int x, int y, int width, int height, int color)
{
    for (int i = 0; i < width; i++)
    {
        for (int j = 0; j < height; j++)
        {
            if (x + i >= 0 && x + i < WIN_WIDTH && y + j >= 0 && y + j < WIN_HEIGHT)
                put_pixel(game->mlx, game->win, x + i, y + j, color);
        }
    }
}

void draw_circle(t_game *game, int cx, int cy, int radius, int color)
{
    for (int x = cx - radius; x <= cx + radius; x++)
    {
        for (int y = cy - radius; y <= cy + radius; y++)
        {
            int dx = x - cx;
            int dy = y - cy;
            if (dx*dx + dy*dy <= radius*radius)
            {
                if (x >= 0 && x < WIN_WIDTH && y >= 0 && y < WIN_HEIGHT)
                    put_pixel(game->mlx, game->win, x, y, color);
            }
        }
    }
}

void render_game(t_game *game)
{
    mlx_clear_window(game->mlx, game->win);

    render_map(game);
    render_enemies(game);
    render_projectiles(game);
    render_player(game);
    render_army(game);
    render_hud(game);

    if (game->game_over)
    {
        draw_rectangle(game, WIN_WIDTH/2 - 100, WIN_HEIGHT/2 - 20, 200, 40, 0xFF0000);
        mlx_string_put(game->mlx, game->win, WIN_WIDTH/2 - 60, WIN_HEIGHT/2, 0xFFFFFF, "[GAME OVER]");
    }
    else if (game->victory)
    {
        draw_rectangle(game, WIN_WIDTH/2 - 100, WIN_HEIGHT/2 - 20, 200, 40, 0x00FF00);
        mlx_string_put(game->mlx, game->win, WIN_WIDTH/2 - 40, WIN_HEIGHT/2, 0xFFFFFF, "[VICTORY!]");
    }
}

void render_map(t_game *game)
{
    for (int y = 0; y < game->map_height; y++)
    {
        for (int x = 0; x < game->map_width; x++)
        {
            int screen_x = x * TILE_SIZE;
            int screen_y = y * TILE_SIZE;

            if (game->map[y][x] == '1')
            {
                draw_rectangle(game, screen_x, screen_y, TILE_SIZE, TILE_SIZE, 0x808080);
            }
            else if (game->map[y][x] == 'C')
            {
                draw_rectangle(game, screen_x + 5, screen_y + 5, TILE_SIZE - 10, TILE_SIZE - 10, 0x00FFFF);

                int gate_index = 0;
                for (int cy = 0; cy <= y; cy++)
                {
                    for (int cx = 0; cx < game->map_width; cx++)
                    {
                        if (game->map[cy][cx] == 'C')
                        {
                            if (cx == x && cy == y)
                            {
                                t_math_gate *gate = &game->math_gates[gate_index];
                                char op_str[10];
                                sprintf(op_str, "%c%d", gate->operation, gate->value);
                                mlx_string_put(game->mlx, game->win, screen_x + 10, screen_y + 20, 0x000000, op_str);
                                goto next_tile;
                            }
                            gate_index++;
                        }
                    }
                }
                next_tile:;
            }
            else if (game->map[y][x] == 'E')
            {
                int color = (game->collectibles_taken == game->collectibles_total) ? 0x00FF00 : 0xFF6600;
                draw_rectangle(game, screen_x + 5, screen_y + 5, TILE_SIZE - 10, TILE_SIZE - 10, color);
                mlx_string_put(game->mlx, game->win, screen_x + 15, screen_y + 20, 0xFFFFFF, "E");
            }

            if (y <= game->wall_level)
            {
                draw_rectangle(game, screen_x, screen_y, TILE_SIZE, TILE_SIZE, 0x660000);
            }
        }
    }
}

void render_player(t_game *game)
{
    int x = game->player.x * TILE_SIZE + 20;
    int y = game->player.y * TILE_SIZE + 20;

    draw_circle(game, x, y, 15, 0x0066FF);
    draw_circle(game, x, y, 10, 0x00AAFF);
}

void render_army(t_game *game)
{
    int display_count = (game->army_count > MAX_SOLDIERS_DISPLAY) ?
                        MAX_SOLDIERS_DISPLAY : game->army_count;

    for (int i = 0; i < display_count; i++)
    {
        int x = (int)game->soldiers[i].x;
        int y = (int)game->soldiers[i].y;
        draw_circle(game, x, y, 3, 0x00FF00);
    }

    if (game->army_count > MAX_SOLDIERS_DISPLAY)
    {
        int extra = game->army_count - MAX_SOLDIERS_DISPLAY;
        char badge[20];
        sprintf(badge, "+%d", extra);
        mlx_string_put(game->mlx, game->win, game->player.x * TILE_SIZE + 40,
                       game->player.y * TILE_SIZE, 0xFFFF00, badge);
    }
}

void render_projectiles(t_game *game)
{
    for (int i = 0; i < MAX_PROJECTILES; i++)
    {
        if (game->projectiles[i].active)
        {
            int x = (int)game->projectiles[i].x;
            int y = (int)game->projectiles[i].y;
            draw_circle(game, x, y, 2, 0xFFFF00);
        }
    }
}

void render_enemies(t_game *game)
{
    for (int i = 0; i < MAX_ENEMIES; i++)
    {
        if (game->enemies[i].active)
        {
            int x = (int)game->enemies[i].x;
            int y = (int)game->enemies[i].y;

            int color;
            int size;

            if (game->enemies[i].type == 0)
            {
                color = 0xFF4444;
                size = 8;
            }
            else if (game->enemies[i].type == 1)
            {
                color = 0xFF8844;
                size = 10;
            }
            else
            {
                color = 0xFF0044;
                size = 12;
            }

            draw_circle(game, x, y, size, color);
            if (game->enemies[i].hp > 1)
            {
                draw_circle(game, x, y, size - 2, 0xFF6666);
            }
        }
    }
}

void render_hud(t_game *game)
{
    char army_str[50];
    sprintf(army_str, "Army: %d", game->army_count);
    mlx_string_put(game->mlx, game->win, 10, 20, 0xFFFFFF, army_str);

    char moves_str[50];
    sprintf(moves_str, "Moves: %d", game->moves);
    mlx_string_put(game->mlx, game->win, 10, 40, 0xFFFFFF, moves_str);

    char collectibles_str[50];
    sprintf(collectibles_str, "Gates: %d/%d", game->collectibles_taken, game->collectibles_total);
    mlx_string_put(game->mlx, game->win, 10, 60, 0xFFFFFF, collectibles_str);

    char wall_str[50];
    sprintf(wall_str, "Wall Level: %d", game->wall_level);
    mlx_string_put(game->mlx, game->win, 10, 80, 0xFFFFFF, wall_str);

    mlx_string_put(game->mlx, game->win, 10, WIN_HEIGHT - 80, 0xCCCCCC, "Controls:");
    mlx_string_put(game->mlx, game->win, 10, WIN_HEIGHT - 60, 0xCCCCCC, "WASD: Move");
    mlx_string_put(game->mlx, game->win, 10, WIN_HEIGHT - 40, 0xCCCCCC, "SPACE: Shoot");
    mlx_string_put(game->mlx, game->win, 10, WIN_HEIGHT - 20, 0xCCCCCC, "ESC: Quit");
}
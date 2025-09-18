#include "MLX42/include/MLX42/MLX42.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include <math.h>

#define WIN_WIDTH 600
#define WIN_HEIGHT 500
#define TILE_SIZE 25

typedef struct {
    mlx_t *mlx;
    mlx_image_t *img;
    int player_x, player_y;
    int army_count;
    char map[20][20];
    int map_width, map_height;
    int collectibles_total, collectibles_taken;
    int moves;
    int victory;
} t_game;

void put_pixel(mlx_image_t *img, int x, int y, uint32_t color)
{
    if (x >= 0 && x < (int)img->width && y >= 0 && y < (int)img->height)
        mlx_put_pixel(img, x, y, color);
}

void draw_rectangle(mlx_image_t *img, int x, int y, int width, int height, uint32_t color)
{
    for (int i = 0; i < width && x + i < (int)img->width; i++)
        for (int j = 0; j < height && y + j < (int)img->height; j++)
            if (x + i >= 0 && y + j >= 0)
                put_pixel(img, x + i, y + j, color);
}

void draw_circle(mlx_image_t *img, int cx, int cy, int radius, uint32_t color)
{
    for (int x = cx - radius; x <= cx + radius; x++)
        for (int y = cy - radius; y <= cy + radius; y++)
            if ((x-cx)*(x-cx) + (y-cy)*(y-cy) <= radius*radius)
                put_pixel(img, x, y, color);
}

void init_game(t_game *game)
{
    // Mappa semplice e funzionante
    char test_map[12][15] = {
        "11111111111111",
        "10000000000001",
        "100C0000C00001",
        "10000000000001",
        "10000000000001",
        "100C0000C00001",
        "10000000000001",
        "1000000E000001",
        "10000000000001",
        "10000000000001",
        "1000000P000001",
        "11111111111111"
    };

    game->map_height = 12;
    game->map_width = 14;
    game->collectibles_total = 0;

    for (int y = 0; y < game->map_height; y++)
    {
        strcpy(game->map[y], test_map[y]);
        for (int x = 0; x < game->map_width; x++)
        {
            if (game->map[y][x] == 'P')
            {
                game->player_x = x;
                game->player_y = y;
                game->map[y][x] = '0';
                printf("Player starts at (%d,%d)\n", x, y);
            }
            else if (game->map[y][x] == 'C')
            {
                game->collectibles_total++;
            }
        }
    }

    game->army_count = 30;
    game->collectibles_taken = 0;
    game->moves = 0;
    game->victory = 0;

    printf("Map loaded: %dx%d, collectibles: %d\n",
           game->map_width, game->map_height, game->collectibles_total);
}

void render_game(t_game *game)
{
    // Clear con sfondo scuro
    for (int y = 0; y < WIN_HEIGHT; y++)
        for (int x = 0; x < WIN_WIDTH; x++)
            put_pixel(game->img, x, y, 0x222222FF);

    // Render mappa
    for (int y = 0; y < game->map_height; y++)
    {
        for (int x = 0; x < game->map_width; x++)
        {
            int screen_x = x * TILE_SIZE + 20;
            int screen_y = y * TILE_SIZE + 20;

            // Muri grigi
            if (game->map[y][x] == '1')
            {
                draw_rectangle(game->img, screen_x, screen_y, TILE_SIZE-1, TILE_SIZE-1, 0x808080FF);
            }
            // Collectibles ciano
            else if (game->map[y][x] == 'C')
            {
                draw_rectangle(game->img, screen_x+2, screen_y+2, TILE_SIZE-4, TILE_SIZE-4, 0x00FFFFFF);
            }
            // Exit verde
            else if (game->map[y][x] == 'E')
            {
                uint32_t color = (game->collectibles_taken == game->collectibles_total) ? 0x00FF00FF : 0x006600FF;
                draw_rectangle(game->img, screen_x+2, screen_y+2, TILE_SIZE-4, TILE_SIZE-4, color);
            }
            // Floor
            else if (game->map[y][x] == '0')
            {
                draw_rectangle(game->img, screen_x+3, screen_y+3, TILE_SIZE-6, TILE_SIZE-6, 0x444444FF);
            }
        }
    }

    // Player (cerchio blu)
    int px = game->player_x * TILE_SIZE + 20 + TILE_SIZE/2;
    int py = game->player_y * TILE_SIZE + 20 + TILE_SIZE/2;
    draw_circle(game->img, px, py, 8, 0x0088FFFF);

    // Army (cerchietti verdi)
    int army_show = game->army_count > 16 ? 16 : game->army_count;
    for (int i = 0; i < army_show; i++)
    {
        double angle = (2.0 * M_PI * i) / army_show;
        int sx = px + cos(angle) * 15;
        int sy = py + sin(angle) * 15;
        draw_circle(game->img, sx, sy, 2, 0x00FF00FF);
    }

    // UI in alto
    draw_rectangle(game->img, 10, 10, 300, 60, 0x000000AA);

    // Victory message
    if (game->victory)
    {
        draw_rectangle(game->img, WIN_WIDTH/2-60, WIN_HEIGHT/2-15, 120, 30, 0x00FF00FF);
    }
}

void key_hook(mlx_key_data_t keydata, void *param)
{
    t_game *game = (t_game*)param;

    if (keydata.key == MLX_KEY_ESCAPE && keydata.action == MLX_PRESS)
    {
        printf("ESC - Closing game\n");
        mlx_close_window(game->mlx);
        return;
    }

    if (game->victory)
        return;

    if (keydata.action != MLX_PRESS)
        return;

    int new_x = game->player_x;
    int new_y = game->player_y;
    int moved = 0;

    printf("Key: %d\n", keydata.key);

    switch (keydata.key)
    {
        case MLX_KEY_A:
        case MLX_KEY_LEFT:
            new_x--;
            moved = 1;
            printf("LEFT\n");
            break;
        case MLX_KEY_D:
        case MLX_KEY_RIGHT:
            new_x++;
            moved = 1;
            printf("RIGHT\n");
            break;
        case MLX_KEY_W:
        case MLX_KEY_UP:
            new_y--;
            moved = 1;
            printf("UP\n");
            break;
        case MLX_KEY_S:
        case MLX_KEY_DOWN:
            new_y++;
            moved = 1;
            printf("DOWN\n");
            break;
    }

    if (moved)
    {
        // Check bounds
        if (new_x >= 0 && new_x < game->map_width &&
            new_y >= 0 && new_y < game->map_height)
        {
            // Check not wall
            if (game->map[new_y][new_x] != '1')
            {
                game->player_x = new_x;
                game->player_y = new_y;
                game->moves++;
                printf("Moved to (%d,%d) - moves: %d\n", new_x, new_y, game->moves);

                // Check collectible
                if (game->map[new_y][new_x] == 'C')
                {
                    game->army_count += 25;
                    game->map[new_y][new_x] = '0';
                    game->collectibles_taken++;
                    printf("Collected! Army: %d, C taken: %d/%d\n",
                           game->army_count, game->collectibles_taken, game->collectibles_total);
                }
                // Check exit
                else if (game->map[new_y][new_x] == 'E')
                {
                    if (game->collectibles_taken == game->collectibles_total)
                    {
                        game->victory = 1;
                        printf("*** VICTORY! You escaped! ***\n");
                    }
                    else
                    {
                        printf("Exit locked! Need %d more collectibles\n",
                               game->collectibles_total - game->collectibles_taken);
                    }
                }
            }
            else
            {
                printf("Wall blocked!\n");
            }
        }
        else
        {
            printf("Out of bounds!\n");
        }
    }
}

void game_loop(void *param)
{
    t_game *game = (t_game*)param;
    render_game(game);
}

int main(void)
{
    t_game game;

    printf("Initializing So Long Runner...\n");

    game.mlx = mlx_init(WIN_WIDTH, WIN_HEIGHT, "So Long Runner - FIXED", true);
    if (!game.mlx)
    {
        printf("MLX init failed!\n");
        return 1;
    }

    game.img = mlx_new_image(game.mlx, WIN_WIDTH, WIN_HEIGHT);
    if (!game.img || mlx_image_to_window(game.mlx, game.img, 0, 0) == -1)
    {
        printf("Image creation failed!\n");
        return 1;
    }

    init_game(&game);

    printf("\n=== GAME STARTED ===\n");
    printf("WASD or Arrow Keys = Move\n");
    printf("Collect all cyan squares (C), then reach green exit (E)\n");
    printf("ESC = Quit\n\n");

    mlx_key_hook(game.mlx, key_hook, &game);
    mlx_loop_hook(game.mlx, game_loop, &game);
    mlx_loop(game.mlx);

    mlx_terminate(game.mlx);
    printf("Game ended.\n");
    return 0;
}
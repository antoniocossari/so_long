#include "MLX42/include/MLX42/MLX42.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include <math.h>

#define WIN_WIDTH 700
#define WIN_HEIGHT 600
#define TILE_SIZE 30
#define AUTO_STEP_MS 2000    // Auto-step ogni 2 secondi
#define RISE_MS 5000         // Muro sale ogni 5 secondi

typedef struct {
    mlx_t *mlx;
    mlx_image_t *img;
    int player_x, player_y;
    int army_count;
    char map[20][25];
    int map_width, map_height;
    int collectibles_total, collectibles_taken;
    int moves;
    int victory, game_over;
    int wall_level;
    long last_auto_step, last_rise;
} t_game;

long get_time_ms(void)
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (tv.tv_sec * 1000 + tv.tv_usec / 1000);
}

void put_pixel(mlx_image_t *img, int x, int y, uint32_t color)
{
    if (x >= 0 && x < (int)img->width && y >= 0 && y < (int)img->height)
        mlx_put_pixel(img, x, y, color);
}

void draw_rectangle(mlx_image_t *img, int x, int y, int width, int height, uint32_t color)
{
    for (int i = 0; i < width; i++)
        for (int j = 0; j < height; j++)
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
    char test_map[15][20] = {
        "1111111111111111111",
        "1000000000000000001",
        "100C000C000000C0001",
        "1000000000000000001",
        "1000000000000000001",
        "100C000C000000C0001",
        "1000000000000000001",
        "1000000000000000001",
        "100C000C000000C0001",
        "1000000000000000001",
        "10000000E0000000001",
        "1000000000000000001",
        "1000000000000000001",
        "10000000P0000000001",
        "1111111111111111111"
    };

    game->map_height = 15;
    game->map_width = 19;
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
            }
            else if (game->map[y][x] == 'C')
                game->collectibles_total++;
        }
    }

    game->army_count = 30;
    game->collectibles_taken = 0;
    game->moves = 0;
    game->victory = 0;
    game->game_over = 0;
    game->wall_level = game->map_height - 1;

    long current_time = get_time_ms();
    game->last_auto_step = current_time;
    game->last_rise = current_time;

    printf("So Long Runner initialized!\n");
    printf("Player at (%d,%d), Army: %d, Collectibles: %d\n",
           game->player_x, game->player_y, game->army_count, game->collectibles_total);
}

void update_game(t_game *game)
{
    if (game->game_over || game->victory)
        return;

    long current_time = get_time_ms();

    // Auto-step UP
    if (current_time - game->last_auto_step >= AUTO_STEP_MS)
    {
        if (game->player_y > 0 &&
            game->map[game->player_y - 1][game->player_x] != '1' &&
            game->player_y - 1 > game->wall_level)
        {
            game->player_y--;
            game->moves++;
            printf("AUTO-STEP UP! Position: (%d,%d), Moves: %d\n",
                   game->player_x, game->player_y, game->moves);

            // Check collectible
            if (game->map[game->player_y][game->player_x] == 'C')
            {
                game->army_count += 30;
                game->map[game->player_y][game->player_x] = '0';
                game->collectibles_taken++;
                printf("AUTO-COLLECTED! Army: %d, C: %d/%d\n",
                       game->army_count, game->collectibles_taken, game->collectibles_total);
            }
            // Check exit
            else if (game->map[game->player_y][game->player_x] == 'E' &&
                     game->collectibles_taken == game->collectibles_total)
            {
                game->victory = 1;
                printf("*** AUTO-VICTORY! ***\n");
            }
        }
        game->last_auto_step = current_time;
    }

    // Wall rising
    if (current_time - game->last_rise >= RISE_MS)
    {
        if (game->wall_level > 0)
        {
            for (int x = 0; x < game->map_width; x++)
                game->map[game->wall_level][x] = '1';

            game->wall_level--;
            printf("WALL RISING! Level: %d\n", game->wall_level);
        }
        game->last_rise = current_time;
    }

    // Check wall collision
    if (game->player_y <= game->wall_level)
    {
        game->game_over = 1;
        printf("*** GAME OVER! Wall caught you! ***\n");
    }
}

void render_game(t_game *game)
{
    // Clear background
    for (int y = 0; y < WIN_HEIGHT; y++)
        for (int x = 0; x < WIN_WIDTH; x++)
            put_pixel(game->img, x, y, 0x001122FF);

    // Render map
    for (int y = 0; y < game->map_height; y++)
    {
        for (int x = 0; x < game->map_width; x++)
        {
            int screen_x = x * TILE_SIZE + 30;
            int screen_y = y * TILE_SIZE + 80;

            // Walls + risen wall
            if (game->map[y][x] == '1' || y <= game->wall_level)
            {
                uint32_t color = (y <= game->wall_level) ? 0xFF2222FF : 0x888888FF;
                draw_rectangle(game->img, screen_x, screen_y, TILE_SIZE-2, TILE_SIZE-2, color);
            }
            // Collectibles
            else if (game->map[y][x] == 'C')
            {
                draw_rectangle(game->img, screen_x+3, screen_y+3, TILE_SIZE-6, TILE_SIZE-6, 0x00FFFFFF);
                // Math operation text (simplified)
                draw_rectangle(game->img, screen_x+8, screen_y+8, 8, 8, 0xFFFF00FF);
            }
            // Exit
            else if (game->map[y][x] == 'E')
            {
                uint32_t color = (game->collectibles_taken == game->collectibles_total) ? 0x00FF00FF : 0x664400FF;
                draw_rectangle(game->img, screen_x+3, screen_y+3, TILE_SIZE-6, TILE_SIZE-6, color);
            }
            // Floor
            else if (game->map[y][x] == '0')
            {
                draw_rectangle(game->img, screen_x+5, screen_y+5, TILE_SIZE-10, TILE_SIZE-10, 0x333333FF);
            }
        }
    }

    // Player
    int px = game->player_x * TILE_SIZE + 30 + TILE_SIZE/2;
    int py = game->player_y * TILE_SIZE + 80 + TILE_SIZE/2;
    draw_circle(game->img, px, py, 10, 0x0088FFFF);

    // Army (rotating soldiers)
    int army_show = game->army_count > 20 ? 20 : game->army_count;
    for (int i = 0; i < army_show; i++)
    {
        double angle = (2.0 * M_PI * i) / army_show + (get_time_ms() / 1000.0);
        int sx = px + cos(angle) * 18;
        int sy = py + sin(angle) * 18;
        draw_circle(game->img, sx, sy, 2, 0x00FF00FF);
    }

    // HUD
    draw_rectangle(game->img, 10, 10, 680, 60, 0x000000BB);

    // Status text backgrounds
    draw_rectangle(game->img, 20, 20, 100, 15, 0x004488FF);  // Army
    draw_rectangle(game->img, 140, 20, 80, 15, 0x884400FF);  // Moves
    draw_rectangle(game->img, 240, 20, 120, 15, 0x008844FF); // Collectibles
    draw_rectangle(game->img, 380, 20, 100, 15, 0x884400FF); // Wall

    // Instructions
    draw_rectangle(game->img, 20, 40, 450, 15, 0x333333FF);

    // Game over/victory
    if (game->game_over)
    {
        draw_rectangle(game->img, WIN_WIDTH/2-100, WIN_HEIGHT/2-20, 200, 40, 0xFF0000FF);
        draw_rectangle(game->img, WIN_WIDTH/2-80, WIN_HEIGHT/2-10, 160, 20, 0xFFFFFFFF);
    }
    else if (game->victory)
    {
        draw_rectangle(game->img, WIN_WIDTH/2-100, WIN_HEIGHT/2-20, 200, 40, 0x00FF00FF);
        draw_rectangle(game->img, WIN_WIDTH/2-80, WIN_HEIGHT/2-10, 160, 20, 0xFFFFFFFF);
    }
}

void key_hook(mlx_key_data_t keydata, void *param)
{
    t_game *game = (t_game*)param;

    if (keydata.key == MLX_KEY_ESCAPE && keydata.action == MLX_PRESS)
    {
        mlx_close_window(game->mlx);
        return;
    }

    if (game->game_over || game->victory || keydata.action != MLX_PRESS)
        return;

    int new_x = game->player_x;
    int new_y = game->player_y;
    int moved = 0;

    switch (keydata.key)
    {
        case MLX_KEY_A:
        case MLX_KEY_LEFT:
            new_x--;
            moved = 1;
            break;
        case MLX_KEY_D:
        case MLX_KEY_RIGHT:
            new_x++;
            moved = 1;
            break;
        case MLX_KEY_W:
        case MLX_KEY_UP:
            new_y--;
            moved = 1;
            break;
        case MLX_KEY_S:
        case MLX_KEY_DOWN:
            new_y++;
            moved = 1;
            break;
        default:
            break;
    }

    if (moved &&
        new_x >= 0 && new_x < game->map_width &&
        new_y >= 0 && new_y < game->map_height &&
        new_y > game->wall_level &&
        game->map[new_y][new_x] != '1')
    {
        game->player_x = new_x;
        game->player_y = new_y;
        game->moves++;

        printf("Manual move to (%d,%d) - moves: %d\n", new_x, new_y, game->moves);

        // Check collectible
        if (game->map[new_y][new_x] == 'C')
        {
            game->army_count += 30;
            game->map[new_y][new_x] = '0';
            game->collectibles_taken++;
            printf("COLLECTED! Army: %d, C: %d/%d\n",
                   game->army_count, game->collectibles_taken, game->collectibles_total);
        }
        // Check exit
        else if (game->map[new_y][new_x] == 'E')
        {
            if (game->collectibles_taken == game->collectibles_total)
            {
                game->victory = 1;
                printf("*** MANUAL VICTORY! ***\n");
            }
            else
            {
                printf("Exit locked! Need %d more C\n",
                       game->collectibles_total - game->collectibles_taken);
            }
        }
    }
}

void game_loop(void *param)
{
    t_game *game = (t_game*)param;
    update_game(game);
    render_game(game);
}

int main(void)
{
    t_game game;

    game.mlx = mlx_init(WIN_WIDTH, WIN_HEIGHT, "So Long Runner - COMPLETE", true);
    if (!game.mlx)
        return 1;

    game.img = mlx_new_image(game.mlx, WIN_WIDTH, WIN_HEIGHT);
    if (!game.img || mlx_image_to_window(game.mlx, game.img, 0, 0) == -1)
        return 1;

    init_game(&game);

    printf("\n=== SO LONG RUNNER - COMPLETE VERSION ===\n");
    printf("🎮 WASD/Arrows = Move manually\n");
    printf("⏰ Auto-step UP every 2 seconds\n");
    printf("⚠️  Red wall rises every 5 seconds\n");
    printf("🎯 Collect all cyan squares (C) to unlock exit (E)\n");
    printf("🛡️  Your army grows with each collectible!\n");
    printf("ESC = Quit\n\n");

    mlx_key_hook(game.mlx, key_hook, &game);
    mlx_loop_hook(game.mlx, game_loop, &game);
    mlx_loop(game.mlx);

    mlx_terminate(game.mlx);
    return 0;
}
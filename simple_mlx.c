#include "MLX42/include/MLX42/MLX42.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include <math.h>

#define WIN_WIDTH 800
#define WIN_HEIGHT 600
#define TILE_SIZE 30
#define AUTO_STEP_MS 1500
#define RISE_MS 3000

typedef struct {
    mlx_t *mlx;
    mlx_image_t *img;
    int player_x, player_y;
    int army_count;
    int map_width, map_height;
    char map[20][25];
    int wall_level;
    long last_auto_step;
    long last_rise;
    int game_over;
    int victory;
    int moves;
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

void init_map(t_game *game)
{
    char test_map[15][19] = {
        "111111111111111111",
        "100000000000000001",
        "10C000C000000C0001",
        "100000000000000001",
        "100000000000000001",
        "10C000C000000C0001",
        "100000000000000001",
        "10000000E000000001",
        "100000000000000001",
        "100000000000000001",
        "10C000C000000C0001",
        "100000000000000001",
        "100000000000000001",
        "10000000P000000001",
        "111111111111111111"
    };

    game->map_height = 15;
    game->map_width = 18;

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
        }
    }

    game->army_count = 30;
    game->wall_level = game->map_height - 1;
    game->game_over = 0;
    game->victory = 0;
    game->moves = 0;

    long current_time = get_time_ms();
    game->last_auto_step = current_time;
    game->last_rise = current_time;
}

void update_game(t_game *game)
{
    if (game->game_over || game->victory)
        return;

    long current_time = get_time_ms();

    // Auto-step up
    if (current_time - game->last_auto_step >= AUTO_STEP_MS)
    {
        if (game->player_y > 0 &&
            game->map[game->player_y - 1][game->player_x] != '1' &&
            game->player_y - 1 > game->wall_level)
        {
            game->player_y--;
            game->moves++;
            printf("Auto-step! Moves: %d, Position: (%d,%d)\n",
                   game->moves, game->player_x, game->player_y);

            // Check collectible
            if (game->map[game->player_y][game->player_x] == 'C')
            {
                game->army_count += 20; // Simple +20
                game->map[game->player_y][game->player_x] = '0';
                printf("Collected! Army now: %d\n", game->army_count);
            }
            // Check exit
            else if (game->map[game->player_y][game->player_x] == 'E')
            {
                game->victory = 1;
                printf("VICTORY! You've reached the exit!\n");
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
            printf("Wall rising! Level: %d\n", game->wall_level);
        }
        game->last_rise = current_time;
    }

    // Check if wall caught player
    if (game->player_y <= game->wall_level)
    {
        game->game_over = 1;
        printf("GAME OVER! Wall caught you!\n");
    }
}

void render_game(t_game *game)
{
    // Clear screen
    for (int y = 0; y < WIN_HEIGHT; y++)
        for (int x = 0; x < WIN_WIDTH; x++)
            put_pixel(game->img, x, y, 0x001122FF);

    // Render map
    for (int y = 0; y < game->map_height; y++)
    {
        for (int x = 0; x < game->map_width; x++)
        {
            int screen_x = x * TILE_SIZE + 50;
            int screen_y = y * TILE_SIZE + 50;

            // Wall or risen wall
            if (game->map[y][x] == '1' || y <= game->wall_level)
            {
                uint32_t color = (y <= game->wall_level) ? 0xFF2222FF : 0x888888FF;
                draw_rectangle(game->img, screen_x, screen_y, TILE_SIZE-2, TILE_SIZE-2, color);
            }
            // Collectible
            else if (game->map[y][x] == 'C')
            {
                draw_rectangle(game->img, screen_x+3, screen_y+3, TILE_SIZE-6, TILE_SIZE-6, 0x00FFFFFF);
            }
            // Exit
            else if (game->map[y][x] == 'E')
            {
                draw_rectangle(game->img, screen_x+3, screen_y+3, TILE_SIZE-6, TILE_SIZE-6, 0x00FF00FF);
            }
            // Floor
            else
            {
                draw_rectangle(game->img, screen_x+5, screen_y+5, TILE_SIZE-10, TILE_SIZE-10, 0x333333FF);
            }
        }
    }

    // Player
    int px = game->player_x * TILE_SIZE + 50 + TILE_SIZE/2;
    int py = game->player_y * TILE_SIZE + 50 + TILE_SIZE/2;
    draw_circle(game->img, px, py, 12, 0x0088FFFF);

    // Army (simplified circles around player)
    int army_display = game->army_count > 20 ? 20 : game->army_count;
    for (int i = 0; i < army_display; i++)
    {
        double angle = (2.0 * M_PI * i) / army_display;
        int sx = px + cos(angle) * 20;
        int sy = py + sin(angle) * 20;
        draw_circle(game->img, sx, sy, 2, 0x00FF00FF);
    }

    // UI
    char ui_text[100];
    sprintf(ui_text, "Army: %d | Moves: %d | Wall: %d",
            game->army_count, game->moves, game->wall_level);

    // Simple text background
    draw_rectangle(game->img, 10, 10, 400, 30, 0x000000AA);

    // Game over/victory
    if (game->game_over)
    {
        draw_rectangle(game->img, WIN_WIDTH/2-80, WIN_HEIGHT/2-20, 160, 40, 0xFF0000FF);
    }
    else if (game->victory)
    {
        draw_rectangle(game->img, WIN_WIDTH/2-80, WIN_HEIGHT/2-20, 160, 40, 0x00FF00FF);
    }
}

void key_hook(mlx_key_data_t keydata, void *param)
{
    t_game *game = (t_game*)param;

    printf("Key pressed: %d, action: %d\n", keydata.key, keydata.action);

    if (keydata.key == MLX_KEY_ESCAPE && keydata.action == MLX_PRESS)
    {
        printf("ESC pressed - closing\n");
        mlx_close_window(game->mlx);
        return;
    }

    if (game->game_over || game->victory)
        return;

    if (keydata.action != MLX_PRESS)
        return;

    int new_x = game->player_x;
    int new_y = game->player_y;

    if (keydata.key == MLX_KEY_A || keydata.key == MLX_KEY_LEFT)
    {
        new_x--;
        printf("Moving LEFT\n");
    }
    else if (keydata.key == MLX_KEY_D || keydata.key == MLX_KEY_RIGHT)
    {
        new_x++;
        printf("Moving RIGHT\n");
    }
    else if (keydata.key == MLX_KEY_W || keydata.key == MLX_KEY_UP)
    {
        new_y--;
        printf("Moving UP\n");
    }
    else if (keydata.key == MLX_KEY_S || keydata.key == MLX_KEY_DOWN)
    {
        new_y++;
        printf("Moving DOWN\n");
    }

    // Check bounds and walls
    if (new_x >= 0 && new_x < game->map_width &&
        new_y >= 0 && new_y < game->map_height &&
        new_y > game->wall_level &&
        game->map[new_y][new_x] != '1')
    {
        game->player_x = new_x;
        game->player_y = new_y;
        game->moves++;
        printf("Moved to (%d,%d), moves: %d\n", new_x, new_y, game->moves);

        // Check collectible
        if (game->map[new_y][new_x] == 'C')
        {
            game->army_count += 20;
            game->map[new_y][new_x] = '0';
            printf("Collected! Army now: %d\n", game->army_count);
        }
        // Check exit
        else if (game->map[new_y][new_x] == 'E')
        {
            game->victory = 1;
            printf("VICTORY!\n");
        }
    }
    else
    {
        printf("Invalid move to (%d,%d)\n", new_x, new_y);
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

    game.mlx = mlx_init(WIN_WIDTH, WIN_HEIGHT, "So Long Runner - WORKING", true);
    if (!game.mlx)
    {
        printf("Error: Failed to initialize MLX\n");
        return 1;
    }

    game.img = mlx_new_image(game.mlx, WIN_WIDTH, WIN_HEIGHT);
    if (!game.img)
    {
        printf("Error: Failed to create image\n");
        return 1;
    }

    if (mlx_image_to_window(game.mlx, game.img, 0, 0) == -1)
    {
        printf("Error: Failed to put image to window\n");
        return 1;
    }

    init_map(&game);

    printf("Game started!\n");
    printf("Controls: WASD or Arrow Keys = Move, ESC = Quit\n");
    printf("Player starts at (%d,%d)\n", game.player_x, game.player_y);

    mlx_key_hook(game.mlx, key_hook, &game);
    mlx_loop_hook(game.mlx, game_loop, &game);
    mlx_loop(game.mlx);

    mlx_terminate(game.mlx);
    return 0;
}
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
    char map[15][20];
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
            }
            else if (game->map[y][x] == 'C')
                game->collectibles_total++;
        }
    }

    game->army_count = 30;
    game->collectibles_taken = 0;
    game->moves = 0;
    game->victory = 0;

    printf("=== GAME READY ===\n");
    printf("Player at (%d,%d)\n", game->player_x, game->player_y);
    printf("Army: %d soldiers\n", game->army_count);
    printf("Collectibles to find: %d\n", game->collectibles_total);
}

void render_game(t_game *game)
{
    // Clear background - blue
    for (int y = 0; y < WIN_HEIGHT; y++)
        for (int x = 0; x < WIN_WIDTH; x++)
            put_pixel(game->img, x, y, 0x112244FF);

    // Render map tiles
    for (int y = 0; y < game->map_height; y++)
    {
        for (int x = 0; x < game->map_width; x++)
        {
            int screen_x = x * TILE_SIZE + 50;
            int screen_y = y * TILE_SIZE + 50;

            // Walls - gray
            if (game->map[y][x] == '1')
            {
                draw_rectangle(game->img, screen_x, screen_y, TILE_SIZE-1, TILE_SIZE-1, 0x888888FF);
            }
            // Collectibles - cyan
            else if (game->map[y][x] == 'C')
            {
                draw_rectangle(game->img, screen_x+3, screen_y+3, TILE_SIZE-6, TILE_SIZE-6, 0x00FFFFFF);
            }
            // Exit - green if all collected, orange if locked
            else if (game->map[y][x] == 'E')
            {
                uint32_t color = (game->collectibles_taken == game->collectibles_total) ? 0x00FF00FF : 0xFF8800FF;
                draw_rectangle(game->img, screen_x+3, screen_y+3, TILE_SIZE-6, TILE_SIZE-6, color);
            }
            // Floor - dark gray
            else if (game->map[y][x] == '0')
            {
                draw_rectangle(game->img, screen_x+5, screen_y+5, TILE_SIZE-10, TILE_SIZE-10, 0x333333FF);
            }
        }
    }

    // Player - big blue circle
    int px = game->player_x * TILE_SIZE + 50 + TILE_SIZE/2;
    int py = game->player_y * TILE_SIZE + 50 + TILE_SIZE/2;
    draw_circle(game->img, px, py, 8, 0x0088FFFF);

    // Army - green dots rotating around player
    int army_show = game->army_count > 16 ? 16 : game->army_count;
    for (int i = 0; i < army_show; i++)
    {
        double angle = (2.0 * M_PI * i) / army_show;
        int sx = px + cos(angle) * 15;
        int sy = py + sin(angle) * 15;
        draw_circle(game->img, sx, sy, 2, 0x00FF00FF);
    }

    // HUD - black background
    draw_rectangle(game->img, 10, 10, 580, 30, 0x000000DD);

    // Show army count with colored background
    draw_rectangle(game->img, 15, 15, 150, 20, 0x004400FF);

    // Show moves
    draw_rectangle(game->img, 175, 15, 100, 20, 0x440000FF);

    // Show collectibles
    draw_rectangle(game->img, 285, 15, 150, 20, 0x004444FF);

    // Victory message
    if (game->victory)
    {
        draw_rectangle(game->img, WIN_WIDTH/2-80, WIN_HEIGHT/2-20, 160, 40, 0x00FF00FF);
        draw_rectangle(game->img, WIN_WIDTH/2-70, WIN_HEIGHT/2-10, 140, 20, 0xFFFFFFFF);
    }
}

void move_player(t_game *game, int new_x, int new_y, const char* direction)
{
    printf("Trying to move %s: (%d,%d) -> (%d,%d)\n",
           direction, game->player_x, game->player_y, new_x, new_y);

    // Check bounds
    if (new_x < 0 || new_x >= game->map_width || new_y < 0 || new_y >= game->map_height)
    {
        printf("  -> OUT OF BOUNDS\n");
        return;
    }

    // Check wall
    if (game->map[new_y][new_x] == '1')
    {
        printf("  -> WALL BLOCKED\n");
        return;
    }

    // Valid move
    game->player_x = new_x;
    game->player_y = new_y;
    game->moves++;

    printf("  -> SUCCESS! New position: (%d,%d), moves: %d\n",
           new_x, new_y, game->moves);

    // Check collectible
    if (game->map[new_y][new_x] == 'C')
    {
        int old_army = game->army_count;
        game->army_count += 25;
        game->map[new_y][new_x] = '0';
        game->collectibles_taken++;

        printf("  -> COLLECTED! Army: %d -> %d, C: %d/%d\n",
               old_army, game->army_count, game->collectibles_taken, game->collectibles_total);
    }
    // Check exit
    else if (game->map[new_y][new_x] == 'E')
    {
        if (game->collectibles_taken == game->collectibles_total)
        {
            game->victory = 1;
            printf("  -> *** VICTORY! YOU WIN! ***\n");
        }
        else
        {
            printf("  -> Exit locked! Need %d more collectibles\n",
                   game->collectibles_total - game->collectibles_taken);
        }
    }
}

void key_hook(mlx_key_data_t keydata, void *param)
{
    t_game *game = (t_game*)param;

    // Debug key press
    printf("\n=== KEY EVENT ===\n");
    printf("Key code: %d, action: %d\n", keydata.key, keydata.action);

    if (keydata.key == MLX_KEY_ESCAPE)
    {
        printf("ESC pressed - closing window\n");
        mlx_close_window(game->mlx);
        return;
    }

    if (game->victory)
    {
        printf("Game won - ignoring input\n");
        return;
    }

    // Only respond to key press (not release)
    if (keydata.action != MLX_PRESS)
    {
        printf("Not a key press - ignoring\n");
        return;
    }

    int new_x = game->player_x;
    int new_y = game->player_y;

    switch (keydata.key)
    {
        case MLX_KEY_A:
            move_player(game, new_x - 1, new_y, "LEFT");
            break;
        case MLX_KEY_D:
            move_player(game, new_x + 1, new_y, "RIGHT");
            break;
        case MLX_KEY_W:
            move_player(game, new_x, new_y - 1, "UP");
            break;
        case MLX_KEY_S:
            move_player(game, new_x, new_y + 1, "DOWN");
            break;
        case MLX_KEY_LEFT:
            move_player(game, new_x - 1, new_y, "LEFT ARROW");
            break;
        case MLX_KEY_RIGHT:
            move_player(game, new_x + 1, new_y, "RIGHT ARROW");
            break;
        case MLX_KEY_UP:
            move_player(game, new_x, new_y - 1, "UP ARROW");
            break;
        case MLX_KEY_DOWN:
            move_player(game, new_x, new_y + 1, "DOWN ARROW");
            break;
        default:
            printf("Unknown key: %d\n", keydata.key);
            break;
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

    printf("Starting So Long Runner...\n");

    game.mlx = mlx_init(WIN_WIDTH, WIN_HEIGHT, "So Long Runner - WORKING", true);
    if (!game.mlx)
    {
        printf("MLX init failed!\n");
        return 1;
    }

    game.img = mlx_new_image(game.mlx, WIN_WIDTH, WIN_HEIGHT);
    if (!game.img)
    {
        printf("Image creation failed!\n");
        return 1;
    }

    if (mlx_image_to_window(game.mlx, game.img, 0, 0) == -1)
    {
        printf("Image to window failed!\n");
        return 1;
    }

    init_game(&game);

    printf("\n🎮 CONTROLS:\n");
    printf("WASD or Arrow Keys = Move\n");
    printf("ESC = Quit\n");
    printf("\n🎯 GOAL:\n");
    printf("Collect all CYAN squares, then reach the GREEN exit\n");
    printf("Watch your army (green dots) grow!\n\n");

    mlx_key_hook(game.mlx, key_hook, &game);
    mlx_loop_hook(game.mlx, game_loop, &game);
    mlx_loop(game.mlx);

    mlx_terminate(game.mlx);
    printf("Game ended.\n");
    return 0;
}
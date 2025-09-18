#include "MLX42/include/MLX42/MLX42.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include <math.h>

#define WIN_WIDTH 700
#define WIN_HEIGHT 600
#define TILE_SIZE 25

// TIMING CONSTANTS for Runner mechanics (BALANCED)
#define AUTO_STEP_MS 1200    // Auto-step UP every 1.2 seconds (faster)
#define RISE_MS 8000         // Wall rises every 8 seconds (much slower)
#define SHOT_COOLDOWN_MS 300 // Shooting cooldown

typedef struct {
    char operation;          // +, -, *
    int value;              // operation value
} t_math_gate;

typedef struct {
    float x, y;             // position
    float dx, dy;           // velocity
    int active;             // is active
} t_projectile;

typedef struct {
    float x, y;             // position
    int type;               // 0=patrol, 1=chaser, 2=falling
    int active;             // is active
    int move_timer;         // movement timer
} t_enemy;

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

    // RUNNER MECHANICS
    int wall_level;                     // current wall level
    long last_auto_step, last_rise;     // timers
    long last_shot;                     // shooting timer
    t_math_gate gates[20];             // math operations for gates
    t_projectile projectiles[50];      // player projectiles
    t_enemy enemies[10];               // enemies
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
    // RUNNER-STYLE MAP: Player starts at BOTTOM, Exit at TOP
    char runner_map[15][20] = {
        "1111111111111111111",
        "1000000000000000001",  // TOP
        "100C000C000000C0001",  // Gates near top
        "1000000000000000001",
        "1000000000000000001",
        "100C000C000000C0001",  // Middle gates
        "1000000000000000001",
        "10000000E0000000001",  // EXIT in middle-top
        "1000000000000000001",
        "100C000C000000C0001",  // Lower gates
        "1000000000000000001",
        "1000000000000000001",
        "1000000000000000001",
        "10000000P0000000001",  // PLAYER starts at BOTTOM
        "1111111111111111111"   // BOTTOM
    };

    game->map_height = 15;
    game->map_width = 19;
    game->collectibles_total = 0;

    for (int y = 0; y < game->map_height; y++)
    {
        strcpy(game->map[y], runner_map[y]);
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

    // Setup math gates with different operations
    int gate_idx = 0;
    for (int i = 0; i < game->collectibles_total; i++)
    {
        if (i % 3 == 0) {
            game->gates[i].operation = '+';
            game->gates[i].value = 20 + (rand() % 20);  // +20 to +40
        }
        else if (i % 3 == 1) {
            game->gates[i].operation = '*';
            game->gates[i].value = 2;                   // x2
        }
        else {
            game->gates[i].operation = '-';
            game->gates[i].value = 10 + (rand() % 10);  // -10 to -20
        }
    }

    // Initialize other systems
    game->army_count = 30;
    game->collectibles_taken = 0;
    game->moves = 0;
    game->victory = 0;
    game->game_over = 0;

    // RUNNER MECHANICS
    game->wall_level = game->map_height - 1;  // Wall starts at bottom

    long current_time = get_time_ms();
    game->last_auto_step = current_time;
    game->last_rise = current_time;
    game->last_shot = 0;

    // Clear projectiles and enemies
    for (int i = 0; i < 50; i++)
        game->projectiles[i].active = 0;
    for (int i = 0; i < 10; i++)
        game->enemies[i].active = 0;

    printf("=== SO LONG RUNNER INITIALIZED ===\n");
    printf("🎯 Player starts at BOTTOM (%d,%d)\n", game->player_x, game->player_y);
    printf("🏁 Exit 'E' is at TOP\n");
    printf("⬆️  Auto-step UP every %.1f seconds\n", AUTO_STEP_MS/1000.0);
    printf("🧱 Wall rises from BOTTOM every %.1f seconds\n", RISE_MS/1000.0);
    printf("🔢 Army: %d soldiers\n", game->army_count);
    printf("📦 Collectibles: %d math gates\n", game->collectibles_total);
}

void update_runner_mechanics(t_game *game)
{
    if (game->game_over || game->victory)
        return;

    long current_time = get_time_ms();

    // AUTO-STEP UP (core runner mechanic)
    if (current_time - game->last_auto_step >= AUTO_STEP_MS)
    {
        // Try to move UP automatically
        if (game->player_y > 0 &&                                    // Not at top
            game->map[game->player_y - 1][game->player_x] != '1' &&  // No wall above
            game->player_y - 1 > game->wall_level)                   // Above risen wall
        {
            game->player_y--;
            game->moves++;

            printf("⬆️  AUTO-STEP UP! Position: (%d,%d), Moves: %d\n",
                   game->player_x, game->player_y, game->moves);

            // Check what we stepped on
            char cell = game->map[game->player_y][game->player_x];
            if (cell == 'C')
            {
                // Find which gate this is
                int gate_idx = 0;
                for (int y = 0; y < game->map_height; y++) {
                    for (int x = 0; x < game->map_width; x++) {
                        if (game->map[y][x] == 'C') {
                            if (x == game->player_x && y == game->player_y) {
                                // Apply math operation
                                t_math_gate *gate = &game->gates[gate_idx];
                                int old_army = game->army_count;

                                if (gate->operation == '+')
                                    game->army_count += gate->value;
                                else if (gate->operation == '*')
                                    game->army_count *= gate->value;
                                else if (gate->operation == '-') {
                                    game->army_count -= gate->value;
                                    if (game->army_count < 0) game->army_count = 0;
                                }

                                printf("🔢 AUTO-COLLECTED! %d %c%d = %d\n",
                                       old_army, gate->operation, gate->value, game->army_count);

                                game->map[game->player_y][game->player_x] = '0';
                                game->collectibles_taken++;

                                if (game->army_count <= 0) {
                                    game->game_over = 1;
                                    printf("💀 GAME OVER! Army destroyed!\n");
                                }
                                goto found_gate;
                            }
                            gate_idx++;
                        }
                    }
                }
                found_gate:;
            }
            else if (cell == 'E' && game->collectibles_taken == game->collectibles_total)
            {
                game->victory = 1;
                printf("🏆 AUTO-VICTORY! You've escaped!\n");
            }
        }
        else
        {
            printf("⚠️  AUTO-STEP blocked! Wall or obstacle above.\n");
        }

        game->last_auto_step = current_time;
    }

    // WALL RISING (pressure mechanic)
    if (current_time - game->last_rise >= RISE_MS)
    {
        if (game->wall_level > 0)
        {
            // Turn bottom row into wall
            for (int x = 0; x < game->map_width; x++)
                game->map[game->wall_level][x] = '1';

            game->wall_level--;
            printf("🧱 WALL RISING! New level: %d\n", game->wall_level);

            // Check if wall caught player
            if (game->player_y <= game->wall_level)
            {
                game->game_over = 1;
                printf("💀 GAME OVER! Wall caught you!\n");
            }
        }
        game->last_rise = current_time;
    }

    // Update projectiles
    for (int i = 0; i < 50; i++)
    {
        if (game->projectiles[i].active)
        {
            game->projectiles[i].x += game->projectiles[i].dx;
            game->projectiles[i].y += game->projectiles[i].dy;

            // Remove if off screen
            if (game->projectiles[i].y < 0 || game->projectiles[i].y > WIN_HEIGHT ||
                game->projectiles[i].x < 0 || game->projectiles[i].x > WIN_WIDTH)
            {
                game->projectiles[i].active = 0;
            }
        }
    }
}

void render_game(t_game *game)
{
    // Clear background - darker blue for runner feel
    for (int y = 0; y < WIN_HEIGHT; y++)
        for (int x = 0; x < WIN_WIDTH; x++)
            put_pixel(game->img, x, y, 0x001133FF);

    // Render map
    for (int y = 0; y < game->map_height; y++)
    {
        for (int x = 0; x < game->map_width; x++)
        {
            int screen_x = x * TILE_SIZE + 50;
            int screen_y = y * TILE_SIZE + 100;

            // Walls and risen wall (RED for risen areas)
            if (game->map[y][x] == '1')
            {
                uint32_t color = (y <= game->wall_level) ? 0xFF2222FF : 0x888888FF;
                draw_rectangle(game->img, screen_x, screen_y, TILE_SIZE-1, TILE_SIZE-1, color);
            }
            // Math gates - cyan with operation indicator
            else if (game->map[y][x] == 'C')
            {
                draw_rectangle(game->img, screen_x+2, screen_y+2, TILE_SIZE-4, TILE_SIZE-4, 0x00FFFFFF);

                // Find gate index and show operation
                int gate_idx = 0;
                for (int cy = 0; cy < game->map_height; cy++) {
                    for (int cx = 0; cx < game->map_width; cx++) {
                        if (game->map[cy][cx] == 'C') {
                            if (cx == x && cy == y) {
                                t_math_gate *gate = &game->gates[gate_idx];
                                // Small colored dot for operation type
                                uint32_t op_color = (gate->operation == '+') ? 0x00FF00FF :
                                                  (gate->operation == '*') ? 0xFFFF00FF : 0xFF4444FF;
                                draw_circle(game->img, screen_x + TILE_SIZE/2, screen_y + TILE_SIZE/2, 3, op_color);
                                goto next_tile;
                            }
                            gate_idx++;
                        }
                    }
                }
                next_tile:;
            }
            // Exit - green if unlocked, orange if locked
            else if (game->map[y][x] == 'E')
            {
                uint32_t color = (game->collectibles_taken == game->collectibles_total) ? 0x00FF00FF : 0xFF8800FF;
                draw_rectangle(game->img, screen_x+2, screen_y+2, TILE_SIZE-4, TILE_SIZE-4, color);
            }
            // Floor
            else if (game->map[y][x] == '0')
            {
                draw_rectangle(game->img, screen_x+6, screen_y+6, TILE_SIZE-12, TILE_SIZE-12, 0x333333FF);
            }
        }
    }

    // Player - big blue circle
    int px = game->player_x * TILE_SIZE + 50 + TILE_SIZE/2;
    int py = game->player_y * TILE_SIZE + 100 + TILE_SIZE/2;
    draw_circle(game->img, px, py, 8, 0x0088FFFF);

    // Army - rotating green soldiers
    int army_show = game->army_count > 20 ? 20 : game->army_count;
    for (int i = 0; i < army_show; i++)
    {
        double angle = (2.0 * M_PI * i) / army_show + (get_time_ms() / 2000.0);
        int sx = px + cos(angle) * 16;
        int sy = py + sin(angle) * 16;
        draw_circle(game->img, sx, sy, 2, 0x00FF00FF);
    }

    // Show army surplus
    if (game->army_count > 20) {
        draw_rectangle(game->img, px + 20, py - 10, 30, 15, 0x444444FF);
    }

    // Projectiles - yellow dots
    for (int i = 0; i < 50; i++)
    {
        if (game->projectiles[i].active)
        {
            draw_circle(game->img, (int)game->projectiles[i].x, (int)game->projectiles[i].y, 2, 0xFFFF00FF);
        }
    }

    // HUD
    draw_rectangle(game->img, 10, 10, 680, 80, 0x000000CC);

    // Army counter
    draw_rectangle(game->img, 20, 20, 120, 20, 0x004400FF);

    // Moves counter
    draw_rectangle(game->img, 150, 20, 100, 20, 0x440000FF);

    // Gates counter
    draw_rectangle(game->img, 260, 20, 150, 20, 0x004444FF);

    // Wall level
    draw_rectangle(game->img, 420, 20, 120, 20, 0x440044FF);

    // Instructions
    draw_rectangle(game->img, 20, 45, 500, 15, 0x222222FF);
    draw_rectangle(game->img, 20, 65, 400, 15, 0x222222FF);

    // Game over/victory
    if (game->game_over)
    {
        draw_rectangle(game->img, WIN_WIDTH/2-100, WIN_HEIGHT/2-30, 200, 60, 0xFF0000FF);
        draw_rectangle(game->img, WIN_WIDTH/2-90, WIN_HEIGHT/2-20, 180, 40, 0xFFFFFFFF);
    }
    else if (game->victory)
    {
        draw_rectangle(game->img, WIN_WIDTH/2-100, WIN_HEIGHT/2-30, 200, 60, 0x00FF00FF);
        draw_rectangle(game->img, WIN_WIDTH/2-90, WIN_HEIGHT/2-20, 180, 40, 0xFFFFFFFF);
    }
}

void shoot_projectiles(t_game *game)
{
    if (game->army_count <= 0)
        return;

    // Number of shots based on army size
    int shots = game->army_count / 15 + 1;
    if (shots > 5) shots = 5;

    printf("🔫 SHOOTING %d projectiles!\n", shots);

    for (int s = 0; s < shots; s++)
    {
        // Find free projectile slot
        for (int i = 0; i < 50; i++)
        {
            if (!game->projectiles[i].active)
            {
                game->projectiles[i].x = game->player_x * TILE_SIZE + 50 + TILE_SIZE/2;
                game->projectiles[i].y = game->player_y * TILE_SIZE + 100 + TILE_SIZE/2;

                // Spread pattern
                float spread = (shots > 1) ? (s - shots/2.0) * 0.3 : 0;
                game->projectiles[i].dx = spread;
                game->projectiles[i].dy = -5;  // Shoot UP
                game->projectiles[i].active = 1;
                break;
            }
        }
    }
}

void move_player(t_game *game, int new_x, int new_y, const char* direction)
{
    printf("\nTrying to move %s: (%d,%d) -> (%d,%d)\n",
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

    // Check if above wall level
    if (new_y <= game->wall_level)
    {
        printf("  -> WALL LEVEL BLOCKED (wall at %d)\n", game->wall_level);
        return;
    }

    // Valid move
    game->player_x = new_x;
    game->player_y = new_y;
    game->moves++;

    printf("  -> SUCCESS! New position: (%d,%d), moves: %d\n", new_x, new_y, game->moves);

    // Check collectible
    if (game->map[new_y][new_x] == 'C')
    {
        // Find gate index
        int gate_idx = 0;
        for (int cy = 0; cy < game->map_height; cy++) {
            for (int cx = 0; cx < game->map_width; cx++) {
                if (game->map[cy][cx] == 'C') {
                    if (cx == new_x && cy == new_y) {
                        t_math_gate *gate = &game->gates[gate_idx];
                        int old_army = game->army_count;

                        if (gate->operation == '+')
                            game->army_count += gate->value;
                        else if (gate->operation == '*')
                            game->army_count *= gate->value;
                        else if (gate->operation == '-') {
                            game->army_count -= gate->value;
                            if (game->army_count < 0) game->army_count = 0;
                        }

                        printf("  -> 🔢 MANUAL COLLECTED! %d %c%d = %d\n",
                               old_army, gate->operation, gate->value, game->army_count);

                        game->map[new_y][new_x] = '0';
                        game->collectibles_taken++;

                        if (game->army_count <= 0) {
                            game->game_over = 1;
                            printf("💀 ARMY DESTROYED!\n");
                        }
                        return;
                    }
                    gate_idx++;
                }
            }
        }
    }
    // Check exit
    else if (game->map[new_y][new_x] == 'E')
    {
        if (game->collectibles_taken == game->collectibles_total)
        {
            game->victory = 1;
            printf("  -> 🏆 MANUAL VICTORY!\n");
        }
        else
        {
            printf("  -> Exit locked! Need %d more gates\n",
                   game->collectibles_total - game->collectibles_taken);
        }
    }
}

void key_hook(mlx_key_data_t keydata, void *param)
{
    t_game *game = (t_game*)param;

    if (keydata.key == MLX_KEY_ESCAPE)
    {
        mlx_close_window(game->mlx);
        return;
    }

    if (game->game_over || game->victory)
        return;

    if (keydata.action != MLX_PRESS)
        return;

    int new_x = game->player_x;
    int new_y = game->player_y;

    switch (keydata.key)
    {
        case MLX_KEY_A:
        case MLX_KEY_LEFT:
            move_player(game, new_x - 1, new_y, "LEFT");
            break;
        case MLX_KEY_D:
        case MLX_KEY_RIGHT:
            move_player(game, new_x + 1, new_y, "RIGHT");
            break;
        case MLX_KEY_W:
        case MLX_KEY_UP:
            move_player(game, new_x, new_y - 1, "UP");
            break;
        case MLX_KEY_S:
        case MLX_KEY_DOWN:
            move_player(game, new_x, new_y + 1, "DOWN");
            break;
        case MLX_KEY_SPACE:
            {
                long current_time = get_time_ms();
                if (current_time - game->last_shot >= SHOT_COOLDOWN_MS)
                {
                    shoot_projectiles(game);
                    game->last_shot = current_time;
                }
            }
            break;
        default:
            break;
    }
}

void game_loop(void *param)
{
    t_game *game = (t_game*)param;

    // Update all runner mechanics
    update_runner_mechanics(game);

    // Render everything
    render_game(game);
}

int main(void)
{
    t_game game;

    printf("Initializing So Long Runner - COMPLETE VERSION...\n");

    game.mlx = mlx_init(WIN_WIDTH, WIN_HEIGHT, "So Long Runner - COMPLETE", true);
    if (!game.mlx)
        return 1;

    game.img = mlx_new_image(game.mlx, WIN_WIDTH, WIN_HEIGHT);
    if (!game.img || mlx_image_to_window(game.mlx, game.img, 0, 0) == -1)
        return 1;

    init_game(&game);

    printf("\n🎮 === SO LONG RUNNER - COMPLETE ===\n");
    printf("🎯 GOAL: Start at BOTTOM, reach EXIT at TOP\n");
    printf("⬆️  AUTO-STEP: Player moves UP automatically every %.1fs\n", AUTO_STEP_MS/1000.0);
    printf("🧱 WALL RISING: Red wall rises every %.1fs\n", RISE_MS/1000.0);
    printf("🔢 MATH GATES: Collect colored gates to modify army\n");
    printf("   • Green dot = ADD soldiers\n");
    printf("   • Yellow dot = MULTIPLY army\n");
    printf("   • Red dot = SUBTRACT soldiers\n");
    printf("🔫 SHOOTING: SPACE = shoot projectiles UP\n");
    printf("🎮 MANUAL: WASD/Arrows = move manually\n");
    printf("ESC = Quit\n\n");

    mlx_key_hook(game.mlx, key_hook, &game);
    mlx_loop_hook(game.mlx, game_loop, &game);
    mlx_loop(game.mlx);

    mlx_terminate(game.mlx);
    return 0;
}
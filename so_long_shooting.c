#include "MLX42/include/MLX42/MLX42.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include <math.h>

#define WIN_WIDTH 600
#define WIN_HEIGHT 500
#define TILE_SIZE 25

// Runner timing (very slow for testing)
#define AUTO_STEP_MS 3000    // Auto-step every 3 seconds
#define RISE_MS 10000        // Wall rises every 10 seconds
#define SHOOT_INTERVAL_MS 800  // Auto-shoot every 800ms
#define MAX_PROJECTILES 100   // Maximum projectiles on screen

typedef struct {
    char operation;  // '+', '*', '-'
    int value;       // operation value
} t_math_gate;

typedef struct {
    float x, y;      // position
    float dx, dy;    // velocity
    int active;      // is projectile active
} t_projectile;

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

    // RUNNER ADDITIONS
    int wall_level;
    long last_auto_step, last_rise;
    t_math_gate gates[10];  // Math operations for each gate

    // SHOOTING SYSTEM
    t_projectile projectiles[MAX_PROJECTILES];
    long last_shoot;        // Last shooting time
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

void draw_digit(mlx_image_t *img, int x, int y, int digit, uint32_t color)
{
    // Simple 3x5 pixel font for digits 0-9
    int patterns[10][5] = {
        {0b111, 0b101, 0b101, 0b101, 0b111}, // 0
        {0b010, 0b110, 0b010, 0b010, 0b111}, // 1
        {0b111, 0b001, 0b111, 0b100, 0b111}, // 2
        {0b111, 0b001, 0b111, 0b001, 0b111}, // 3
        {0b101, 0b101, 0b111, 0b001, 0b001}, // 4
        {0b111, 0b100, 0b111, 0b001, 0b111}, // 5
        {0b111, 0b100, 0b111, 0b101, 0b111}, // 6
        {0b111, 0b001, 0b001, 0b001, 0b001}, // 7
        {0b111, 0b101, 0b111, 0b101, 0b111}, // 8
        {0b111, 0b101, 0b111, 0b001, 0b111}  // 9
    };

    if (digit < 0 || digit > 9) return;

    for (int row = 0; row < 5; row++) {
        for (int col = 0; col < 3; col++) {
            if (patterns[digit][row] & (1 << (2 - col))) {
                put_pixel(img, x + col, y + row, color);
            }
        }
    }
}

void draw_number(mlx_image_t *img, int x, int y, int number, uint32_t color)
{
    if (number == 0) {
        draw_digit(img, x, y, 0, color);
        return;
    }

    // Handle negative numbers
    if (number < 0) {
        // Draw minus sign
        draw_rectangle(img, x, y + 2, 3, 1, color);
        x += 4;
        number = -number;
    }

    // Convert to string to get digits
    char num_str[10];
    sprintf(num_str, "%d", number);

    for (int i = 0; num_str[i] != '\0'; i++) {
        int digit = num_str[i] - '0';
        draw_digit(img, x + (i * 4), y, digit, color);
    }
}

void draw_operation_text(mlx_image_t *img, int x, int y, char operation, int value, uint32_t color)
{
    // Draw operation symbol
    if (operation == '+') {
        // Draw + symbol (3x3)
        draw_rectangle(img, x + 1, y, 1, 3, color);      // vertical
        draw_rectangle(img, x, y + 1, 3, 1, color);      // horizontal
    } else if (operation == '*') {
        // Draw × symbol
        draw_rectangle(img, x, y, 1, 1, color);
        draw_rectangle(img, x + 2, y, 1, 1, color);
        draw_rectangle(img, x + 1, y + 1, 1, 1, color);
        draw_rectangle(img, x, y + 2, 1, 1, color);
        draw_rectangle(img, x + 2, y + 2, 1, 1, color);
    } else if (operation == '-') {
        // Draw - symbol
        draw_rectangle(img, x, y + 1, 3, 1, color);      // horizontal line
    }

    // Draw the number
    draw_number(img, x + 5, y - 1, value, color);
}

void init_game(t_game *game)
{
    // RUNNER MAP: Player at bottom, exit at top
    char test_map[12][15] = {
        "11111111111111",
        "10000000000001",  // TOP - EXIT HERE
        "100C0000C00001",
        "10000000000001",
        "10000000000001",
        "100C0000C00001",
        "10000000000001",
        "1000000E000001",  // EXIT
        "10000000000001",
        "10000000000001",
        "1000000P000001",  // PLAYER AT BOTTOM
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

    // Setup mathematical operations for gates (more dramatic for testing)
    for (int i = 0; i < game->collectibles_total; i++)
    {
        if (i % 4 == 0) {
            game->gates[i].operation = '+';
            game->gates[i].value = 15 + (rand() % 15);  // +15 to +30
        }
        else if (i % 4 == 1) {
            game->gates[i].operation = '*';
            game->gates[i].value = 2;                   // x2
        }
        else if (i % 4 == 2) {
            game->gates[i].operation = '-';
            game->gates[i].value = 20 + (rand() % 20);  // -20 to -40 (BIG reduction!)
        }
        else {
            game->gates[i].operation = '-';
            game->gates[i].value = 10 + (rand() % 15);  // -10 to -25
        }
        printf("🎲 Gate %d: %c%d\n", i, game->gates[i].operation, game->gates[i].value);
    }

    // RUNNER SETUP - Wall starts WAY below the map (disabled)
    game->wall_level = -5;  // Wall starts way below, won't interfere
    long current_time = get_time_ms();
    game->last_auto_step = current_time;
    game->last_rise = current_time;

    // SHOOTING SETUP
    game->last_shoot = current_time;
    for (int i = 0; i < MAX_PROJECTILES; i++) {
        game->projectiles[i].active = 0;
    }

    printf("=== RUNNER GAME READY ===\n");
    printf("Player at BOTTOM (%d,%d)\n", game->player_x, game->player_y);
    printf("Army: %d soldiers\n", game->army_count);
    printf("Collectibles: %d\n", game->collectibles_total);
    printf("Auto-step every %.1f seconds\n", AUTO_STEP_MS/1000.0);
    printf("Wall rises every %.1f seconds\n", RISE_MS/1000.0);
}

void auto_shoot(t_game *game)
{
    if (game->army_count <= 0) return;  // No army, no shooting

    // Calculate shots based on army size
    int shots = game->army_count;
    if (shots > 20) shots = 20;  // Cap at 20 simultaneous shots

    printf("🔫 AUTO-SHOOTING %d projectiles (army: %d)\n", shots, game->army_count);

    for (int s = 0; s < shots; s++)
    {
        // Find free projectile slot
        for (int i = 0; i < MAX_PROJECTILES; i++)
        {
            if (!game->projectiles[i].active)
            {
                // Start projectile from player position
                game->projectiles[i].x = game->player_x * TILE_SIZE + 50 + TILE_SIZE/2;
                game->projectiles[i].y = game->player_y * TILE_SIZE + 50 + TILE_SIZE/2;

                // Spread pattern for multiple shots (restored)
                float spread_angle = 0;
                if (shots > 1) {
                    spread_angle = ((float)s / (shots - 1) - 0.5) * 0.8;  // Spread between -0.4 and +0.4 radians
                }

                // Shoot UP with slight spread
                game->projectiles[i].dx = spread_angle * 3;  // Horizontal spread
                game->projectiles[i].dy = -6;  // Shoot UP (negative Y)
                game->projectiles[i].active = 1;
                break;
            }
        }
    }
}

void update_projectiles(t_game *game)
{
    for (int i = 0; i < MAX_PROJECTILES; i++)
    {
        if (!game->projectiles[i].active)
            continue;

        // Move projectile
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

void update_runner_mechanics(t_game *game)
{
    long current_time = get_time_ms();

    // AUTO-SHOOTING (constant firing based on army size)
    if (current_time - game->last_shoot >= SHOOT_INTERVAL_MS)
    {
        auto_shoot(game);
        game->last_shoot = current_time;
    }

    // Update projectiles
    update_projectiles(game);

    // AUTO-STEP UP
    if (current_time - game->last_auto_step >= AUTO_STEP_MS)
    {
        printf("\n=== AUTO-STEP ATTEMPT ===\n");
        printf("Current position: (%d,%d)\n", game->player_x, game->player_y);
        printf("Wall level: %d\n", game->wall_level);

        // Try to move UP
        if (game->player_y > 0 &&                                    // Not at top
            game->map[game->player_y - 1][game->player_x] != '1')    // No wall above
        {
            game->player_y--;
            game->moves++;
            printf("AUTO-STEP SUCCESS! New position: (%d,%d)\n", game->player_x, game->player_y);

            // Check what we stepped on
            if (game->map[game->player_y][game->player_x] == 'C')
            {
                // Find which gate this is by position, not by collection count
                int gate_idx = 0;
                int found = 0;
                for (int cy = 0; cy < game->map_height && !found; cy++) {
                    for (int cx = 0; cx < game->map_width && !found; cx++) {
                        if (game->map[cy][cx] == 'C') {
                            if (cx == game->player_x && cy == game->player_y) {
                                found = 1;
                                break;
                            }
                            gate_idx++;
                        }
                    }
                }

                t_math_gate *gate = &game->gates[gate_idx];
                int old_army = game->army_count;

                if (gate->operation == '+') {
                    game->army_count += gate->value;
                    printf("🔥 AUTO-COLLECTED! %d +%d = %d 🔥\n", old_army, gate->value, game->army_count);
                }
                else if (gate->operation == '*') {
                    game->army_count *= gate->value;
                    printf("🔥 AUTO-COLLECTED! %d ×%d = %d 🔥\n", old_army, gate->value, game->army_count);
                }
                else if (gate->operation == '-') {
                    game->army_count -= gate->value;
                    if (game->army_count < 0) game->army_count = 0;
                    printf("🔥 AUTO-COLLECTED! %d -%d = %d 🔥\n", old_army, gate->value, game->army_count);
                }

                game->map[game->player_y][game->player_x] = '0';
                game->collectibles_taken++;

                if (game->army_count <= 0) {
                    printf("💀 ARMY DESTROYED! Game Over!\n");
                }
            }
            else if (game->map[game->player_y][game->player_x] == 'E' &&
                     game->collectibles_taken == game->collectibles_total)
            {
                game->victory = 1;
                printf("AUTO-VICTORY!\n");
            }
        }
        else
        {
            printf("AUTO-STEP BLOCKED!\n");
        }

        game->last_auto_step = current_time;
    }

    // WALL RISING (disabled for now)
    if (current_time - game->last_rise >= RISE_MS)
    {
        if (game->wall_level > 0)
        {
            printf("WALL RISING from level %d to %d\n", game->wall_level, game->wall_level - 1);
            game->wall_level--;
        }
        game->last_rise = current_time;
    }
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

            // Walls and risen wall
            if (game->map[y][x] == '1' || y <= game->wall_level)
            {
                uint32_t color = (y <= game->wall_level) ? 0xFF2222FF : 0x888888FF;
                draw_rectangle(game->img, screen_x, screen_y, TILE_SIZE-1, TILE_SIZE-1, color);
            }
            // Collectibles - cyan with operation indicators
            else if (game->map[y][x] == 'C')
            {
                draw_rectangle(game->img, screen_x+3, screen_y+3, TILE_SIZE-6, TILE_SIZE-6, 0x00FFFFFF);

                // Find which gate this is to show operation with TEXT
                int gate_idx = 0;
                int found = 0;
                for (int cy = 0; cy < game->map_height && !found; cy++) {
                    for (int cx = 0; cx < game->map_width && !found; cx++) {
                        if (game->map[cy][cx] == 'C') {
                            if (cx == x && cy == y) {
                                // This is the gate we're looking for
                                t_math_gate *gate = &game->gates[gate_idx];

                                // Background for text
                                uint32_t bg_color = (gate->operation == '+') ? 0x004400FF :    // Dark green for +
                                                   (gate->operation == '*') ? 0x444400FF :    // Dark yellow for *
                                                                             0x440000FF;     // Dark red for -
                                draw_rectangle(game->img, screen_x + 2, screen_y + 2, TILE_SIZE - 4, 12, bg_color);

                                // Draw operation and number with bitmap font
                                uint32_t text_color = 0xFFFFFFFF;  // White text
                                int text_x = screen_x + 4;
                                int text_y = screen_y + 4;

                                draw_operation_text(game->img, text_x, text_y, gate->operation, gate->value, text_color);

                                found = 1;
                            }
                            gate_idx++;
                        }
                    }
                }
                next_tile:;
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

    // Army - green dots rotating around player (VERY CLEAR COUNT)
    // printf("DEBUG: Rendering army with %d soldiers\n", game->army_count);  // Too much spam

    if (game->army_count > 0) {
        // Show ACTUAL number of soldiers, capped at 50 for visibility
        int max_visual = game->army_count > 50 ? 50 : game->army_count;

        for (int i = 0; i < max_visual; i++)
        {
            // Different rings for different amounts
            int ring = i / 8;  // 8 per ring instead of 12
            double radius = 12 + (ring * 6);
            double angle = (2.0 * M_PI * (i % 8)) / 8 + (get_time_ms() / 1500.0);

            int sx = px + cos(angle) * radius;
            int sy = py + sin(angle) * radius;

            // Bigger dots for better visibility
            draw_circle(game->img, sx, sy, 3, 0x00FF00FF);
        }

        // ALWAYS show army count as actual number
        int count_width = (game->army_count >= 100) ? 50 :
                         (game->army_count >= 10) ? 35 : 25;
        draw_rectangle(game->img, px + 30, py - 10, count_width, 12, 0x000000DD);

        // Draw the actual army number
        draw_number(game->img, px + 32, py - 8, game->army_count, 0x00FF00FF);
    }

    // If army is 0, show warning
    if (game->army_count == 0) {
        draw_circle(game->img, px, py, 20, 0xFF0000AA);  // Red warning circle
    }

    // Render projectiles (yellow/orange bullets)
    for (int i = 0; i < MAX_PROJECTILES; i++)
    {
        if (game->projectiles[i].active)
        {
            int proj_x = (int)game->projectiles[i].x;
            int proj_y = (int)game->projectiles[i].y;
            draw_circle(game->img, proj_x, proj_y, 2, 0xFFAA00FF);  // Orange bullets
        }
    }

    // HUD - black background
    draw_rectangle(game->img, 10, 10, 580, 50, 0x000000DD);

    // Show army count with dynamic color
    uint32_t army_color = (game->army_count == 0) ? 0xFF0000FF :        // Red if 0
                         (game->army_count < 20) ? 0xFF8800FF :         // Orange if low
                         (game->army_count > 100) ? 0x00FF00FF :        // Bright green if high
                                                   0x004400FF;          // Normal green
    draw_rectangle(game->img, 15, 15, 150, 20, army_color);

    // Show moves
    draw_rectangle(game->img, 175, 15, 100, 20, 0x440000FF);

    // Show collectibles with progress bar
    int progress_width = (int)(130.0 * game->collectibles_taken / game->collectibles_total);
    draw_rectangle(game->img, 285, 15, 130, 20, 0x444444FF);  // Background
    draw_rectangle(game->img, 285, 15, progress_width, 20, 0x004444FF);  // Progress

    // Show next gate info
    if (game->collectibles_taken < game->collectibles_total) {
        t_math_gate *next_gate = &game->gates[game->collectibles_taken];
        uint32_t next_color = (next_gate->operation == '+') ? 0x00FF00FF :
                             (next_gate->operation == '*') ? 0xFFFF00FF :
                                                            0xFF4444FF;
        draw_rectangle(game->img, 425, 15, 60, 20, next_color);
    }

    // Instructions
    draw_rectangle(game->img, 15, 35, 500, 15, 0x333333FF);

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

    // For now, no wall level check - just allow movement

    // Valid move
    game->player_x = new_x;
    game->player_y = new_y;
    game->moves++;

    printf("  -> SUCCESS! New position: (%d,%d), moves: %d\n",
           new_x, new_y, game->moves);

    // Check collectible
    if (game->map[new_y][new_x] == 'C')
    {
        // Find which gate this is by position
        int gate_idx = 0;
        int found = 0;
        for (int cy = 0; cy < game->map_height && !found; cy++) {
            for (int cx = 0; cx < game->map_width && !found; cx++) {
                if (game->map[cy][cx] == 'C') {
                    if (cx == new_x && cy == new_y) {
                        found = 1;
                        break;
                    }
                    gate_idx++;
                }
            }
        }

        t_math_gate *gate = &game->gates[gate_idx];
        int old_army = game->army_count;

        if (gate->operation == '+') {
            game->army_count += gate->value;
            printf("🎯 MANUAL COLLECTED! %d +%d = %d 🎯\n", old_army, gate->value, game->army_count);
        }
        else if (gate->operation == '*') {
            game->army_count *= gate->value;
            printf("🎯 MANUAL COLLECTED! %d ×%d = %d 🎯\n", old_army, gate->value, game->army_count);
        }
        else if (gate->operation == '-') {
            game->army_count -= gate->value;
            if (game->army_count < 0) game->army_count = 0;
            printf("🎯 MANUAL COLLECTED! %d -%d = %d 🎯\n", old_army, gate->value, game->army_count);
        }

        game->map[new_y][new_x] = '0';
        game->collectibles_taken++;

        if (game->army_count <= 0) {
            printf("💀 ARMY DESTROYED! Game Over!\n");
        }
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

    if (keydata.key == MLX_KEY_ESCAPE)
    {
        printf("ESC pressed - closing window\n");
        mlx_close_window(game->mlx);
        return;
    }

    if (game->victory)
        return;

    // Only respond to key press (not release)
    if (keydata.action != MLX_PRESS)
        return;

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
            break;
    }
}

void game_loop(void *param)
{
    t_game *game = (t_game*)param;

    // Update runner mechanics
    update_runner_mechanics(game);

    // Render
    render_game(game);
}

int main(void)
{
    t_game game;

    printf("Starting So Long Runner - FIXED VERSION...\n");

    game.mlx = mlx_init(WIN_WIDTH, WIN_HEIGHT, "So Long Runner - FIXED", true);
    if (!game.mlx)
        return 1;

    game.img = mlx_new_image(game.mlx, WIN_WIDTH, WIN_HEIGHT);
    if (!game.img || mlx_image_to_window(game.mlx, game.img, 0, 0) == -1)
        return 1;

    init_game(&game);

    printf("\n🎮 CONTROLS:\n");
    printf("WASD or Arrow Keys = Move manually\n");
    printf("Player auto-steps UP every %.1f seconds\n", AUTO_STEP_MS/1000.0);
    printf("Wall rises every %.1f seconds\n", RISE_MS/1000.0);
    printf("ESC = Quit\n\n");

    mlx_key_hook(game.mlx, key_hook, &game);
    mlx_loop_hook(game.mlx, game_loop, &game);
    mlx_loop(game.mlx);

    mlx_terminate(game.mlx);
    printf("Game ended.\n");
    return 0;
}
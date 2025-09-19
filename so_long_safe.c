#include "mlx_mac/minilibx/mlx.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>

#define TILE_SIZE 32
#define MAX_WIDTH 50
#define MAX_HEIGHT 50

typedef struct s_sprites
{
    void    *floor;
    void    *wall;
    void    *player;
    void    *collectible_valgrind;
    void    *collectible_norminette;
    void    *collectible_tests;
    void    *exit_locked;
    void    *exit_open;
    void    *enemy_norminette;
    void    *enemy_segfault;
    void    *enemy_memory_leak;
    void    *enemy_peer;
    void    *bonus_coin;
} t_sprites;

typedef struct s_enemy
{
    int x;
    int y;
    int type; // 0=norminette, 1=segfault, 2=memory_leak
    int active;
} t_enemy;

typedef struct s_game
{
    void        *mlx;
    void        *window;
    char        map[MAX_HEIGHT][MAX_WIDTH];  // Fixed size arrays instead of malloc
    int         map_width;
    int         map_height;
    int         player_x;
    int         player_y;
    int         collectibles;
    int         collected;
    int         moves;
    int         score;
    int         current_eval;
    int         victory;
    t_enemy     enemies[9]; // 3 enemies per level, max 3 levels
    int         num_enemies;
    int         enemy_move_counter; // Count player moves to slow enemy movement
    t_sprites   sprites;  // Sprite assets
} t_game;

// Function prototypes
int     load_sprites(t_game *game);
int     load_map(t_game *game, char *filename);
int     next_eval(t_game *game);
void    render_game(t_game *game);
int     key_hook(int keycode, t_game *game);
int     close_game(t_game *game);
void    move_player(t_game *game, int new_x, int new_y);
void    spawn_enemies(t_game *game);
void    move_enemies(t_game *game);
void    render_enemies(t_game *game);

int main(int argc, char **argv)
{
    t_game game;

    if (argc != 2)
    {
        printf("Usage: %s eval1.ber\n", argv[0]);
        return (1);
    }

    printf("🚀 Starting Escape from the Cluster...\n");

    // Initialize MLX
    game.mlx = mlx_init();
    if (!game.mlx)
    {
        printf("❌ Error: Failed to initialize MLX\n");
        return (1);
    }

    printf("✅ MLX initialized\n");

    // Initialize enemies array
    game.num_enemies = 0;
    game.enemy_move_counter = 0;
    int i;
    for (i = 0; i < 9; i++)
        game.enemies[i].active = 0;

    // Load sprites first
    if (!load_sprites(&game))
    {
        printf("❌ Error: Failed to load sprites\n");
        return (1);
    }

    printf("✅ Sprites loaded\n");

    // Initialize game state
    game.score = 0;
    game.current_eval = 1;
    game.victory = 0;

    // Load map
    if (!load_map(&game, argv[1]))
    {
        printf("❌ Error: Failed to load map\n");
        return (1);
    }

    printf("✅ Map loaded successfully\n");

    // Create window
    game.window = mlx_new_window(game.mlx,
                                game.map_width * TILE_SIZE,
                                game.map_height * TILE_SIZE,
                                "Escape from the Cluster");
    if (!game.window)
    {
        printf("❌ Error: Failed to create window\n");
        return (1);
    }

    printf("✅ Window created\n");

    printf("\n=== ESCAPE FROM THE CLUSTER ===\n");
    printf("🎓 Eval %d/3 - 42 School Cluster\n", game.current_eval);
    printf("📍 Map: %dx%d\n", game.map_width, game.map_height);
    printf("👤 Peer at: (%d,%d)\n", game.player_x, game.player_y);
    printf("📚 Eval Requirements (C): %d\n", game.collectibles);
    printf("🎯 Goal: Pass all 3 Evals to escape the cluster!\n");
    printf("🎮 Controls: WASD | Progress: Eval1 → Eval2 → Eval3 → Victory!\n\n");

    // Initialize enemies
    spawn_enemies(&game);

    // Set hooks
    mlx_key_hook(game.window, key_hook, &game);
    mlx_hook(game.window, 17, 0, close_game, &game);

    printf("✅ Starting game loop...\n");

    // Render initial state
    render_game(&game);

    // Start event loop
    mlx_loop(game.mlx);

    return (0);
}

int load_sprites(t_game *game)
{
    int w, h;

    printf("🎨 Loading sprites...\n");

    // Load 32x32 sprites with detailed debug
    printf("📂 Loading floor...\n");
    game->sprites.floor = mlx_xpm_file_to_image(game->mlx, "assets/floor_32.xpm", &w, &h);
    if (!game->sprites.floor) { printf("❌ Failed to load floor_32.xpm\n"); return (0); }

    printf("📂 Loading wall...\n");
    game->sprites.wall = mlx_xpm_file_to_image(game->mlx, "assets/wall_desk_32.xpm", &w, &h);
    if (!game->sprites.wall) { printf("❌ Failed to load wall_desk_32.xpm\n"); return (0); }

    printf("📂 Loading player...\n");
    game->sprites.player = mlx_xpm_file_to_image(game->mlx, "assets/player_peer_32.xpm", &w, &h);
    if (!game->sprites.player) { printf("❌ Failed to load player_peer_32.xpm\n"); return (0); }

    printf("📂 Loading collectible_valgrind...\n");
    game->sprites.collectible_valgrind = mlx_xpm_file_to_image(game->mlx, "assets/checkbox_valgrind_32.xpm", &w, &h);
    if (!game->sprites.collectible_valgrind) { printf("❌ Failed to load checkbox_valgrind_32.xpm\n"); return (0); }

    printf("📂 Loading collectible_norminette...\n");
    game->sprites.collectible_norminette = mlx_xpm_file_to_image(game->mlx, "assets/checkbox_norminette_32.xpm", &w, &h);
    if (!game->sprites.collectible_norminette) { printf("❌ Failed to load checkbox_norminette_32.xpm\n"); return (0); }

    printf("📂 Loading collectible_tests...\n");
    game->sprites.collectible_tests = mlx_xpm_file_to_image(game->mlx, "assets/checkbox_tests_32.xpm", &w, &h);
    if (!game->sprites.collectible_tests) { printf("❌ Failed to load checkbox_tests_32.xpm\n"); return (0); }

    printf("📂 Loading exit_locked...\n");
    game->sprites.exit_locked = mlx_xpm_file_to_image(game->mlx, "assets/exit_code_review_32.xpm", &w, &h);
    if (!game->sprites.exit_locked) { printf("❌ Failed to load exit_code_review_32.xpm\n"); return (0); }

    printf("📂 Loading exit_open...\n");
    game->sprites.exit_open = mlx_xpm_file_to_image(game->mlx, "assets/exit_merged_32.xpm", &w, &h);
    if (!game->sprites.exit_open) { printf("❌ Failed to load exit_merged_32.xpm\n"); return (0); }

    printf("📂 Loading bonus_coin...\n");
    game->sprites.bonus_coin = mlx_xpm_file_to_image(game->mlx, "assets/bonus_coin_32.xpm", &w, &h);
    if (!game->sprites.bonus_coin) { printf("❌ Failed to load bonus_coin_32.xpm\n"); return (0); }

    // Load 32x32 enemy checkboxes (red)
    printf("📂 Loading enemy_norminette...\n");
    game->sprites.enemy_norminette = mlx_xpm_file_to_image(game->mlx, "assets/enemy_norminette_32.xpm", &w, &h);
    if (!game->sprites.enemy_norminette) { printf("❌ Failed to load enemy_norminette_32.xpm\n"); return (0); }

    printf("📂 Loading enemy_segfault...\n");
    game->sprites.enemy_segfault = mlx_xpm_file_to_image(game->mlx, "assets/enemy_segfault_32.xpm", &w, &h);
    if (!game->sprites.enemy_segfault) { printf("❌ Failed to load enemy_segfault_32.xpm\n"); return (0); }

    printf("📂 Loading enemy_memory_leak...\n");
    game->sprites.enemy_memory_leak = mlx_xpm_file_to_image(game->mlx, "assets/enemy_memory_leak_32.xpm", &w, &h);
    if (!game->sprites.enemy_memory_leak) { printf("❌ Failed to load enemy_memory_leak_32.xpm\n"); return (0); }

    printf("📂 Loading enemy_peer...\n");
    game->sprites.enemy_peer = mlx_xpm_file_to_image(game->mlx, "assets/enemy_peer_64.xpm", &w, &h);
    if (!game->sprites.enemy_peer) { printf("❌ Failed to load enemy_peer_64.xpm\n"); return (0); }

    printf("✅ All sprites loaded successfully\n");
    return (1);
}

int load_map(t_game *game, char *filename)
{
    int fd;
    char buffer[4000];  // Larger buffer
    int i;

    printf("📂 Loading map: %s\n", filename);

    fd = open(filename, O_RDONLY);
    if (fd < 0)
    {
        printf("❌ Cannot open file: %s\n", filename);
        return (0);
    }

    // Read entire file
    int bytes_read = read(fd, buffer, sizeof(buffer) - 1);
    close(fd);

    if (bytes_read <= 0)
    {
        printf("❌ Cannot read file or file is empty\n");
        return (0);
    }

    buffer[bytes_read] = '\0';
    printf("📄 Read %d bytes\n", bytes_read);

    // Count lines for height - FIXED
    game->map_height = 0;
    for (i = 0; buffer[i]; i++)
        if (buffer[i] == '\n')
            game->map_height++;

    // Add 1 if file doesn't end with newline
    if (i > 0 && buffer[i-1] != '\n')
        game->map_height++;

    // Get width from first line
    game->map_width = 0;
    for (i = 0; buffer[i] && buffer[i] != '\n'; i++)
        game->map_width++;

    printf("📏 Map dimensions: %dx%d\n", game->map_width, game->map_height);
    printf("🔍 Buffer ends with: '%c' (ascii %d)\n", buffer[bytes_read-1], buffer[bytes_read-1]);

    // Safety checks
    if (game->map_width >= MAX_WIDTH || game->map_height >= MAX_HEIGHT)
    {
        printf("❌ Map too large! Max: %dx%d\n", MAX_WIDTH, MAX_HEIGHT);
        return (0);
    }

    // Fill map and find player/collectibles
    int line_idx = 0;
    int char_idx = 0;
    game->collectibles = 0;
    game->collected = 0;
    game->moves = 0;

    for (i = 0; buffer[i] && line_idx < game->map_height; i++)
    {
        if (buffer[i] == '\n')
        {
            game->map[line_idx][char_idx] = '\0';
            line_idx++;
            char_idx = 0;
        }
        else if (char_idx < game->map_width)
        {
            game->map[line_idx][char_idx] = buffer[i];

            if (buffer[i] == 'P')
            {
                game->player_x = char_idx;
                game->player_y = line_idx;
                game->map[line_idx][char_idx] = '0';
                printf("👤 Player found at: (%d,%d)\n", char_idx, line_idx);
            }
            else if (buffer[i] == 'C')
            {
                game->collectibles++;
            }
            else if (buffer[i] == 'E')
            {
                printf("🚪 Exit found at: (%d,%d)\n", char_idx, line_idx);
            }

            char_idx++;
        }
    }

    printf("✅ Map parsing complete\n");
    printf("📚 Collectibles found: %d\n", game->collectibles);

    return (1);
}

int next_eval(t_game *game)
{
    char filename[50];

    // Increment eval level
    game->current_eval++;

    // Construct filename based on eval level
    if (game->current_eval == 2)
        strcpy(filename, "eval2.ber");
    else if (game->current_eval == 3)
        strcpy(filename, "eval3.ber");
    else
        return (0); // Invalid eval level

    printf("\n🎓 === STARTING EVAL %d === 🎓\n", game->current_eval);
    printf("📂 Loading: %s\n", filename);

    // Store old window dimensions for comparison
    int old_width = game->map_width;
    int old_height = game->map_height;

    // Load new map
    if (!load_map(game, filename))
    {
        printf("❌ Failed to load %s\n", filename);
        return (0);
    }

    // Check if window needs resizing
    if (game->map_width != old_width || game->map_height != old_height)
    {
        printf("🔧 Resizing window: %dx%d → %dx%d\n",
               old_width, old_height, game->map_width, game->map_height);

        // Destroy old window
        mlx_destroy_window(game->mlx, game->window);

        // Create new window with correct size
        game->window = mlx_new_window(game->mlx,
                                     game->map_width * TILE_SIZE,
                                     game->map_height * TILE_SIZE,
                                     "Escape from the Cluster");
        if (!game->window)
        {
            printf("❌ Failed to create new window\n");
            return (0);
        }

        // Re-set hooks for new window
        mlx_key_hook(game->window, key_hook, game);
        mlx_hook(game->window, 17, 0, close_game, game);
    }

    // Reset position and stats for new eval
    game->moves = 0; // Reset move counter for new eval
    game->enemy_move_counter = 0; // Reset enemy movement counter

    printf("✅ Eval %d loaded successfully!\n", game->current_eval);
    printf("📚 New requirements: %d collectibles\n", game->collectibles);
    printf("👤 Player position: (%d,%d)\n", game->player_x, game->player_y);

    // Spawn new enemies for this level
    spawn_enemies(game);

    // Re-render with new map
    render_game(game);

    return (1);
}

void draw_rectangle(t_game *game, int x, int y, int width, int height, int color)
{
    for (int i = 0; i < width; i++)
        for (int j = 0; j < height; j++)
            mlx_pixel_put(game->mlx, game->window, x + i, y + j, color);
}

void render_game(t_game *game)
{
    int x, y;

    // Clear screen
    mlx_clear_window(game->mlx, game->window);

    // Render map with SPRITES! 🎨
    for (y = 0; y < game->map_height; y++)
    {
        for (x = 0; x < game->map_width; x++)
        {
            int screen_x = x * TILE_SIZE;
            int screen_y = y * TILE_SIZE;

            // First draw floor everywhere
            mlx_put_image_to_window(game->mlx, game->window, game->sprites.floor, screen_x, screen_y);

            // Then draw specific tiles on top
            if (game->map[y][x] == '1')
            {
                mlx_put_image_to_window(game->mlx, game->window, game->sprites.wall, screen_x, screen_y);
            }
            else if (game->map[y][x] == 'C')
            {
                // Rotate collectible types based on position for variety
                int collectible_type = (x + y) % 3;
                if (collectible_type == 0)
                    mlx_put_image_to_window(game->mlx, game->window, game->sprites.collectible_valgrind, screen_x, screen_y);
                else if (collectible_type == 1)
                    mlx_put_image_to_window(game->mlx, game->window, game->sprites.collectible_norminette, screen_x, screen_y);
                else
                    mlx_put_image_to_window(game->mlx, game->window, game->sprites.collectible_tests, screen_x, screen_y);
            }
            else if (game->map[y][x] == 'E')
            {
                if (game->collected == game->collectibles)
                    mlx_put_image_to_window(game->mlx, game->window, game->sprites.exit_open, screen_x, screen_y);
                else
                    mlx_put_image_to_window(game->mlx, game->window, game->sprites.exit_locked, screen_x, screen_y);
            }
        }
    }

    // Render player with SPRITE! 🎮
    int px = game->player_x * TILE_SIZE;
    int py = game->player_y * TILE_SIZE;
    mlx_put_image_to_window(game->mlx, game->window, game->sprites.player, px, py);

    // Add text overlay for player (as suggested)
    mlx_string_put(game->mlx, game->window, px + 8, py + 20, 0xFFFFFF, "PEER");

    // Render enemies
    render_enemies(game);
}

int key_hook(int keycode, t_game *game)
{
    int new_x = game->player_x;
    int new_y = game->player_y;

    // Handle key presses - STANDARD so_long keycodes
    if (keycode == 53) // ESC
    {
        printf("🚪 ESC pressed - exiting\n");
        close_game(game);
        return (0);
    }

    // WASD movement (standard so_long)
    if (keycode == 13) // W
        new_y--;
    else if (keycode == 1) // S
        new_y++;
    else if (keycode == 0) // A
        new_x--;
    else if (keycode == 2) // D
        new_x++;
    // Arrow keys (alternative)
    else if (keycode == 126) // UP
        new_y--;
    else if (keycode == 125) // DOWN
        new_y++;
    else if (keycode == 123) // LEFT
        new_x--;
    else if (keycode == 124) // RIGHT
        new_x++;
    else
        return (0); // Ignore other keys

    // Move player if position changed
    if (new_x != game->player_x || new_y != game->player_y)
        move_player(game, new_x, new_y);

    return (0);
}

void move_player(t_game *game, int new_x, int new_y)
{
    // Check bounds
    if (new_x < 0 || new_x >= game->map_width ||
        new_y < 0 || new_y >= game->map_height)
    {
        printf("🚫 Move blocked: out of bounds\n");
        return;
    }

    // Check for walls
    if (game->map[new_y][new_x] == '1')
    {
        printf("🧱 Move blocked: wall\n");
        return;
    }

    // Check for exit
    if (game->map[new_y][new_x] == 'E')
    {
        printf("🚪 Found exit at (%d,%d)\n", new_x, new_y);
        printf("📊 Status: collected %d/%d collectibles\n", game->collected, game->collectibles);

        if (game->collected == game->collectibles)
        {
            printf("\n🎉 EVAL %d PASSED! Completed in %d moves! 🎉\n",
                   game->current_eval, game->moves + 1);

            // 3 EVAL PROGRESSION SYSTEM
            if (game->current_eval < 3)
            {
                printf("📈 Advancing to next eval...\n");
                if (next_eval(game))
                    return; // Successfully loaded next eval
                else
                {
                    printf("❌ Failed to load next eval\n");
                    close_game(game);
                    return;
                }
            }
            else
            {
                printf("\n🏆 ALL 3 EVALS COMPLETED! ESCAPED FROM THE CLUSTER! 🏆\n");
                printf("🎯 Final Score: %d/125 points\n", game->score);
                game->victory = 1;
                close_game(game);
                return;
            }
        }
        else
        {
            printf("🚫 Exit locked! Complete all eval requirements first (%d/%d)\n",
                   game->collected, game->collectibles);
            return;
        }
    }

    // Move player
    game->player_x = new_x;
    game->player_y = new_y;
    game->moves++;

    // Print moves (MANDATORY for so_long subject)
    printf("Eval %d - Moves: %d\n", game->current_eval, game->moves);

    // Check for collectible
    if (game->map[new_y][new_x] == 'C')
    {
        game->map[new_y][new_x] = '0'; // Remove collectible
        game->collected++;
        game->score += 10; // +10 points per eval requirement

        printf("✅ Eval requirement completed! (%d/%d) +10 points\n",
               game->collected, game->collectibles);

        if (game->collected == game->collectibles)
            printf("🚪 All requirements met! Exit is now open!\n");
    }

    // Move enemies only every 3 player moves for balanced gameplay
    game->enemy_move_counter++;
    if (game->enemy_move_counter >= 3)
    {
        move_enemies(game);
        game->enemy_move_counter = 0;
    }

    // Check for enemy collisions
    int i;
    for (i = 0; i < game->num_enemies; i++)
    {
        if (game->enemies[i].active &&
            game->enemies[i].x == game->player_x &&
            game->enemies[i].y == game->player_y)
        {
            // Enemy collision detected!
            printf("💀 GAME OVER! Hit by ");
            if (game->enemies[i].type == 0)
                printf("NORMINETTE error!\n");
            else if (game->enemies[i].type == 1)
                printf("SEGFAULT!\n");
            else if (game->enemies[i].type == 2)
                printf("MEMORY LEAK!\n");

            printf("🎯 Final score: %d points\n", game->score);
            close_game(game);
            return;
        }
    }

    // Re-render ONLY when needed
    render_game(game);
}

int close_game(t_game *game)
{
    // Clean up and exit
    printf("🎯 Final score: %d points\n", game->score);
    printf("👋 Thanks for playing Escape from the Cluster!\n");

    if (game->window)
        mlx_destroy_window(game->mlx, game->window);

    exit(0);
    return (0);
}

void spawn_enemies(t_game *game)
{
    int i;
    int spawn_count = 3; // 3 enemies per level

    // Clear existing enemies
    for (i = 0; i < 9; i++)
        game->enemies[i].active = 0;

    game->num_enemies = spawn_count;

    // Spawn 3 enemies: norminette, segfault, memory_leak
    for (i = 0; i < spawn_count; i++)
    {
        // Find empty spaces to spawn enemies
        int spawn_x, spawn_y;
        int attempts = 0;

        do {
            spawn_x = 1 + (rand() % (game->map_width - 2));
            spawn_y = 1 + (rand() % (game->map_height - 2));
            attempts++;
        } while ((game->map[spawn_y][spawn_x] != '0' ||
                 (spawn_x == game->player_x && spawn_y == game->player_y)) &&
                 attempts < 100);

        if (attempts < 100)
        {
            game->enemies[i].x = spawn_x;
            game->enemies[i].y = spawn_y;
            game->enemies[i].type = i % 3; // 0=norminette, 1=segfault, 2=memory_leak
            game->enemies[i].active = 1;
        }
    }
}

void move_enemies(t_game *game)
{
    int i;

    for (i = 0; i < game->num_enemies; i++)
    {
        if (!game->enemies[i].active)
            continue;

        // Simple chase AI: move towards player
        int dx = 0, dy = 0;

        if (game->enemies[i].x < game->player_x)
            dx = 1;
        else if (game->enemies[i].x > game->player_x)
            dx = -1;

        if (game->enemies[i].y < game->player_y)
            dy = 1;
        else if (game->enemies[i].y > game->player_y)
            dy = -1;

        // Try to move in preferred direction
        int new_x = game->enemies[i].x + dx;
        int new_y = game->enemies[i].y + dy;

        // Check if new position is valid (not wall, in bounds, and no other enemy there)
        int enemy_collision = 0;
        int j;
        for (j = 0; j < game->num_enemies; j++)
        {
            if (j != i && game->enemies[j].active &&
                game->enemies[j].x == new_x && game->enemies[j].y == new_y)
            {
                enemy_collision = 1;
                break;
            }
        }

        if (new_x >= 0 && new_x < game->map_width &&
            new_y >= 0 && new_y < game->map_height &&
            game->map[new_y][new_x] != '1' && !enemy_collision)
        {
            game->enemies[i].x = new_x;
            game->enemies[i].y = new_y;
        }
    }
}

void render_enemies(t_game *game)
{
    int i;

    for (i = 0; i < game->num_enemies; i++)
    {
        if (!game->enemies[i].active)
            continue;

        int screen_x = game->enemies[i].x * 32;
        int screen_y = game->enemies[i].y * 32;

        // Render based on enemy type
        if (game->enemies[i].type == 0) // norminette
        {
            mlx_put_image_to_window(game->mlx, game->window, game->sprites.enemy_norminette, screen_x, screen_y);
            mlx_string_put(game->mlx, game->window, screen_x + 2, screen_y + 20, 0xFF0000, "NORM");
        }
        else if (game->enemies[i].type == 1) // segfault
        {
            mlx_put_image_to_window(game->mlx, game->window, game->sprites.enemy_segfault, screen_x, screen_y);
            mlx_string_put(game->mlx, game->window, screen_x + 2, screen_y + 20, 0xFF0000, "SEGV");
        }
        else if (game->enemies[i].type == 2) // memory_leak
        {
            mlx_put_image_to_window(game->mlx, game->window, game->sprites.enemy_memory_leak, screen_x, screen_y);
            mlx_string_put(game->mlx, game->window, screen_x + 1, screen_y + 20, 0xFF0000, "LEAK");
        }
    }
}
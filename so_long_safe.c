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
    void    *collectible;
    void    *exit_locked;
    void    *exit_open;
    void    *enemy_norminette;
    void    *enemy_segfault;
    void    *enemy_peer;
    void    *bonus_coin;
} t_sprites;

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

    printf("📂 Loading collectible...\n");
    game->sprites.collectible = mlx_xpm_file_to_image(game->mlx, "assets/collectible_c_32.xpm", &w, &h);
    if (!game->sprites.collectible) { printf("❌ Failed to load collectible_c_32.xpm\n"); return (0); }

    printf("📂 Loading exit_locked...\n");
    game->sprites.exit_locked = mlx_xpm_file_to_image(game->mlx, "assets/exit_locked_32.xpm", &w, &h);
    if (!game->sprites.exit_locked) { printf("❌ Failed to load exit_locked_32.xpm\n"); return (0); }

    printf("📂 Loading exit_open...\n");
    game->sprites.exit_open = mlx_xpm_file_to_image(game->mlx, "assets/exit_open_32.xpm", &w, &h);
    if (!game->sprites.exit_open) { printf("❌ Failed to load exit_open_32.xpm\n"); return (0); }

    printf("📂 Loading bonus_coin...\n");
    game->sprites.bonus_coin = mlx_xpm_file_to_image(game->mlx, "assets/bonus_coin_32.xpm", &w, &h);
    if (!game->sprites.bonus_coin) { printf("❌ Failed to load bonus_coin_32.xpm\n"); return (0); }

    // Load 64x64 enemies
    printf("📂 Loading enemy_norminette...\n");
    game->sprites.enemy_norminette = mlx_xpm_file_to_image(game->mlx, "assets/enemy_norminette_64.xpm", &w, &h);
    if (!game->sprites.enemy_norminette) { printf("❌ Failed to load enemy_norminette_64.xpm\n"); return (0); }

    printf("📂 Loading enemy_segfault...\n");
    game->sprites.enemy_segfault = mlx_xpm_file_to_image(game->mlx, "assets/enemy_segfault_64.xpm", &w, &h);
    if (!game->sprites.enemy_segfault) { printf("❌ Failed to load enemy_segfault_64.xpm\n"); return (0); }

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

    printf("✅ Eval %d loaded successfully!\n", game->current_eval);
    printf("📚 New requirements: %d collectibles\n", game->collectibles);
    printf("👤 Player position: (%d,%d)\n", game->player_x, game->player_y);

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
                mlx_put_image_to_window(game->mlx, game->window, game->sprites.collectible, screen_x, screen_y);
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
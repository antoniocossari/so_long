#include "minilibx-linux/mlx.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>

#define TILE_SIZE 32
#define MAX_WIDTH 100
#define MAX_HEIGHT 100

typedef struct s_sprites
{
    void    *floor;
    void    *wall;
    void    *player;
    void    *player_walk;
    void    *collectible;
    void    *exit_closed;
    void    *exit_open;
    void    *enemy;
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
    int         game_over;
    int         game_over_reason; // 0=enemy collision, 1=completed
    t_enemy     enemies[9]; // 3 enemies per level, max 3 levels
    int         num_enemies;
    int         enemy_move_counter; // Count player moves to slow enemy movement
    int         player_anim_frame; // 0 or 1 for player animation
    int         collect_anim_x; // X position of collection animation
    int         collect_anim_y; // Y position of collection animation
    int         collect_anim_timer; // Animation timer (0 = no animation)
    t_sprites   sprites;  // Sprite assets
} t_game;

// Function prototypes
int     load_sprites(t_game *game);
void    destroy_sprites(t_game *game);
int     load_map(t_game *game, char *filename);
int     validate_map(t_game *game);
int     check_file_extension(char *filename);
int     flood_fill_check(t_game *game);
void    fatal_error(char *message);
int     next_eval(t_game *game);
void    render_game(t_game *game);
int     key_hook(int keycode, t_game *game);
int     close_game(t_game *game);
void    move_player(t_game *game, int new_x, int new_y);
void    spawn_enemies(t_game *game);
void    move_enemies(t_game *game);
void    render_enemies(t_game *game);
void    render_ui(t_game *game);
void    render_game_over_menu(t_game *game);
int     restart_game(t_game *game, char *filename);

void fatal_error(char *message)
{
    write(2, "Error\n", 6);
    write(2, message, strlen(message));
    write(2, "\n", 1);
    exit(1);
}

int check_file_extension(char *filename)
{
    int len;

    if (!filename)
        return (0);
    len = strlen(filename);
    if (len < 4)
        return (0);
    return (strcmp(filename + len - 4, ".ber") == 0);
}

void flood_fill_recursive(char map[MAX_HEIGHT][MAX_WIDTH], int x, int y, int width, int height, char visited[MAX_HEIGHT][MAX_WIDTH])
{
    // Check bounds
    if (x < 0 || x >= width || y < 0 || y >= height)
        return;

    // Check if already visited or is a wall
    if (visited[y][x] == 1 || map[y][x] == '1')
        return;

    // Mark as visited
    visited[y][x] = 1;

    // Recursively check all 4 directions
    flood_fill_recursive(map, x + 1, y, width, height, visited);
    flood_fill_recursive(map, x - 1, y, width, height, visited);
    flood_fill_recursive(map, x, y + 1, width, height, visited);
    flood_fill_recursive(map, x, y - 1, width, height, visited);
}

int flood_fill_check(t_game *game)
{
    char visited[MAX_HEIGHT][MAX_WIDTH];
    int x, y;

    // Initialize visited array to 0
    for (y = 0; y < game->map_height; y++)
        for (x = 0; x < game->map_width; x++)
            visited[y][x] = 0;

    // Start flood fill from player position
    flood_fill_recursive(game->map, game->player_x, game->player_y, game->map_width, game->map_height, visited);

    // Check if all collectibles are reachable
    for (y = 0; y < game->map_height; y++)
    {
        for (x = 0; x < game->map_width; x++)
        {
            if (game->map[y][x] == 'C' && visited[y][x] == 0)
                fatal_error("Collectible not reachable from player position");
            if (game->map[y][x] == 'E' && visited[y][x] == 0)
                fatal_error("Exit not reachable from player position");
        }
    }

    return (1);
}

int validate_map(t_game *game)
{
    int x, y;
    int player_count = 0;
    int exit_count = 0;
    int collectible_count = 0;

    // Check rectangle (all rows same length)
    for (y = 0; y < game->map_height; y++)
    {
        if ((int)strlen(game->map[y]) != game->map_width)
            fatal_error("Map is not rectangular");
    }

    // Check borders and count elements
    for (y = 0; y < game->map_height; y++)
    {
        for (x = 0; x < game->map_width; x++)
        {
            char c = game->map[y][x];

            // Check charset
            if (c != '0' && c != '1' && c != 'C' && c != 'E' && c != 'P')
                fatal_error("Invalid character in map");

            // Check borders (allow exit on borders)
            if ((y == 0 || y == game->map_height - 1 || x == 0 || x == game->map_width - 1) && c != '1' && c != 'E')
                fatal_error("Map must be surrounded by walls");

            // Count elements
            if (c == 'P')
                player_count++;
            else if (c == 'E')
                exit_count++;
            else if (c == 'C')
                collectible_count++;
        }
    }

    // Validate counts
    if (player_count != 1)
        fatal_error("Map must have exactly one player");
    if (exit_count != 1)
        fatal_error("Map must have exactly one exit");
    if (collectible_count < 1)
        fatal_error("Map must have at least one collectible");

    return (1);
}

int main(int argc, char **argv)
{
    t_game game;

    if (argc != 2)
    {
        printf("Usage: %s <map_file.ber>\n", argv[0]);
        return (1);
    }

    // Validate file extension
    if (!check_file_extension(argv[1]))
        fatal_error("File must have .ber extension");

    printf("🚀 Starting Escape from the Cluster...\n");

    // Initialize MLX
    game.mlx = mlx_init();
    if (!game.mlx)
    {
        printf("❌ Error: Failed to initialize MLX\n");
        return (1);
    }

    printf("✅ MLX initialized\n");

    // Initialize game state
    game.num_enemies = 0;
    game.enemy_move_counter = 0;
    game.player_anim_frame = 0;
    game.collect_anim_timer = 0;
    game.game_over = 0;
    game.game_over_reason = 0;

    // Initialize sprite pointers to NULL
    game.sprites.floor = NULL;
    game.sprites.wall = NULL;
    game.sprites.player = NULL;
    game.sprites.player_walk = NULL;
    game.sprites.collectible = NULL;
    game.sprites.exit_closed = NULL;
    game.sprites.exit_open = NULL;
    game.sprites.enemy = NULL;

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
    game->sprites.wall = mlx_xpm_file_to_image(game->mlx, "assets/wall_32.xpm", &w, &h);
    if (!game->sprites.wall) { printf("❌ Failed to load wall_32.xpm\n"); return (0); }

    printf("📂 Loading player...\n");
    game->sprites.player = mlx_xpm_file_to_image(game->mlx, "assets/player_peer_idle_32.xpm", &w, &h);
    if (!game->sprites.player) { printf("❌ Failed to load player_peer_idle_32.xpm\n"); return (0); }

    printf("📂 Loading player_walk...\n");
    game->sprites.player_walk = mlx_xpm_file_to_image(game->mlx, "assets/player_peer_walk_32.xpm", &w, &h);
    if (!game->sprites.player_walk) { printf("❌ Failed to load player_peer_walk_32.xpm\n"); return (0); }

    printf("📂 Loading collectible...\n");
    game->sprites.collectible = mlx_xpm_file_to_image(game->mlx, "assets/collectible_32.xpm", &w, &h);
    if (!game->sprites.collectible) { printf("❌ Failed to load collectible_32.xpm\n"); return (0); }

    printf("📂 Loading exit_closed...\n");
    game->sprites.exit_closed = mlx_xpm_file_to_image(game->mlx, "assets/exit_32.xpm", &w, &h);
    if (!game->sprites.exit_closed) { printf("❌ Failed to load exit_32.xpm\n"); return (0); }

    printf("📂 Loading exit_open...\n");
    game->sprites.exit_open = mlx_xpm_file_to_image(game->mlx, "assets/exit_open_32.xpm", &w, &h);
    if (!game->sprites.exit_open) { printf("❌ Failed to load exit_open_32.xpm\n"); return (0); }

    printf("📂 Loading enemy...\n");
    game->sprites.enemy = mlx_xpm_file_to_image(game->mlx, "assets/enemy_32.xpm", &w, &h);
    if (!game->sprites.enemy) { printf("❌ Failed to load enemy_32.xpm\n"); return (0); }


    printf("✅ All sprites loaded successfully\n");
    return (1);
}

void destroy_sprites(t_game *game)
{
    if (game->sprites.floor)
        mlx_destroy_image(game->mlx, game->sprites.floor);
    if (game->sprites.wall)
        mlx_destroy_image(game->mlx, game->sprites.wall);
    if (game->sprites.player)
        mlx_destroy_image(game->mlx, game->sprites.player);
    if (game->sprites.player_walk)
        mlx_destroy_image(game->mlx, game->sprites.player_walk);
    if (game->sprites.collectible)
        mlx_destroy_image(game->mlx, game->sprites.collectible);
    if (game->sprites.exit_closed)
        mlx_destroy_image(game->mlx, game->sprites.exit_closed);
    if (game->sprites.exit_open)
        mlx_destroy_image(game->mlx, game->sprites.exit_open);
    if (game->sprites.enemy)
        mlx_destroy_image(game->mlx, game->sprites.enemy);
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

    // Process last line if file doesn't end with newline
    if (char_idx > 0)
    {
        game->map[line_idx][char_idx] = '\0';
    }

    printf("✅ Map parsing complete\n");
    printf("📚 Collectibles found: %d\n", game->collectibles);

    // Validate map format and content
    validate_map(game);
    printf("✅ Map validation passed\n");

    // Check path connectivity with flood fill
    flood_fill_check(game);
    printf("✅ Path validation passed\n");

    // Convert player position to empty space after validation
    game->map[game->player_y][game->player_x] = '0';

    return (1);
}

int next_eval(t_game *game)
{
    char filename[50];

    // Increment eval level
    game->current_eval++;

    // Construct filename based on eval level
    if (game->current_eval == 2)
        strcpy(filename, "eval1.ber");
    else if (game->current_eval == 3)
        strcpy(filename, "eval2.ber");
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
    game->collected = 0; // Reset collected counter for new eval
    game->score = 0; // Reset score for new eval
    game->enemy_move_counter = 0; // Reset enemy movement counter
    game->player_anim_frame = 0; // Reset animation frame
    game->collect_anim_timer = 0; // Reset collection animation

    printf("✅ Eval %d loaded successfully!\n", game->current_eval);
    printf("📚 New requirements: %d collectibles\n", game->collectibles);
    printf("👤 Player position: (%d,%d)\n", game->player_x, game->player_y);

    // Spawn new enemies for this level
    spawn_enemies(game);

    // Re-render with new map
    render_game(game);

    return (1);
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
                char score_text[20];
                sprintf(score_text, "%d/100", game->score);

                if (game->collected == game->collectibles)
                {
                    mlx_put_image_to_window(game->mlx, game->window, game->sprites.exit_open, screen_x, screen_y);
                    mlx_string_put(game->mlx, game->window, screen_x + 8, screen_y - 18, 0x00FF00, "EXIT");
                    mlx_string_put(game->mlx, game->window, screen_x + 5, screen_y - 5, 0x00FF00, score_text);
                }
                else
                {
                    mlx_put_image_to_window(game->mlx, game->window, game->sprites.exit_closed, screen_x, screen_y);
                    mlx_string_put(game->mlx, game->window, screen_x + 8, screen_y - 18, 0xFFD700, "EXIT");
                    mlx_string_put(game->mlx, game->window, screen_x + 5, screen_y - 5, 0xFFD700, score_text);
                }
            }
        }
    }

    // Render player with ANIMATED SPRITE! 🎮
    int px = game->player_x * TILE_SIZE;
    int py = game->player_y * TILE_SIZE;

    // Use animated frame
    if (game->player_anim_frame == 0)
        mlx_put_image_to_window(game->mlx, game->window, game->sprites.player, px, py);
    else
        mlx_put_image_to_window(game->mlx, game->window, game->sprites.player_walk, px, py);

    // Add text overlay for player (as suggested)
    mlx_string_put(game->mlx, game->window, px + 8, py - 10, 0xFFFFFF, "PEER");

    // Render enemies
    render_enemies(game);

    // Render collection animation if active
    if (game->collect_anim_timer > 0)
    {
        int anim_x = game->collect_anim_x * TILE_SIZE;
        int anim_y = game->collect_anim_y * TILE_SIZE;

        // Create expanding yellow circle effect
        int radius = (11 - game->collect_anim_timer) * 3; // Expands as timer decreases
        int color = 0xFFD700; // Gold color

        // Draw simple expanding circle
        for (int i = -radius; i <= radius; i++)
        {
            for (int j = -radius; j <= radius; j++)
            {
                if (i*i + j*j <= radius*radius)
                {
                    int px = anim_x + 16 + i; // Center on tile
                    int py = anim_y + 16 + j;
                    if (px >= 0 && px < game->map_width * TILE_SIZE &&
                        py >= 0 && py < game->map_height * TILE_SIZE)
                    {
                        mlx_pixel_put(game->mlx, game->window, px, py, color);
                    }
                }
            }
        }

        // Decrease timer
        game->collect_anim_timer--;
    }

    // Render UI overlay
    if (game->game_over)
        render_game_over_menu(game);
    else
        render_ui(game);
}

int key_hook(int keycode, t_game *game)
{
    // Handle game over menu
    if (game->game_over)
    {
        if (keycode == 65307) // ESC
        {
            printf("🚪 ESC pressed - exiting\n");
            close_game(game);
            return (0);
        }
        else if (keycode == 114) // R - Restart
        {
            printf("🔄 Restarting game...\n");
            if (restart_game(game, "eval1.ber"))
            {
                game->game_over = 0;
                render_game(game);
            }
            return (0);
        }
        else if (keycode == 113) // Q - Quit
        {
            printf("👋 Quitting game...\n");
            close_game(game);
            return (0);
        }
        return (0); // Ignore other keys during game over
    }

    int new_x = game->player_x;
    int new_y = game->player_y;

    // Handle key presses - STANDARD so_long keycodes
    if (keycode == 65307) // ESC
    {
        printf("🚪 ESC pressed - exiting\n");
        close_game(game);
        return (0);
    }

    // WASD movement (standard so_long)
    if (keycode == 119) // W
        new_y--;
    else if (keycode == 115) // S
        new_y++;
    else if (keycode == 97) // A
        new_x--;
    else if (keycode == 100) // D
        new_x++;
    // Arrow keys (alternative)
    else if (keycode == 65362) // UP
        new_y--;
    else if (keycode == 65364) // DOWN
        new_y++;
    else if (keycode == 65361) // LEFT
        new_x--;
    else if (keycode == 65363) // RIGHT
        new_x++;
    else
        return (0); // Ignore other keys

    // Move player if position changed
    if (new_x != game->player_x || new_y != game->player_y)
    {
        // Toggle animation frame on movement
        game->player_anim_frame = (game->player_anim_frame + 1) % 2;
        move_player(game, new_x, new_y);
    }

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
            printf("\n🎉 EVAL %d PASSED! Score: %d/100 points in %d moves! 🎉\n",
                   game->current_eval, game->score, game->moves + 1);

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
                printf("🎯 Final Eval Score: %d/100 points\n", game->score);
                game->victory = 1;
                game->game_over = 1;
                game->game_over_reason = 1; // Victory
                render_game(game);
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

        // Start collection animation
        game->collect_anim_x = new_x;
        game->collect_anim_y = new_y;
        game->collect_anim_timer = 10; // Animation lasts 10 frames

        // Simple scoring: 100 points max per eval
        int points_to_add;
        if (game->collected == game->collectibles) {
            // Last collectible: award remaining points to reach exactly 100
            points_to_add = 100 - game->score;
        } else {
            // Regular collectible: award base points
            points_to_add = 100 / game->collectibles;
        }
        game->score += points_to_add;

        printf("✅ Eval requirement completed! (%d/%d) +%d points\n",
               game->collected, game->collectibles, points_to_add);

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

            printf("🎯 Eval %d score: %d/100 points\n", game->current_eval, game->score);
            game->game_over = 1;
            game->game_over_reason = 0; // Enemy collision
            render_game(game);
            return;
        }
    }

    // Re-render ONLY when needed
    render_game(game);
}

int close_game(t_game *game)
{
    // Clean up and exit
    printf("🎯 Final score: %d/100 points\n", game->score);
    printf("👋 Thanks for playing Escape from the Cluster!\n");

    // Destroy all sprites
    destroy_sprites(game);

    // Destroy window
    if (game->window)
        mlx_destroy_window(game->mlx, game->window);

    // Destroy MLX instance (Linux specific)
    if (game->mlx)
    {
        mlx_destroy_display(game->mlx);
        free(game->mlx);
    }

    exit(0);
    return (0);
}

void spawn_enemies(t_game *game)
{
    int i;
    int spawn_count = 3; // 3 enemies per level

    printf("🔄 Spawning %d enemies...\n", spawn_count);

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
            printf("👹 Enemy %d spawned at (%d,%d) type %d\n", i, spawn_x, spawn_y, game->enemies[i].type);
        }
        else
        {
            printf("❌ Failed to spawn enemy %d after %d attempts\n", i, attempts);
        }
    }
    printf("✅ Enemy spawning complete. Active enemies: %d\n", game->num_enemies);
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

        // Render enemy sprite (same for all types)
        mlx_put_image_to_window(game->mlx, game->window, game->sprites.enemy, screen_x, screen_y);

        // Render type-specific label
        if (game->enemies[i].type == 0) // norminette
        {
            mlx_string_put(game->mlx, game->window, screen_x + 2, screen_y - 10, 0xFF0000, "NORM");
        }
        else if (game->enemies[i].type == 1) // segfault
        {
            mlx_string_put(game->mlx, game->window, screen_x + 2, screen_y - 10, 0xFF0000, "SEGV");
        }
        else if (game->enemies[i].type == 2) // memory_leak
        {
            mlx_string_put(game->mlx, game->window, screen_x + 1, screen_y - 10, 0xFF0000, "LEAK");
        }
    }
}

void render_ui(t_game *game)
{
    char text[50];

    // Background color for better readability
    int ui_color = 0xFFFFFF; // White text
    int shadow_color = 0x000000; // Black shadow

    // UI block positioned over the map for better visibility
    int ui_x = 50;  // More to the right
    int ui_y = 50;  // More down
    int line_height = 15;

    // Eval progress - larger and more visible
    sprintf(text, "EVAL %d/3", game->current_eval);
    mlx_string_put(game->mlx, game->window, ui_x + 2, ui_y + 2, shadow_color, text);
    mlx_string_put(game->mlx, game->window, ui_x + 1, ui_y + 1, shadow_color, text);
    mlx_string_put(game->mlx, game->window, ui_x, ui_y, 0xFFD700, text); // Gold for better visibility
    ui_y += line_height;




    // Move counter
    sprintf(text, "MOVES: %d", game->moves);
    mlx_string_put(game->mlx, game->window, ui_x + 1, ui_y + 1, shadow_color, text);
    mlx_string_put(game->mlx, game->window, ui_x, ui_y, ui_color, text);
    ui_y += line_height;

    // Status
    if (game->collected == game->collectibles)
    {
        sprintf(text, "EXIT OPEN!");
        mlx_string_put(game->mlx, game->window, ui_x + 1, ui_y + 1, shadow_color, text);
        mlx_string_put(game->mlx, game->window, ui_x, ui_y, 0x00FF00, text); // Green
    }
    else
    {
        sprintf(text, "FIND TASKS");
        mlx_string_put(game->mlx, game->window, ui_x + 1, ui_y + 1, shadow_color, text);
        mlx_string_put(game->mlx, game->window, ui_x, ui_y, 0xFFFF00, text); // Yellow
    }
}

void render_game_over_menu(t_game *game)
{
    // Don't clear screen - overlay on existing game

    char text[100];
    int center_x = (game->map_width * TILE_SIZE) / 2;
    int center_y = (game->map_height * TILE_SIZE) / 2;

    // Background box for menu - more balanced
    for (int i = -80; i < 80; i++)
    {
        for (int j = -35; j < 35; j++)  // Smaller vertical size
        {
            if (center_x + i >= 0 && center_x + i < game->map_width * TILE_SIZE &&
                center_y + j >= 0 && center_y + j < game->map_height * TILE_SIZE)
            {
                mlx_pixel_put(game->mlx, game->window, center_x + i, center_y + j, 0x000000);
            }
        }
    }

    int menu_x = center_x - 70;
    int menu_y = center_y - 15;  // A bit more space above
    int line_height = 18;

    // Title
    if (game->game_over_reason == 1) // Victory
    {
        sprintf(text, "VICTORY!");
        mlx_string_put(game->mlx, game->window, menu_x, menu_y, 0x00FF00, text);
    }
    else // Game Over
    {
        sprintf(text, "GAME OVER");
        mlx_string_put(game->mlx, game->window, menu_x, menu_y, 0xFF0000, text);
    }

    menu_y += line_height + 5;  // Small extra space after title

    // Menu options
    sprintf(text, "R - Restart");
    mlx_string_put(game->mlx, game->window, menu_x, menu_y, 0xFFFFFF, text);
    menu_y += line_height;

    sprintf(text, "ESC/Q - Quit");
    mlx_string_put(game->mlx, game->window, menu_x, menu_y, 0xFFFFFF, text);
    menu_y += line_height;
}

int restart_game(t_game *game, char *filename)
{
    // Reset game state
    game->current_eval = 1;
    game->collected = 0;
    game->moves = 0;
    game->score = 0;
    game->victory = 0;
    game->game_over = 0;
    game->game_over_reason = 0;
    game->num_enemies = 0;
    game->enemy_move_counter = 0;
    game->player_anim_frame = 0;
    game->collect_anim_timer = 0;

    // Clear enemies
    int i;
    for (i = 0; i < 9; i++)
        game->enemies[i].active = 0;

    // Reload map
    if (!load_map(game, filename))
    {
        printf("❌ Failed to restart: Could not load map\n");
        return (0);
    }

    // Recreate window with correct dimensions for eval1
    if (game->window)
        mlx_destroy_window(game->mlx, game->window);

    game->window = mlx_new_window(game->mlx, game->map_width * TILE_SIZE,
                                  game->map_height * TILE_SIZE, "Escape from the Cluster");
    if (!game->window)
    {
        printf("❌ Failed to recreate window on restart\n");
        return (0);
    }

    // Reset hooks for new window
    mlx_key_hook(game->window, key_hook, game);
    mlx_hook(game->window, 17, 0, close_game, game);

    // Respawn enemies
    spawn_enemies(game);

    printf("🔄 Game restarted successfully!\n");
    return (1);
}
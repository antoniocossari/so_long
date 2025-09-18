#include "mlx_mac/minilibx/mlx.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>

#define TILE_SIZE 32

typedef struct s_game
{
    void    *mlx;
    void    *window;
    char    **map;
    int     map_width;
    int     map_height;
    int     player_x;
    int     player_y;
    int     collectibles;
    int     collected;
    int     moves;
    int     score;
    int     current_eval;
    int     victory;
} t_game;

// Function prototypes
int     load_map(t_game *game, char *filename);
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
        printf("Available maps: eval1.ber, eval2.ber, eval3.ber\n");
        return (1);
    }

    // Initialize MLX
    game.mlx = mlx_init();
    if (!game.mlx)
    {
        printf("Error: Failed to initialize MLX\n");
        return (1);
    }

    // Initialize game state
    game.score = 0;
    game.current_eval = 1;
    game.victory = 0;

    // Load map
    if (!load_map(&game, argv[1]))
    {
        printf("Error: Failed to load map\n");
        return (1);
    }

    // Create window
    game.window = mlx_new_window(game.mlx,
                                game.map_width * TILE_SIZE,
                                game.map_height * TILE_SIZE,
                                "Escape from the Cluster");
    if (!game.window)
    {
        printf("Error: Failed to create window\n");
        return (1);
    }

    printf("=== ESCAPE FROM THE CLUSTER ===\n");
    printf("🎓 Eval %d - 42 School Cluster\n", game.current_eval);
    printf("📍 Map: %dx%d\n", game.map_width, game.map_height);
    printf("👤 Peer at: (%d,%d)\n", game.player_x, game.player_y);
    printf("📚 Eval Requirements (C): %d\n", game.collectibles);
    printf("\n🎮 Controls: WASD | Goal: Collect all C, reach E\n\n");

    // Set hooks
    mlx_key_hook(game.window, key_hook, &game);
    mlx_hook(game.window, 17, 0, close_game, &game);

    // Render initial state
    render_game(&game);

    // Start event loop
    mlx_loop(game.mlx);

    return (0);
}

int load_map(t_game *game, char *filename)
{
    int fd;
    char buffer[2000];
    int i;

    fd = open(filename, O_RDONLY);
    if (fd < 0)
        return (0);

    // Read entire file
    int bytes_read = read(fd, buffer, sizeof(buffer) - 1);
    close(fd);

    if (bytes_read <= 0)
        return (0);

    buffer[bytes_read] = '\0';

    // Count lines for height
    game->map_height = 0;
    for (i = 0; buffer[i]; i++)
        if (buffer[i] == '\n')
            game->map_height++;

    // Get width from first line
    game->map_width = 0;
    for (i = 0; buffer[i] && buffer[i] != '\n'; i++)
        game->map_width++;

    // Allocate map
    game->map = malloc(sizeof(char*) * game->map_height);
    for (i = 0; i < game->map_height; i++)
        game->map[i] = malloc(sizeof(char) * (game->map_width + 1));

    // Fill map and find player/collectibles
    int line_idx = 0;
    int char_idx = 0;
    game->collectibles = 0;
    game->collected = 0;
    game->moves = 0;

    for (i = 0; buffer[i]; i++)
    {
        if (buffer[i] == '\n')
        {
            game->map[line_idx][char_idx] = '\0';
            line_idx++;
            char_idx = 0;
        }
        else
        {
            game->map[line_idx][char_idx] = buffer[i];

            if (buffer[i] == 'P')
            {
                game->player_x = char_idx;
                game->player_y = line_idx;
                game->map[line_idx][char_idx] = '0';
            }
            else if (buffer[i] == 'C')
            {
                game->collectibles++;
            }

            char_idx++;
        }
    }

    printf("✅ Map loaded: %dx%d, Player: (%d,%d), Collectibles: %d\n",
           game->map_width, game->map_height, game->player_x, game->player_y, game->collectibles);

    return (1);
}

void render_game(t_game *game)
{
    int x, y;
    int color;

    // Clear screen
    mlx_clear_window(game->mlx, game->window);

    // Render map (42 cluster theme)
    for (y = 0; y < game->map_height; y++)
    {
        for (x = 0; x < game->map_width; x++)
        {
            int screen_x = x * TILE_SIZE;
            int screen_y = y * TILE_SIZE;

            // 42 Cluster themed colors
            if (game->map[y][x] == '1')
                color = 0x2F2F2F; // Dark gray for desks/walls
            else if (game->map[y][x] == 'C')
                color = 0x00FF88; // Green for eval requirements
            else if (game->map[y][x] == 'E')
            {
                if (game->collected == game->collectibles)
                    color = 0x00FFFF; // Cyan if exit is open
                else
                    color = 0xFF4444; // Red if exit is locked
            }
            else
                color = 0x1A1A1A; // Very dark for cluster floor

            // Draw tile
            for (int i = 0; i < TILE_SIZE; i++)
                for (int j = 0; j < TILE_SIZE; j++)
                    mlx_pixel_put(game->mlx, game->window,
                                 screen_x + i, screen_y + j, color);
        }
    }

    // Render player (42 peer - blue circle)
    int px = game->player_x * TILE_SIZE + TILE_SIZE/2;
    int py = game->player_y * TILE_SIZE + TILE_SIZE/2;

    for (int i = -10; i <= 10; i++)
    {
        for (int j = -10; j <= 10; j++)
        {
            if (i*i + j*j <= 100) // Circle radius 10
                mlx_pixel_put(game->mlx, game->window,
                             px + i, py + j, 0x4169E1); // Royal blue peer
        }
    }
}

int key_hook(int keycode, t_game *game)
{
    int new_x = game->player_x;
    int new_y = game->player_y;

    // Handle key presses
    if (keycode == 53) // ESC
    {
        close_game(game);
        return (0);
    }
    else if (keycode == 13 || keycode == 126) // W or UP
        new_y--;
    else if (keycode == 1 || keycode == 125) // S or DOWN
        new_y++;
    else if (keycode == 0 || keycode == 123) // A or LEFT
        new_x--;
    else if (keycode == 2 || keycode == 124) // D or RIGHT
        new_x++;

    // Move player if valid
    if (new_x != game->player_x || new_y != game->player_y)
        move_player(game, new_x, new_y);

    return (0);
}

void move_player(t_game *game, int new_x, int new_y)
{
    // Check bounds
    if (new_x < 0 || new_x >= game->map_width ||
        new_y < 0 || new_y >= game->map_height)
        return;

    // Check for walls
    if (game->map[new_y][new_x] == '1')
        return;

    // Check for exit
    if (game->map[new_y][new_x] == 'E')
    {
        if (game->collected == game->collectibles)
        {
            printf("\n🎉 EVAL %d PASSED! Completed in %d moves! 🎉\n",
                   game->current_eval, game->moves + 1);
            game->victory = 1;
            close_game(game);
            return;
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
    printf("Moves: %d\n", game->moves);

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

    // Re-render
    render_game(game);
}

int close_game(t_game *game)
{
    // Clean up and exit
    if (game->window)
        mlx_destroy_window(game->mlx, game->window);

    printf("Game ended. Thanks for playing!\n");
    exit(0);
    return (0);
}
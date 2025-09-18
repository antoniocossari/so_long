#include <mlx.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/time.h>

#define TILE_SIZE 32
#define MAX_ENEMIES 10
#define MAX_BONUS 20

typedef struct s_enemy
{
    int     x;
    int     y;
    int     type;       // 0=Segfault, 1=Norminette, 2=Peer rival
    int     direction;  // 0=up, 1=right, 2=down, 3=left
    int     move_timer;
    int     active;
} t_enemy;

typedef struct s_bonus
{
    int     x;
    int     y;
    int     value;      // points to give
    int     collected;
    int     type;       // 0=Piscine Star, 1=Blackhole Avoided, 2=Peer Kudos
} t_bonus;

typedef struct s_game
{
    void        *mlx;
    void        *window;
    char        **map;
    int         map_width;
    int         map_height;
    int         player_x;
    int         player_y;
    int         collectibles;
    int         collected;
    int         moves;
    int         score;
    int         current_eval;   // 1, 2, or 3
    int         game_over;
    int         victory;

    // Enemies (runtime, not in .ber)
    t_enemy     enemies[MAX_ENEMIES];
    int         enemy_count;

    // Bonus points (overlay on '0' tiles)
    t_bonus     bonuses[MAX_BONUS];
    int         bonus_count;

    // Timing
    long        last_enemy_move;
} t_game;

// Function prototypes
long    get_time_ms(void);
int     load_map(t_game *game, char *filename);
void    init_entities(t_game *game);
void    render_game(t_game *game);
void    render_hud(t_game *game);
int     key_hook(int keycode, t_game *game);
int     close_game(t_game *game);
void    move_player(t_game *game, int new_x, int new_y);
void    update_enemies(t_game *game);
void    check_collisions(t_game *game);
void    collect_bonus(t_game *game, int x, int y);
int     next_eval(t_game *game);

long get_time_ms(void)
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (tv.tv_sec * 1000 + tv.tv_usec / 1000);
}

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
    game.game_over = 0;
    game.victory = 0;
    game.last_enemy_move = get_time_ms();

    // Load map
    if (!load_map(&game, argv[1]))
    {
        printf("Error: Failed to load map\n");
        return (1);
    }

    // Initialize simple game state
    game.enemy_count = 0;
    game.bonus_count = 0;

    // Create window
    game.window = mlx_new_window(game.mlx,
                                game.map_width * TILE_SIZE,
                                game.map_height * TILE_SIZE,
                                "So Long");
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
    printf("💰 Bonus Points Available: %d\n", game.bonus_count * 5);
    printf("\n🎮 Controls: WASD | Goal: Pass all 3 Evals | Avoid: Segfault/Norminette\n\n");

    // Set hooks
    mlx_key_hook(game.window, key_hook, &game);
    mlx_hook(game.window, 17, 0, close_game, &game); // Close button
    mlx_loop_hook(game.mlx, (int (*)(void *))render_game, &game); // Game loop

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
                game->map[line_idx][char_idx] = '0'; // Replace P with empty space
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

    // Render map
    for (y = 0; y < game->map_height; y++)
    {
        for (x = 0; x < game->map_width; x++)
        {
            int screen_x = x * TILE_SIZE;
            int screen_y = y * TILE_SIZE;

            // Choose color based on tile type
            if (game->map[y][x] == '1')
                color = 0x8B4513; // Brown for walls
            else if (game->map[y][x] == 'C')
                color = 0xFFD700; // Gold for collectibles
            else if (game->map[y][x] == 'E')
            {
                if (game->collected == game->collectibles)
                    color = 0x00FF00; // Green if exit is open
                else
                    color = 0xFF0000; // Red if exit is locked
            }
            else
                color = 0x87CEEB; // Light blue for empty space

            // Draw tile
            for (int i = 0; i < TILE_SIZE; i++)
                for (int j = 0; j < TILE_SIZE; j++)
                    mlx_pixel_put(game->mlx, game->window,
                                 screen_x + i, screen_y + j, color);
        }
    }

    // Render player (blue circle)
    int px = game->player_x * TILE_SIZE + TILE_SIZE/2;
    int py = game->player_y * TILE_SIZE + TILE_SIZE/2;

    for (int i = -8; i <= 8; i++)
    {
        for (int j = -8; j <= 8; j++)
        {
            if (i*i + j*j <= 64) // Circle radius 8
                mlx_pixel_put(game->mlx, game->window,
                             px + i, py + j, 0x0000FF);
        }
    }

    // Print status
    printf("Moves: %d | Collected: %d/%d %s\n",
           game->moves, game->collected, game->collectibles,
           (game->collected == game->collectibles) ? "- EXIT OPEN!" : "");
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
            printf("\n🎉 VICTORY! Completed in %d moves! 🎉\n", game->moves + 1);
            close_game(game);
            return;
        }
        else
        {
            printf("Exit locked! Collect all items first (%d/%d)\n",
                   game->collected, game->collectibles);
            return;
        }
    }

    // Move player
    game->player_x = new_x;
    game->player_y = new_y;
    game->moves++;

    // Check for collectible
    if (game->map[new_y][new_x] == 'C')
    {
        game->map[new_y][new_x] = '0'; // Remove collectible
        game->collected++;
        printf("💎 Collected item! (%d/%d)\n", game->collected, game->collectibles);

        if (game->collected == game->collectibles)
            printf("🚪 All items collected! Exit is now open!\n");
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
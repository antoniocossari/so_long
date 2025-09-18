#include "MLX42/include/MLX42/MLX42.h"
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include <math.h>

#define WIN_WIDTH 1000
#define WIN_HEIGHT 800
#define TILE_SIZE 40
#define AUTO_STEP_MS 600
#define RISE_MS 4000
#define SHOT_COOLDOWN_MS 300
#define START_SOLDIERS 30
#define MAX_PROJECTILES 100
#define MAX_ENEMIES 20

typedef struct s_pos {
    int x;
    int y;
} t_pos;

typedef struct s_math_gate {
    char operation;
    int value;
    int collected;
} t_math_gate;

typedef struct s_projectile {
    float x;
    float y;
    float dx;
    float dy;
    int active;
} t_projectile;

typedef struct s_enemy {
    float x;
    float y;
    float dx;
    float dy;
    int hp;
    int type;
    int active;
    int move_timer;
} t_enemy;

typedef struct s_game {
    mlx_t *mlx;
    mlx_image_t *img;
    char **map;
    int map_width;
    int map_height;
    t_pos player;
    int army_count;
    t_math_gate *math_gates;
    int collectibles_total;
    int collectibles_taken;
    int moves;
    long last_auto_step;
    long last_rise;
    long last_shot;
    int wall_level;
    t_projectile projectiles[MAX_PROJECTILES];
    t_enemy enemies[MAX_ENEMIES];
    int game_over;
    int victory;
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
    {
        for (int j = 0; j < height; j++)
        {
            put_pixel(img, x + i, y + j, color);
        }
    }
}

void draw_circle(mlx_image_t *img, int cx, int cy, int radius, uint32_t color)
{
    for (int x = cx - radius; x <= cx + radius; x++)
    {
        for (int y = cy - radius; y <= cy + radius; y++)
        {
            int dx = x - cx;
            int dy = y - cy;
            if (dx*dx + dy*dy <= radius*radius)
                put_pixel(img, x, y, color);
        }
    }
}

char *get_next_line(int fd)
{
    static char buffer[1000];
    static int buffer_pos = 0;
    static int buffer_size = 0;
    char *line = malloc(1000);
    int line_pos = 0;

    while (1)
    {
        if (buffer_pos >= buffer_size)
        {
            buffer_size = read(fd, buffer, 999);
            buffer_pos = 0;
            if (buffer_size <= 0)
            {
                if (line_pos == 0)
                {
                    free(line);
                    return NULL;
                }
                break;
            }
        }

        char c = buffer[buffer_pos++];
        line[line_pos++] = c;

        if (c == '\n')
            break;
    }

    line[line_pos] = '\0';
    return line;
}

void load_map(t_game *game, char *filename)
{
    int fd = open(filename, O_RDONLY);
    if (fd < 0)
    {
        printf("Error: Cannot open map file\n");
        exit(1);
    }

    char *line;
    char **temp_map = malloc(sizeof(char*) * 100);
    int lines = 0;
    int first_width = -1;

    while ((line = get_next_line(fd)) != NULL)
    {
        int len = strlen(line);
        if (line[len - 1] == '\n')
            line[len - 1] = '\0';

        if (first_width == -1)
            first_width = strlen(line);
        else if ((int)strlen(line) != first_width)
        {
            printf("Error: Map is not rectangular. Line %d: expected %d, got %d\n",
                   lines, first_width, (int)strlen(line));
            printf("Line content: '%s'\n", line);
            exit(1);
        }

        temp_map[lines] = line;
        lines++;
    }
    close(fd);

    game->map_height = lines;
    game->map_width = first_width;
    game->map = malloc(sizeof(char*) * lines);

    for (int i = 0; i < lines; i++)
        game->map[i] = temp_map[i];
    free(temp_map);

    game->collectibles_total = 0;
    int players = 0;
    int exits = 0;

    for (int y = 0; y < game->map_height; y++)
    {
        for (int x = 0; x < game->map_width; x++)
        {
            if (game->map[y][x] == 'P')
            {
                game->player.x = x;
                game->player.y = y;
                game->map[y][x] = '0';
                players++;
            }
            else if (game->map[y][x] == 'C')
                game->collectibles_total++;
            else if (game->map[y][x] == 'E')
                exits++;
        }
    }

    if (players != 1 || exits != 1 || game->collectibles_total == 0)
    {
        printf("Error: Invalid map format\n");
        exit(1);
    }
}

void setup_math_gates(t_game *game)
{
    game->math_gates = malloc(sizeof(t_math_gate) * game->collectibles_total);
    int gate_index = 0;

    for (int y = 0; y < game->map_height; y++)
    {
        for (int x = 0; x < game->map_width; x++)
        {
            if (game->map[y][x] == 'C')
            {
                t_math_gate *gate = &game->math_gates[gate_index];

                if (gate_index % 3 == 0)
                {
                    gate->operation = '+';
                    gate->value = 20 + (rand() % 30);
                }
                else if (gate_index % 3 == 1)
                {
                    gate->operation = '*';
                    gate->value = 2;
                }
                else
                {
                    gate->operation = '-';
                    gate->value = 5 + (rand() % 15);
                }

                gate->collected = 0;
                gate_index++;
            }
        }
    }
}

void setup_enemies(t_game *game)
{
    int enemy_count = 0;

    for (int y = 1; y < game->map_height - 1 && enemy_count < MAX_ENEMIES; y++)
    {
        for (int x = 1; x < game->map_width - 1 && enemy_count < MAX_ENEMIES; x++)
        {
            if (game->map[y][x] == '0' && rand() % 15 == 0)
            {
                t_enemy *enemy = &game->enemies[enemy_count];
                enemy->x = x * TILE_SIZE + 20;
                enemy->y = y * TILE_SIZE + 20;
                enemy->type = rand() % 3;
                enemy->hp = 1 + (enemy->type == 2 ? 1 : 0);
                enemy->active = 1;
                enemy->move_timer = 0;

                if (enemy->type == 0)
                {
                    enemy->dx = (rand() % 2 == 0) ? 1 : -1;
                    enemy->dy = 0;
                }
                else if (enemy->type == 1)
                {
                    enemy->dx = 0;
                    enemy->dy = 0;
                }
                else
                {
                    enemy->dx = 0;
                    enemy->dy = 1;
                }

                enemy_count++;
            }
        }
    }

    for (int i = enemy_count; i < MAX_ENEMIES; i++)
        game->enemies[i].active = 0;
}

void handle_collectible(t_game *game, int x, int y)
{
    int gate_index = 0;
    for (int cy = 0; cy < game->map_height; cy++)
    {
        for (int cx = 0; cx < game->map_width; cx++)
        {
            if (game->map[cy][cx] == 'C')
            {
                if (cx == x && cy == y)
                {
                    t_math_gate *gate = &game->math_gates[gate_index];
                    if (!gate->collected)
                    {
                        int old_army = game->army_count;

                        if (gate->operation == '+')
                            game->army_count += gate->value;
                        else if (gate->operation == '*')
                            game->army_count *= gate->value;
                        else if (gate->operation == '-')
                        {
                            game->army_count -= gate->value;
                            if (game->army_count < 0)
                                game->army_count = 0;
                        }

                        printf("Army: %d %c%d = %d\n", old_army, gate->operation,
                               gate->value, game->army_count);

                        gate->collected = 1;
                        game->map[y][x] = '0';
                        game->collectibles_taken++;

                        if (game->army_count <= 0)
                        {
                            game->game_over = 1;
                            printf("[MORTO] Your army was completely destroyed!\n");
                        }
                    }
                    return;
                }
                gate_index++;
            }
        }
    }
}

void shoot_projectiles(t_game *game)
{
    if (game->army_count <= 0)
        return;

    int shots = game->army_count / 15 + 1;
    if (shots > 5)
        shots = 5;

    for (int s = 0; s < shots; s++)
    {
        for (int i = 0; i < MAX_PROJECTILES; i++)
        {
            if (!game->projectiles[i].active)
            {
                game->projectiles[i].x = game->player.x * TILE_SIZE + 20;
                game->projectiles[i].y = game->player.y * TILE_SIZE + 20;

                float spread = (shots > 1) ? (s - shots/2.0) * 0.2 : 0;
                game->projectiles[i].dx = spread;
                game->projectiles[i].dy = -6;
                game->projectiles[i].active = 1;
                break;
            }
        }
    }
}

void update_game(t_game *game)
{
    long current_time = get_time_ms();

    // Auto-step
    if (current_time - game->last_auto_step >= AUTO_STEP_MS)
    {
        if (game->player.y > 0 && game->map[game->player.y - 1][game->player.x] != '1' &&
            game->player.y - 1 > game->wall_level)
        {
            game->player.y--;
            game->moves++;
            printf("Moves: %d (Auto-step)\n", game->moves);

            if (game->map[game->player.y][game->player.x] == 'C')
                handle_collectible(game, game->player.x, game->player.y);
            else if (game->map[game->player.y][game->player.x] == 'E' &&
                     game->collectibles_taken == game->collectibles_total)
            {
                game->victory = 1;
                printf("[WIN] You've reached the exit!\n");
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

    // Check wall collision
    if (game->player.y <= game->wall_level)
    {
        game->game_over = 1;
        printf("[MORTO] The wall caught you!\n");
    }

    // Update projectiles
    for (int i = 0; i < MAX_PROJECTILES; i++)
    {
        if (!game->projectiles[i].active)
            continue;

        game->projectiles[i].x += game->projectiles[i].dx;
        game->projectiles[i].y += game->projectiles[i].dy;

        if (game->projectiles[i].y < 0 || game->projectiles[i].y > WIN_HEIGHT ||
            game->projectiles[i].x < 0 || game->projectiles[i].x > WIN_WIDTH)
        {
            game->projectiles[i].active = 0;
        }
    }

    // Update enemies
    for (int i = 0; i < MAX_ENEMIES; i++)
    {
        if (!game->enemies[i].active)
            continue;

        t_enemy *enemy = &game->enemies[i];
        enemy->move_timer++;

        if (enemy->move_timer >= 20)
        {
            enemy->move_timer = 0;

            if (enemy->type == 0) // Patrol
            {
                float new_x = enemy->x + enemy->dx * 10;
                int grid_x = (int)(new_x / TILE_SIZE);
                int grid_y = (int)(enemy->y / TILE_SIZE);

                if (grid_x <= 0 || grid_x >= game->map_width - 1 ||
                    game->map[grid_y][grid_x] == '1')
                    enemy->dx = -enemy->dx;
                else
                    enemy->x = new_x;
            }
            else if (enemy->type == 1) // Chaser
            {
                int player_x = game->player.x * TILE_SIZE + 20;
                int player_y = game->player.y * TILE_SIZE + 20;

                if (fabsf(enemy->x - player_x) < 3 * TILE_SIZE &&
                    fabsf(enemy->y - player_y) < 3 * TILE_SIZE)
                {
                    if (enemy->x < player_x)
                        enemy->x += 1;
                    else if (enemy->x > player_x)
                        enemy->x -= 1;

                    if (enemy->y < player_y)
                        enemy->y += 1;
                    else if (enemy->y > player_y)
                        enemy->y -= 1;
                }
            }
            else if (enemy->type == 2) // Falling bar
            {
                enemy->y += enemy->dy * 8;
                if (enemy->y > WIN_HEIGHT)
                    enemy->active = 0;
            }
        }
    }

    // Check collisions
    for (int p = 0; p < MAX_PROJECTILES; p++)
    {
        if (!game->projectiles[p].active)
            continue;

        for (int e = 0; e < MAX_ENEMIES; e++)
        {
            if (!game->enemies[e].active)
                continue;

            float dx = game->projectiles[p].x - game->enemies[e].x;
            float dy = game->projectiles[p].y - game->enemies[e].y;
            float distance = sqrt(dx*dx + dy*dy);

            if (distance < 20)
            {
                game->enemies[e].hp--;
                game->projectiles[p].active = 0;

                if (game->enemies[e].hp <= 0)
                    game->enemies[e].active = 0;
                break;
            }
        }
    }

    // Enemy vs player
    for (int e = 0; e < MAX_ENEMIES; e++)
    {
        if (!game->enemies[e].active)
            continue;

        float dx = (game->player.x * TILE_SIZE + 20) - game->enemies[e].x;
        float dy = (game->player.y * TILE_SIZE + 20) - game->enemies[e].y;
        float distance = sqrt(dx*dx + dy*dy);

        if (distance < 30)
        {
            if (game->army_count > 0)
            {
                game->army_count -= 5;
                game->enemies[e].active = 0;
                printf("Enemy hit! Army reduced to: %d\n", game->army_count);

                if (game->army_count <= 0)
                {
                    game->game_over = 1;
                    printf("[MORTO] Your army was destroyed!\n");
                }
            }
            else
            {
                game->game_over = 1;
                printf("[MORTO] Enemy killed you!\n");
            }
        }
    }
}

void render_game(t_game *game)
{
    // Clear screen with black
    for (int y = 0; y < WIN_HEIGHT; y++)
    {
        for (int x = 0; x < WIN_WIDTH; x++)
        {
            put_pixel(game->img, x, y, 0x000000FF);
        }
    }

    // Render map
    for (int y = 0; y < game->map_height; y++)
    {
        for (int x = 0; x < game->map_width; x++)
        {
            int screen_x = x * TILE_SIZE;
            int screen_y = y * TILE_SIZE;

            if (game->map[y][x] == '1')
            {
                draw_rectangle(game->img, screen_x, screen_y, TILE_SIZE, TILE_SIZE, 0x808080FF);
            }
            else if (game->map[y][x] == 'C')
            {
                draw_rectangle(game->img, screen_x + 5, screen_y + 5, TILE_SIZE - 10, TILE_SIZE - 10, 0x00FFFFFF);
            }
            else if (game->map[y][x] == 'E')
            {
                uint32_t color = (game->collectibles_taken == game->collectibles_total) ? 0x00FF00FF : 0xFF6600FF;
                draw_rectangle(game->img, screen_x + 5, screen_y + 5, TILE_SIZE - 10, TILE_SIZE - 10, color);
            }

            // Wall overlay
            if (y <= game->wall_level)
            {
                draw_rectangle(game->img, screen_x, screen_y, TILE_SIZE, TILE_SIZE, 0x660000FF);
            }
        }
    }

    // Render player
    int px = game->player.x * TILE_SIZE + 20;
    int py = game->player.y * TILE_SIZE + 20;
    draw_circle(game->img, px, py, 15, 0x0066FFFF);

    // Render army (simplified)
    int army_display = game->army_count > 50 ? 50 : game->army_count;
    for (int i = 0; i < army_display; i++)
    {
        double angle = (2.0 * M_PI * i) / army_display;
        double radius = 25 + (i / 10) * 8;
        int sx = px + cos(angle) * radius;
        int sy = py + sin(angle) * radius;
        draw_circle(game->img, sx, sy, 2, 0x00FF00FF);
    }

    // Render projectiles
    for (int i = 0; i < MAX_PROJECTILES; i++)
    {
        if (game->projectiles[i].active)
        {
            int x = (int)game->projectiles[i].x;
            int y = (int)game->projectiles[i].y;
            draw_circle(game->img, x, y, 3, 0xFFFF00FF);
        }
    }

    // Render enemies
    for (int i = 0; i < MAX_ENEMIES; i++)
    {
        if (game->enemies[i].active)
        {
            int x = (int)game->enemies[i].x;
            int y = (int)game->enemies[i].y;

            uint32_t color;
            int size;

            if (game->enemies[i].type == 0)
            {
                color = 0xFF4444FF;
                size = 8;
            }
            else if (game->enemies[i].type == 1)
            {
                color = 0xFF8844FF;
                size = 10;
            }
            else
            {
                color = 0xFF0044FF;
                size = 12;
            }

            draw_circle(game->img, x, y, size, color);
        }
    }

    // Game over/victory overlay
    if (game->game_over)
    {
        draw_rectangle(game->img, WIN_WIDTH/2 - 100, WIN_HEIGHT/2 - 20, 200, 40, 0xFF0000FF);
    }
    else if (game->victory)
    {
        draw_rectangle(game->img, WIN_WIDTH/2 - 100, WIN_HEIGHT/2 - 20, 200, 40, 0x00FF00FF);
    }
}

void key_hook(mlx_key_data_t keydata, void *param)
{
    t_game *game = (t_game*)param;

    if (keydata.key == MLX_KEY_ESCAPE && keydata.action == MLX_PRESS)
        mlx_close_window(game->mlx);

    if (game->game_over || game->victory)
        return;

    if (keydata.action != MLX_PRESS && keydata.action != MLX_REPEAT)
        return;

    t_pos new_pos = game->player;

    if (keydata.key == MLX_KEY_A && new_pos.x > 0)
        new_pos.x--;
    else if (keydata.key == MLX_KEY_D && new_pos.x < game->map_width - 1)
        new_pos.x++;
    else if (keydata.key == MLX_KEY_W && new_pos.y > 0)
        new_pos.y--;
    else if (keydata.key == MLX_KEY_S && new_pos.y < game->map_height - 1)
        new_pos.y++;
    else if (keydata.key == MLX_KEY_SPACE)
    {
        long current_time = get_time_ms();
        if (current_time - game->last_shot >= SHOT_COOLDOWN_MS)
        {
            shoot_projectiles(game);
            game->last_shot = current_time;
        }
        return;
    }

    if (game->map[new_pos.y][new_pos.x] != '1' && new_pos.y > game->wall_level)
    {
        game->player = new_pos;
        game->moves++;
        printf("Moves: %d\n", game->moves);

        if (game->map[new_pos.y][new_pos.x] == 'C')
            handle_collectible(game, new_pos.x, new_pos.y);
        else if (game->map[new_pos.y][new_pos.x] == 'E' &&
                 game->collectibles_taken == game->collectibles_total)
        {
            game->victory = 1;
            printf("[WIN] Congratulations! You've escaped!\n");
        }
    }
}

void game_loop(void *param)
{
    t_game *game = (t_game*)param;

    if (!game->game_over && !game->victory)
        update_game(game);

    render_game(game);
}

void init_game(t_game *game, char *map_file)
{
    game->mlx = mlx_init(WIN_WIDTH, WIN_HEIGHT, "So Long Runner", true);
    if (!game->mlx)
    {
        printf("Error: Failed to initialize MLX\n");
        exit(1);
    }

    game->img = mlx_new_image(game->mlx, WIN_WIDTH, WIN_HEIGHT);
    if (!game->img)
    {
        printf("Error: Failed to create image\n");
        exit(1);
    }

    if (mlx_image_to_window(game->mlx, game->img, 0, 0) == -1)
    {
        printf("Error: Failed to put image to window\n");
        exit(1);
    }

    load_map(game, map_file);

    game->army_count = START_SOLDIERS;
    game->collectibles_taken = 0;
    game->moves = 0;
    game->game_over = 0;
    game->victory = 0;
    game->wall_level = game->map_height - 1;

    long current_time = get_time_ms();
    game->last_auto_step = current_time;
    game->last_rise = current_time;
    game->last_shot = 0;

    setup_math_gates(game);
    setup_enemies(game);

    for (int i = 0; i < MAX_PROJECTILES; i++)
        game->projectiles[i].active = 0;
}

void free_game(t_game *game)
{
    if (game->map)
    {
        for (int i = 0; i < game->map_height; i++)
            free(game->map[i]);
        free(game->map);
    }
    if (game->math_gates)
        free(game->math_gates);
}

int main(int argc, char **argv)
{
    t_game game;

    if (argc != 2)
    {
        printf("Usage: %s <map.ber>\n", argv[0]);
        return (1);
    }

    init_game(&game, argv[1]);

    mlx_key_hook(game.mlx, key_hook, &game);
    mlx_loop_hook(game.mlx, game_loop, &game);
    mlx_loop(game.mlx);

    free_game(&game);
    mlx_terminate(game.mlx);

    return (0);
}
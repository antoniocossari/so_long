#include "so_long_runner.h"

void init_game(t_game *game, char *map_file)
{
    game->mlx = mlx_init();
    game->win = mlx_new_window(game->mlx, WIN_WIDTH, WIN_HEIGHT, "So Long Runner");

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

    for (int i = 0; i < MAX_SOLDIERS_DISPLAY; i++)
    {
        if (i < game->army_count)
        {
            double angle = (2.0 * M_PI * i) / game->army_count;
            game->soldiers[i].x = game->player.x * TILE_SIZE + 20 + cos(angle) * 30;
            game->soldiers[i].y = game->player.y * TILE_SIZE + 20 + sin(angle) * 30;
            game->soldiers[i].target_x = game->soldiers[i].x;
            game->soldiers[i].target_y = game->soldiers[i].y;
            game->soldiers[i].slot = i;
        }
    }
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
            printf("Error: Map is not rectangular\n");
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
            else if (game->map[y][x] != '0' && game->map[y][x] != '1')
            {
                printf("Error: Invalid character in map\n");
                exit(1);
            }
        }
    }

    if (players != 1 || exits != 1 || game->collectibles_total == 0)
    {
        printf("Error: Invalid map format\n");
        exit(1);
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
                    gate->value = 2 + (rand() % 2);
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
            if (game->map[y][x] == '0' && rand() % 10 == 0)
            {
                t_enemy *enemy = &game->enemies[enemy_count];
                enemy->x = x * TILE_SIZE + 20;
                enemy->y = y * TILE_SIZE + 20;
                enemy->type = rand() % 3;
                enemy->hp = 1 + (enemy->type == 2 ? 2 : 0);
                enemy->active = 1;
                enemy->move_timer = 0;

                if (enemy->type == 0)
                {
                    enemy->dx = (rand() % 2 == 0) ? 0.5 : -0.5;
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
    if (game->win)
        mlx_destroy_window(game->mlx, game->win);
}
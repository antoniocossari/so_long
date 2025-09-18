#include "so_long_runner.h"

long get_time_ms(void)
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (tv.tv_sec * 1000 + tv.tv_usec / 1000);
}

int close_game(t_game *game)
{
    free_game(game);
    exit(0);
    return (0);
}

int key_press(int keycode, t_game *game)
{
    if (keycode == KEY_ESC)
        close_game(game);

    if (game->game_over || game->victory)
        return (0);

    t_pos new_pos = game->player;

    if (keycode == KEY_A && new_pos.x > 0)
        new_pos.x--;
    else if (keycode == KEY_D && new_pos.x < game->map_width - 1)
        new_pos.x++;
    else if (keycode == KEY_W && new_pos.y > 0)
        new_pos.y--;
    else if (keycode == KEY_S && new_pos.y < game->map_height - 1)
        new_pos.y++;
    else if (keycode == KEY_SPACE)
    {
        long current_time = get_time_ms();
        if (current_time - game->last_shot >= SHOT_COOLDOWN_MS)
        {
            shoot_projectiles(game);
            game->last_shot = current_time;
        }
        return (0);
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

    return (0);
}

int game_loop(t_game *game)
{
    long current_time = get_time_ms();

    if (!game->game_over && !game->victory)
    {
        update_player(game, current_time);
        update_army(game);
        update_projectiles(game);
        update_enemies(game);
        update_wall(game, current_time);
        check_collisions(game);
    }

    render_game(game);
    return (0);
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

    mlx_hook(game.win, 2, 1L<<0, key_press, &game);
    mlx_hook(game.win, 17, 1L<<17, close_game, &game);
    mlx_loop_hook(game.mlx, game_loop, &game);
    mlx_loop(game.mlx);

    return (0);
}
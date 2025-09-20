NAME = so_long_safe_linux

SRCS = so_long_safe.c

OBJS = $(SRCS:.c=.o)

CC = gcc
CFLAGS = -Wall -Wextra -Werror -g

# MinilibX flags
MLX_PATH = ./minilibx-linux
MLX_FLAGS = -L$(MLX_PATH) -lmlx -lXext -lX11 -lm

all: $(MLX_PATH)/libmlx.a $(NAME)

$(MLX_PATH)/libmlx.a:
	make -C $(MLX_PATH)

$(NAME): $(OBJS)
	$(CC) $(OBJS) $(MLX_FLAGS) -o $(NAME)

%.o: %.c
	$(CC) $(CFLAGS) -I$(MLX_PATH) -c $< -o $@

clean:
	rm -f $(OBJS)

fclean: clean
	rm -f $(NAME)

re: fclean all

test: $(NAME)
	./$(NAME) eval1.ber

.PHONY: all clean fclean re test
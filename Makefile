NAME = so_long_runner

SRCS = main_mlx42.c

OBJS = $(SRCS:.c=.o)

CC = gcc
CFLAGS = -Wall -Wextra -Werror -g

# MLX42 flags
MLX_PATH = ./MLX42
MLX_FLAGS = -L$(MLX_PATH)/build -lmlx42 -L$(MLX_PATH)/build/_deps/glfw-build/src -lglfw3 -framework OpenGL -framework Cocoa -framework IOKit

all: $(NAME)

$(NAME): $(OBJS)
	$(CC) $(OBJS) $(MLX_FLAGS) -o $(NAME)

%.o: %.c so_long_runner.h
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS)

fclean: clean
	rm -f $(NAME)

re: fclean all

test: $(NAME)
	./$(NAME) example_map.ber

.PHONY: all clean fclean re test
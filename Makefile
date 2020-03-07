CFLAGS=-Werror -Wno-format-truncation -Wno-missing-field-initializers -Wextra -Wall -O0 -g3
INCLUDES=-pthread -I./include/ `pkg-config --cflags libpq`
LIBS=-lrt -l38moths -lscrypt -luuid -lm `pkg-config --libs libpq`
NAME=logproj
COMMON_OBJ=db.o blue_midnight_wish.o parson.o views.o models.o


all: $(NAME)

clean:
	rm -f *.o
	rm -f $(NAME)

%.o: ./src/%.c
	$(CC) $(CFLAGS) $(LIB_INCLUDES) $(INCLUDES) -c $<

$(NAME): $(COMMON_OBJ) main.o
	$(CC) $(CLAGS) $(LIB_INCLUDES) $(INCLUDES) -o $(NAME) $^ $(LIBS)

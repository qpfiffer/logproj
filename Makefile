CFLAGS=-Werror -Wextra -Wall -O0 -g3
INCLUDES=-pthread -I./include/
LIBS=-lrt -l38moths -loleg-http -lscrypt -luuid -lm
NAME=logproj
COMMON_OBJ=db.o parson.o views.o models.o


all: $(NAME)

clean:
	rm -f *.o
	rm -f $(NAME)

%.o: ./src/%.c
	$(CC) $(CFLAGS) $(LIB_INCLUDES) $(INCLUDES) -c $<

$(NAME): $(COMMON_OBJ) main.o
	$(CC) $(CLAGS) $(LIB_INCLUDES) $(INCLUDES) -o $(NAME) $^ $(LIBS)

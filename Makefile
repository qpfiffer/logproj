CFLAGS?=-Werror -Wextra -Wall -O0 -g3
INCLUDES=-pthread -I/usr/local/include/ -I./include/ `pkg-config --cflags libpq`
LIBS=-lrt -l38moths -ljwt -lscrypt -lm `pkg-config --libs libpq`
NAME=logproj
COMMON_OBJ=utils.o db.o blue_midnight_wish.o parson.o api_views.o views.o models.o

DESTDIR?=
PREFIX?=/usr/local
INSTALL_BIN=$(DESTDIR)$(PREFIX)/sbin


all: $(NAME)

clean:
	rm -f *.o
	rm -f $(NAME)

%.o: ./src/%.c
	$(CC) $(CFLAGS) $(LIB_INCLUDES) $(INCLUDES) -c $<

$(NAME): $(COMMON_OBJ) main.o
	$(CC) $(CFLAGS) $(LIB_INCLUDES) $(INCLUDES) -o $(NAME) $^ $(LIBS)

install: all
	@mkdir -p $(INSTALL_BIN)
	@install $(NAME) $(INSTALL_BIN)
	@echo "logproj installed to $(DESTDIR)$(PREFIX) :^)."

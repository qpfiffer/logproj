// vim: noet ts=4 sw=4
#include <string.h>
#include <38-moths/38-moths.h>

#include "views.h"

int main_sock_fd;

static const route all_routes[] = {
	{"GET", "generic_static", "^/static/[a-zA-Z0-9/_-]*\\.[a-zA-Z]*$", 0, &static_handler, &mmap_cleanup},
	{"GET", "root_handler", "^/$", 0, &index_handler, &heap_cleanup},
};

int main(int argc, char *argv[]) {
	(void)argc;
	(void)argv;

	http_serve(&main_sock_fd, 8080, 2, all_routes, sizeof(all_routes)/sizeof(all_routes[0]));
	return 0;
}

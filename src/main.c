// vim: noet ts=4 sw=4
#include <signal.h>
#include <string.h>
#include <unistd.h>

#include <38-moths/38-moths.h>

#include "views.h"

int main_sock_fd;

void term(int signum) {
	(void)signum;
	close(main_sock_fd);
	exit(1);
}

static const m38_route all_routes[] = {
	{"POST", "api_user_login", "^/api/user/login$", 0, &api_user_login, &m38_heap_cleanup},
	{"POST", "api_user_register", "^/api/user/register$", 0, &api_user_register, &m38_heap_cleanup},
	{"POST", "api_user", "^/api/user$", 0, &api_user, &m38_heap_cleanup},
	{"POST", "api_user_projects", "^/api/user/projects$", 0, &api_user_projects, &m38_heap_cleanup},
	{"GET", "app_main", "^/app$", 0, &app_main, &m38_heap_cleanup},
	{"GET", "generic_static", "^/static/[a-zA-Z0-9/_-]*\\.[a-zA-Z]*$", 0, &static_handler, &m38_mmap_cleanup},
	{"GET", "root_handler", "^/$", 0, &index_handler, &m38_heap_cleanup},
};

int main(int argc, char *argv[]) {
	(void)argc;
	(void)argv;

	signal(SIGTERM, term);
	signal(SIGINT, term);
	signal(SIGKILL, term);
	signal(SIGCHLD, SIG_IGN);

	m38_http_serve(&main_sock_fd, 8080, 2, all_routes, sizeof(all_routes)/sizeof(all_routes[0]));
	return 0;
}

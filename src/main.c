// vim: noet ts=4 sw=4
#include <signal.h>
#include <string.h>
#include <unistd.h>

#include <38-moths/38-moths.h>

#include "db.h"
#include "api_views.h"
#include "views.h"

int main_sock_fd;

char default_db_conn_string[] = DEFAULT_DB_PG_CONNECTION_INFO;
/* This variable is set at runtime from CLI args, or the default above: */
char *db_conn_string = default_db_conn_string;

void term(int signum) {
	(void)signum;
	close(main_sock_fd);
	exit(1);
}

static const m38_route all_routes[] = {
	{"POST", "lp_api_user_login", "^/api/user/login$", 0, &lp_api_user_login, &m38_heap_cleanup},
	{"POST", "lp_api_user_register", "^/api/user/register$", 0, &lp_api_user_register, &m38_heap_cleanup},
	{"GET",  "lp_api_user", "^/api/user$", 0, &lp_api_user, &m38_heap_cleanup},
	{"POST", "lp_api_user_new_project", "^/api/user/new_project$", 0, &lp_api_user_new_project, &m38_heap_cleanup},

	{"GET",  "lp_app_logout", "^/app/logout$", 0, &lp_app_logout, &m38_heap_cleanup},
	{"GET",  "lp_app_project", "^/app/project/([a-zA-Z0-9-]+)$", 1, &lp_app_project, &m38_heap_cleanup},
	{"GET",  "lp_app_main", "^/app$", 0, &lp_app_main, &m38_heap_cleanup},
	{"GET",  "lp_app_new_project", "^/app/new_project$", 0, &lp_app_new_project, &m38_heap_cleanup},

	{"GET",  "lp_index_handler", "^/$", 0, &lp_index_handler, &m38_heap_cleanup},
	{"POST", "lp_post_index_handler", "^/$", 0, &lp_post_index_handler, &m38_heap_cleanup},
	{"GET",  "lp_generic_static", "^/static/[a-zA-Z0-9/_-]*\\.[a-zA-Z]*$", 0, &lp_static_handler, &m38_mmap_cleanup},
	{"GET",  "lp_error_page", "^/error", 0, &lp_error_page, &m38_heap_cleanup},
};

static m38_app logproj_app = {
	.main_sock_fd = &main_sock_fd,
	.port = 8666,
	.num_threads = 2,
	.routes = all_routes,
	.num_routes = sizeof(all_routes)/sizeof(all_routes[0]),
};


int main(int argc, char *argv[]) {
	signal(SIGTERM, term);
	signal(SIGINT, term);
	signal(SIGKILL, term);
	signal(SIGCHLD, SIG_IGN);

	int i;
	for (i = 1; i < argc; i++) {
		const char *cur_arg = argv[i];
		if (strncmp(cur_arg, "-d", strlen("-d")) == 0) {
			if ((i + 1) < argc) {
				db_conn_string = argv[++i];
			} else {
				m38_log_msg(LOG_ERR, "Not enough arguments to -d.");
				return -1;
			}
		}
	}

	m38_log_msg(LOG_INFO, "Using database string \"%s\".", db_conn_string);
	m38_set_404_handler(&logproj_app, &lp_404_page);
	m38_http_serve(&logproj_app);

	return 0;
}

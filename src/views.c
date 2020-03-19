// vim: noet ts=4 sw=4
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include <38-moths/parse.h>

#include "common-defs.h"
#include "db.h"
#include "models.h"
#include "parson.h"
#include "utils.h"
#include "views.h"

void _lp_setup_base_context(greshunkel_ctext *ctext, const user *current_user, bool with_projects) {
	greshunkel_ctext *user_ctext = gshkl_init_context();
	gshkl_add_string(user_ctext, "email_address", current_user->email_address);
	gshkl_add_string(user_ctext, "uuid", current_user->uuid);
	gshkl_add_sub_context(ctext, "user", user_ctext);

	if (with_projects) {
		greshunkel_var projects_array = gshkl_add_array(ctext, "PROJECTS");
		PGresult *projects_r = get_projects_for_user(current_user->uuid);
		if (projects_r) {
			int i = 0;
			for (i = 0; i < PQntuples(projects_r); i++) {
				greshunkel_ctext *project_ctext = gshkl_init_context();
				gshkl_add_string(project_ctext, "id", PQgetvalue(projects_r, i, 0));
				gshkl_add_string(project_ctext, "name", PQgetvalue(projects_r, i, 1));
				gshkl_add_sub_context_to_loop(&projects_array, project_ctext);
			}

			PQclear(projects_r);
		}
	}
}

int lp_index_handler(const m38_http_request *request, m38_http_response *response) {
	user *current_user = _lp_get_user_from_request(request);
	if (current_user) {
		m38_insert_custom_header(response,
				"Location", strlen("Location"),
				"/app", strlen("/app"));
		return 302;
	}

	greshunkel_ctext *ctext = gshkl_init_context();
	gshkl_add_array(ctext, "errors");
	free(current_user);
	return m38_render_file(ctext, "./templates/index.html", response);
}

int lp_post_index_handler(const m38_http_request *request, m38_http_response *response) {
	user *current_user = _lp_get_user_from_request(request);
	if (current_user) {
		m38_insert_custom_header(response,
				"Location", strlen("Location"),
				"/app", strlen("/app"));
		free(current_user);
		return 302;
	}

	greshunkel_ctext *ctext = gshkl_init_context();
	greshunkel_var errors_arr = gshkl_add_array(ctext, "errors");

	if (!request->form_elements) {
		gshkl_add_string_to_loop(&errors_arr, "No form submitted.");
		goto err;
	}

	char *email_address = NULL, *password = NULL;
	size_t ea_siz = 0, pw_siz = 0;

	bool should_go_to_err = false;
	email_address = sparse_dict_get(request->form_elements,
			"email_address", strlen("email_address"), &ea_siz);
	if (!email_address) {
		gshkl_add_string_to_loop(&errors_arr, "Email Address is required.");
		should_go_to_err = true;
	}

	password = sparse_dict_get(request->form_elements,
			"password", strlen("password"), &pw_siz);
	if (!password) {
		gshkl_add_string_to_loop(&errors_arr, "Password is required.");
		should_go_to_err = true;
	}

	if (should_go_to_err) {
		goto err;
	}

	return m38_render_file(ctext, "./templates/index.html", response);

err:
	return m38_render_file(ctext, "./templates/index.html", response);
}

int lp_static_handler(const m38_http_request *request, m38_http_response *response) {
	/* Remove the leading slash: */
	const char *file_path = request->resource + sizeof(char);
	return m38_mmap_file(file_path, response);
}

int lp_app_logout(const m38_http_request *request, m38_http_response *response) {
	(void)request;
	greshunkel_ctext *ctext = gshkl_init_context();

	const char buf[] = "access_token=deleted; path=/; expires=Thu, 01 Jan 1970 00:00:00 GMT";
	m38_insert_custom_header(response,
			"Set-Cookie", strlen("Set-Cookie"),
			buf, sizeof(buf));

	return m38_render_file(ctext, "./templates/logged_out.html", response);
}

int lp_app_main(const m38_http_request *request, m38_http_response *response) {
	user *current_user = _lp_get_user_from_request(request);
	if (!current_user) {
		m38_insert_custom_header(response,
				"Location", strlen("Location"),
				"/", strlen("/"));
		return 302;
	}

	greshunkel_ctext *ctext = gshkl_init_context();
	_lp_setup_base_context(ctext, current_user, true);

	free(current_user);
	return m38_render_file(ctext, "./templates/dashboard.html", response);
}

int lp_error_page(const m38_http_request *request, m38_http_response *response) {
	(void)request;
	greshunkel_ctext *ctext = gshkl_init_context();
	return m38_render_file(ctext, "./templates/error.html", response);
}

int lp_404_page(const m38_http_request *request, m38_http_response *response) {
	(void)request;
	greshunkel_ctext *ctext = gshkl_init_context();
	return m38_render_file(ctext, "./templates/404.html", response);
}

int lp_app_new_project(const m38_http_request *request, m38_http_response *response) {
	user *current_user = _lp_get_user_from_request(request);
	if (!current_user) {
		m38_insert_custom_header(response,
				"Location", strlen("Location"),
				"/", strlen("/"));
		return 302;
	}

	greshunkel_ctext *ctext = gshkl_init_context();
	_lp_setup_base_context(ctext, current_user, false);

	return m38_render_file(ctext, "./templates/new_project.html", response);
}

int lp_app_project(const m38_http_request *request, m38_http_response *response) {
	char project_uuid[64] = {0};
	strncpy(project_uuid, request->resource + request->matches[1].rm_so, sizeof(project_uuid));

	user *current_user = _lp_get_user_from_request(request);
	if (!current_user) {
		m38_insert_custom_header(response,
				"Location", strlen("Location"),
				"/", strlen("/"));
		return 302;
	}

	greshunkel_ctext *ctext = gshkl_init_context();
	_lp_setup_base_context(ctext, current_user, false);

	PGresult *projects_r = _lp_get_project(current_user->uuid, project_uuid);
	if (projects_r) {
		greshunkel_ctext *project_ctext = gshkl_init_context();
		gshkl_add_string(project_ctext, "id", PQgetvalue(projects_r, 0, 0));
		gshkl_add_string(project_ctext, "name", PQgetvalue(projects_r, 0, 1));
		gshkl_add_sub_context(ctext, "project", project_ctext);

		PQclear(projects_r);
	} else {
		free(current_user);
		return 404;
	}

	free(current_user);

	return m38_render_file(ctext, "./templates/app_project.html", response);
}

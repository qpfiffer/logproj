// vim: noet ts=4 sw=4
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include <38-moths/parse.h>
#include <jwt.h>

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

int _lp_log_user_in(
		const char email_address[static EMAIL_CHAR_SIZE],
		m38_http_response *response) {
	char *jwt = _lp_get_jwt(email_address);
	if (!jwt) {
		m38_log_msg(LOG_ERR, "Could not generate JWT for email address.");
		return 0;
	}

	char buf[512] = {0};
	snprintf(buf, sizeof(buf), "access_token=%s; HttpOnly; Path=/", jwt);
	m38_insert_custom_header(response, "Set-Cookie", strlen("Set-Cookie"), buf, strnlen(buf, sizeof(buf)));
	jwt_free_str(jwt);

	return 1;
}

int lp_post_index_handler(const m38_http_request *request, m38_http_response *response) {
	char *_email_address = NULL, *_password = NULL, *_submit = NULL;
	size_t ea_siz = 0, pw_siz = 0, submit_siz = 0;

	char *email_address = NULL;
	char *password = NULL;
	char *submit = NULL;

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

	_email_address = sparse_dict_get(request->form_elements,
			"email_address", strlen("email_address"), &ea_siz);
	if (ea_siz <= 0) {
		gshkl_add_string_to_loop(&errors_arr, "Email Address is required.");
		goto err;
	}

	email_address = strndup(_email_address, ea_siz);
	gshkl_add_string(ctext, "email_address", email_address);

	_password = sparse_dict_get(request->form_elements,
			"password", strlen("password"), &pw_siz);
	if (pw_siz <= 0) {
		gshkl_add_string_to_loop(&errors_arr, "Password is required.");
		goto err;
	}

	password = strndup(_password, pw_siz);

	_submit = sparse_dict_get(request->form_elements,
			"submit", strlen("submit"), &submit_siz);
	if (submit_siz <= 0) {
		gshkl_add_string_to_loop(&errors_arr, "You need to hit the submit button.");
		goto err;
	}
	submit = strndup(_submit, submit_siz);

	if (strncmp(submit, "Sign Up", strlen("Sign Up")) == 0) {
		/* Check DB for existing user with that email address */
		if (user_exists(email_address)) {
			gshkl_add_string_to_loop(&errors_arr, "That email address is already registered.");
			goto err;
		}

		char hash[256] = {0};
		int rc = 0;
		if ((rc = libscrypt_hash(hash, password, SCRYPT_N, SCRYPT_r, SCRYPT_p)) == 0) {
			m38_log_msg(LOG_ERR, "Could not hash password for user.");
			gshkl_add_string_to_loop(&errors_arr, "Something went wrong.");
			goto err;
		}
		//m38_log_msg(LOG_INFO, "Hash: %s", hash);

		if (!insert_user(email_address, hash)) {
			m38_log_msg(LOG_ERR, "Could not create user.");
			gshkl_add_string_to_loop(&errors_arr, "Something went wrong.");
			goto err;
		}

		if (!_lp_log_user_in(email_address, response)) {
			m38_log_msg(LOG_ERR, "Could not log user in.");
			gshkl_add_string_to_loop(&errors_arr, "Something went wrong.");
			goto err;
		}
	} else if (strncmp(submit, "Log In", strlen("Log In")) == 0) {
		char hash[SCRYPT_MCF_LEN] = {0};
		int rc = get_user_pw_hash_by_email(email_address, hash);
		if (!rc) {
			m38_log_msg(LOG_ERR, "No such user.");
			gshkl_add_string_to_loop(&errors_arr, "No such user.");
			goto err;
		}

		//m38_log_msg(LOG_INFO, "Hash: %s", hash);
		if ((rc = libscrypt_check(hash, password)) <= 0) {
			m38_log_msg(LOG_ERR, "Invalid password for user.");
			gshkl_add_string_to_loop(&errors_arr, "No such user.");
			goto err;
		}

		if (!_lp_log_user_in(email_address, response)) {
			m38_log_msg(LOG_ERR, "Could not log user in.");
			gshkl_add_string_to_loop(&errors_arr, "Something went wrong.");
			goto err;
		}
	} else {
		m38_log_msg(LOG_ERR, "Weird submit button value: %s", submit);
		gshkl_add_string_to_loop(&errors_arr, "Something went wrong.");
		goto err;
	}

	free(email_address);
	free(password);
	free(submit);

	m38_insert_custom_header(response,
			"Location", strlen("Location"),
			"/app", strlen("/app"));
	return 302;

	return m38_render_file(ctext, "./templates/index.html", response);

err:
	free(email_address);
	free(password);
	free(submit);
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

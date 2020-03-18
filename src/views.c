// vim: noet ts=4 sw=4
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include <jwt.h>
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
	free(current_user);
	return m38_render_file(ctext, "./templates/index.html", response);
}

int lp_static_handler(const m38_http_request *request, m38_http_response *response) {
	/* Remove the leading slash: */
	const char *file_path = request->resource + sizeof(char);
	return m38_mmap_file(file_path, response);
}

static int _api_failure(m38_http_response *response, const char *error) {
	JSON_Value *root_value = json_value_init_object();
	JSON_Object *root_object = parson_value_get_object(root_value);

	JSON_Value *data_value = json_value_init_object();

	json_object_set_boolean(root_object, "success", 0);
	json_object_set_string(root_object, "error", error);
	json_object_set_value(root_object, "data", data_value);

	char *output = json_serialize_to_string(root_value);
	json_value_free(root_value);

	return m38_return_raw_buffer(output, strlen(output), response);
}

static int _api_success(m38_http_response *response, JSON_Value *data_value) {
	JSON_Value *root_value = json_value_init_object();
	JSON_Object *root_object = parson_value_get_object(root_value);

	json_object_set_boolean(root_object, "success", 1);
	json_object_set_string(root_object, "error", "");
	json_object_set_value(root_object, "data", (JSON_Value *)data_value);

	char *output = json_serialize_to_string(root_value);
	json_value_free(root_value);

	return m38_return_raw_buffer(output, strlen(output), response);
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

int _lp_log_user_in(const char email_address[static EMAIL_CHAR_SIZE],
				 m38_http_response *response) {
	char *jwt = _lp_get_jwt(email_address);
	if (!jwt)
		return _api_failure(response, "Could not get JWT for email address.");

	char buf[512] = {0};
	snprintf(buf, sizeof(buf), "access_token=%s; HttpOnly; Path=/", jwt);
	m38_insert_custom_header(response, "Set-Cookie", strlen("Set-Cookie"), buf, strnlen(buf, sizeof(buf)));
	jwt_free_str(jwt);

	JSON_Value *root_value = json_value_init_object();
	JSON_Object *root_object = parson_value_get_object(root_value);
	json_object_set_boolean(root_object, "pretend_this_is_a_jwt", 1);

	return _api_success(response, root_value);
}

/* API HANDLERS */
int lp_api_user_register(const m38_http_request *request, m38_http_response *response) {
	/* Deserialize the JSON object. */
	const unsigned char *full_body = request->full_body;
	JSON_Value *body_string = json_parse_string((const char *)full_body);
	if (!body_string)
		return _api_failure(response, "Could not parse JSON object.");

	JSON_Object *new_user_object = parson_value_get_object(body_string);
	if (!new_user_object) {
		json_value_free(body_string);
		return _api_failure(response, "Could not get object from JSON.");
	}

	const char *_email_address = json_object_get_string(new_user_object, "email_address");
	const char *password = json_object_get_string(new_user_object, "password");
	if (!_email_address || !password) {
		json_value_free(body_string);
		return _api_failure(response, "Not all required fields were present.");
	}

	char email_address[EMAIL_CHAR_SIZE] = {0};
	strncpy(email_address, _email_address, sizeof(email_address));

	/* Check DB for existing user with that email address */
	if (user_exists(email_address)) {
		json_value_free(body_string);
		return _api_failure(response, "That email address is already registered.");
	}

	char hash[256] = {0};
	int rc = 0;
	if ((rc = libscrypt_hash(hash, (char *)password, SCRYPT_N, SCRYPT_r, SCRYPT_p)) == 0) {
		json_value_free(body_string);
		return _api_failure(response, "Could not hash password.");
	}
	m38_log_msg(LOG_INFO, "Hash: %s", hash);

	if (!insert_user(email_address, hash)) {
		json_value_free(body_string);
		return _api_failure(response, "Could not create user.");
	}

	json_value_free(body_string);
	return _lp_log_user_in(email_address, response);
}

int lp_api_user_login(const m38_http_request *request, m38_http_response *response) {
	/* Deserialize the JSON object. */
	const unsigned char *full_body = request->full_body;
	JSON_Value *body_string = json_parse_string((const char *)full_body);
	if (!body_string)
		return _api_failure(response, "Could not parse JSON object.");

	JSON_Object *new_user_object = parson_value_get_object(body_string);
	if (!new_user_object) {
		json_value_free(body_string);
		return _api_failure(response, "Could not get object from JSON.");
	}

	const char *_email_address = json_object_get_string(new_user_object, "email_address");
	const char *password = json_object_get_string(new_user_object, "password");

	if (!_email_address || !password) {
		json_value_free(body_string);
		return _api_failure(response, "Not all required fields were present.");
	}

	char email_address[EMAIL_CHAR_SIZE] = {0};
	strncpy(email_address, _email_address, sizeof(email_address));

	/* Check DB for existing user with that email address */
	char hash[SCRYPT_MCF_LEN] = {0};
	int rc = get_user_pw_hash_by_email(email_address, hash);
	if (!rc) {
		json_value_free(body_string);
		return _api_failure(response, "That user does not exist.");
	}

	rc = 0;
	m38_log_msg(LOG_INFO, "Hash: %s", hash);
	if ((rc = libscrypt_check(hash, (char *)password)) <= 0) {
		json_value_free(body_string);
		return _api_failure(response, "Incorrect password.");
	}

	json_value_free(body_string);

	return _lp_log_user_in(email_address, response);
}

int lp_api_user_new_project(const m38_http_request *request, m38_http_response *response) {
	user *current_user = _lp_get_user_from_request(request);
	if (!current_user) {
		return _api_failure(response, "Unauthorized.");
	}

	const unsigned char *full_body = request->full_body;
	JSON_Value *body_string = json_parse_string((const char *)full_body);
	if (!body_string) {
		char msg[] = "Could not parse JSON.";
		m38_log_msg(LOG_ERR, msg);
		return _api_failure(response, msg);
	}

	JSON_Object *new_project_object = parson_value_get_object(body_string);
	if (!new_project_object) {
		json_value_free(body_string);
		char msg[] = "Could not get object from JSON.";
		m38_log_msg(LOG_ERR, msg);
		return _api_failure(response, msg);
	}

	const char *new_project_name = json_object_get_string(new_project_object, "new_project_name");

	int rc = insert_new_project_for_user(current_user, new_project_name);
	json_value_free(body_string);

	if (!rc) {
		char msg[] = "Could not create new project.";
		m38_log_msg(LOG_ERR, msg);
		return _api_failure(response, msg);
	}

	JSON_Value *root_value = json_value_init_array();
	return _api_success(response, root_value);
}

int lp_api_user(const m38_http_request *request, m38_http_response *response) {
	user *current_user = _lp_get_user_from_request(request);
	JSON_Value *root_value = json_value_init_object();

	if (current_user) {
		JSON_Object *root_object = parson_value_get_object(root_value);
		json_object_set_string(root_object, "email_address", current_user->email_address);
		json_object_set_string(root_object, "id", current_user->uuid);
	}

	free(current_user);
	return _api_success(response, root_value);
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

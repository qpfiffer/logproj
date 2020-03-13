// vim: noet ts=4 sw=4
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include <38-moths/parse.h>
#include <jwt.h>

#include "common-defs.h"
#include "db.h"
#include "parson.h"
#include "models.h"
#include "views.h"

#define JWT_ALG JWT_ALG_RS256
#define JWT_KEY_FILENAME "./rsa256.key"
#define JWT_PUB_KEY_FILENAME "./rsa256.key.pub"

user *_lp_get_user_from_request(const m38_http_request *request) {
	/* Key stuff for RSA */
	const char key_filename[] = JWT_PUB_KEY_FILENAME;
	FILE *key_fp = NULL;
	size_t key_len = 0;
	unsigned char key[2048] = {0};

	/* JWT structures */
	jwt_t *new_jwt = NULL;
	jwt_alg_t opt_alg = JWT_ALG;
	jwt_valid_t *jwt_valid = NULL;

	/* Cookie string data from m38 */
	char *access_token = NULL;
	char *cookie_string = m38_get_header_value_request(request, "Cookie");
	if (!cookie_string)
		goto err;

	access_token = m38_get_cookie_value(cookie_string, strlen(cookie_string), "access_token");
	free(cookie_string);
	cookie_string = NULL;

	if (!access_token)
		goto err;

	key_fp = fopen(key_filename, "r");
	if (!key_fp) {
		m38_log_msg(LOG_ERR, "Could not open public key file for JWT: %s", key_filename);
		goto err;
	}

	key_len = fread(key, 1, sizeof(key), key_fp);
	fclose(key_fp);
	key[key_len] = '\0';

	int rc = jwt_valid_new(&jwt_valid, opt_alg);
	if (rc || !jwt_valid) {
		m38_log_msg(LOG_ERR, "Could not allocate new jwt verify: %i", rc);
		goto err;
	}

	jwt_valid_set_headers(jwt_valid, 1);
	jwt_valid_set_now(jwt_valid, time(NULL));

	rc = jwt_decode(&new_jwt, access_token, key, key_len);
	free(access_token);
	access_token = NULL;
	if (rc || !new_jwt) {
		m38_log_msg(LOG_ERR, "Could not decode JWT: %i", rc);
		goto err;
	}

	if (jwt_validate(new_jwt, jwt_valid) > 0) {
		m38_log_msg(LOG_ERR, "JWT failed validation: %08x", jwt_valid_get_status(jwt_valid));
		goto err;
	}

	const char *user_email = jwt_get_grant(new_jwt, "sub");
	if (!user_email) {
		m38_log_msg(LOG_ERR, "Could not get user email from JWT.");
		goto err;
	}

	user *current_user = get_user_by_email(user_email);
	if (!current_user) {
		m38_log_msg(LOG_ERR, "Could not retrieve user from DB.");
		goto err;
	}

	jwt_valid_free(jwt_valid);
	jwt_free(new_jwt);

	return current_user;

err:
	if (access_token)
		free(access_token);
	if (new_jwt)
		jwt_free(new_jwt);
	if (jwt_valid)
		jwt_valid_free(jwt_valid);
	return NULL;
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

char *_lp_get_jwt(const char *email_address) {
	if (!email_address)
		return NULL;

	jwt_t *new_jwt = NULL;
	jwt_alg_t opt_alg = JWT_ALG;

	const char key_filename[] = JWT_KEY_FILENAME;
	FILE *key_fp = NULL;
	size_t key_len = 0;
	unsigned char key[2048] = {0};

	key_fp = fopen(key_filename, "r");
	if (!key_fp) {
		m38_log_msg(LOG_ERR, "Could not open private key file for JWT: %s", key_filename);
		goto err;
	}

	key_len = fread(key, 1, sizeof(key), key_fp);
	fclose(key_fp);
	key[key_len] = '\0';

	int rc = jwt_new(&new_jwt);
	if (rc || !new_jwt) {
		m38_log_msg(LOG_ERR, "Could not create new JWT: %i", rc);
		goto err;
	}

	rc = jwt_set_alg(new_jwt, opt_alg, key, key_len);
	if (rc) {
		m38_log_msg(LOG_ERR, "Could not set algorithm for JWT.");
		goto err;
	}

	const time_t iat = time(NULL);
	rc = jwt_add_grant_int(new_jwt, "iat", iat);
	if (rc) {
		m38_log_msg(LOG_ERR, "Error adding IAT.");
		goto err;
	}

	rc = jwt_add_grant(new_jwt, "iss", "LOGPROJ");
	rc = jwt_add_grant(new_jwt, "sub", email_address);
	if (rc) {
		m38_log_msg(LOG_ERR, "Error adding claim.");
		goto err;
	}

	char *out = jwt_encode_str(new_jwt);
	jwt_free(new_jwt);

	return out;

err:
	jwt_free(new_jwt);
	return NULL;
}

int _log_user_in(const char email_address[static EMAIL_CHAR_SIZE],
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
	return _log_user_in(email_address, response);
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

	return _log_user_in(email_address, response);
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

	greshunkel_ctext *user_ctext = gshkl_init_context();
	gshkl_add_string(user_ctext, "email_address", current_user->email_address);
	gshkl_add_string(user_ctext, "uuid", current_user->uuid);
	gshkl_add_sub_context(ctext, "user", user_ctext);

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

	free(current_user);
	return m38_render_file(ctext, "./templates/dashboard.html", response);
}

int lp_error_page(const m38_http_request *request, m38_http_response *response) {
	(void)request;
	greshunkel_ctext *ctext = gshkl_init_context();
	return m38_render_file(ctext, "./templates/error.html", response);
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

	greshunkel_ctext *user_ctext = gshkl_init_context();
	gshkl_add_string(user_ctext, "email_address", current_user->email_address);
	gshkl_add_string(user_ctext, "uuid", current_user->uuid);
	gshkl_add_sub_context(ctext, "user", user_ctext);
	free(current_user);

	return m38_render_file(ctext, "./templates/new_project.html", response);
}

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

int lp_index_handler(const m38_http_request *request, m38_http_response *response) {
	char *cookie_value = m38_get_header_value_request(request, "Cookie");
	/* TODO: Redirect to app? */
	free(cookie_value);

	greshunkel_ctext *ctext = gshkl_init_context();
	return m38_render_file(ctext, "./templates/index.html", response);
}

int lp_static_handler(const m38_http_request *request, m38_http_response *response) {
	/* Remove the leading slash: */
	const char *file_path = request->resource + sizeof(char);
	return m38_mmap_file(file_path, response);
}

static int _api_failure(m38_http_response *response, const char *error) {
	JSON_Value *root_value = json_value_init_object();
	JSON_Object *root_object = json_value_get_object(root_value);

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
	JSON_Object *root_object = json_value_get_object(root_value);

	json_object_set_boolean(root_object, "success", 1);
	json_object_set_string(root_object, "error", "");
	json_object_set_value(root_object, "data", (JSON_Value *)data_value);

	char *output = json_serialize_to_string(root_value);
	json_value_free(root_value);

	return m38_return_raw_buffer(output, strlen(output), response);
}

int lp_app_logout(const m38_http_request *request, m38_http_response *response) {
	greshunkel_ctext *ctext = gshkl_init_context();

	char *cookie_string = m38_get_header_value_request(request, "Cookie");
	char *session_id = m38_get_cookie_value(cookie_string, strlen(cookie_string), "sessionid");
	free(cookie_string);

	if (!session_id)
		return lp_error_page(request, response);

	int rc = delete_sessions(session_id);
	if (!rc)
		m38_log_msg(LOG_WARN, "Could not delete session with ID '%s'", session_id);
	free(session_id);

	const char buf[] = "sessionid=deleted; path=/; expires=Thu, 01 Jan 1970 00:00:00 GMT";
	m38_insert_custom_header(response,
			"Set-Cookie", strlen("Set-Cookie"),
			buf, sizeof(buf));

	return m38_render_file(ctext, "./templates/logged_out.html", response);
}

char *_lp_get_jwt(const char *email_address) {
	if (!email_address)
		return NULL;

	jwt_t *new_jwt = NULL;
	jwt_alg_t opt_alg = JWT_ALG_RS256;

	const char key_filename[] = "./rsa256.key";
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
	if (rc != 0 || new_jwt == NULL) {
		m38_log_msg(LOG_ERR, "Could not create new JWT: %i", rc);
		goto err;
	}

	rc = jwt_set_alg(new_jwt, opt_alg, key, key_len);
	if (!rc) {
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
				 const m38_http_request *request,
				 m38_http_response *response) {
	// char uuid[UUID_CHAR_SIZE + 1] = {0};
	// int rc = insert_new_session(email_address, uuid);
	// if (!rc) {
	// 	m38_log_msg(LOG_ERR, "Could not insert new session.");
	// 	return lp_error_page(request, response);
	// }
	
	char *jwt = _lp_get_jwt(email_address);
	if (!jwt)
		return lp_error_page(request, response);

	char buf[512] = {0};
	snprintf(buf, sizeof(buf), "access_token=%s; HttpOnly; Path=/", jwt);
	m38_insert_custom_header(response, "Set-Cookie", strlen("Set-Cookie"), buf, strnlen(buf, sizeof(buf)));
	free(jwt);

	JSON_Value *root_value = json_value_init_object();
	JSON_Object *root_object = json_value_get_object(root_value);
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

	JSON_Object *new_user_object = json_value_get_object(body_string);
	if (!new_user_object) {
		json_value_free(body_string);
		return _api_failure(response, "Could not get object from JSON.");
	}

	const char *email_address = json_object_get_string(new_user_object, "email_address");
	const char *password = json_object_get_string(new_user_object, "password");

	if (!email_address || !password) {
		json_value_free(body_string);
		return _api_failure(response, "Not all required fields were present.");
	}

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
	return _log_user_in(email_address, request, response);
}

int lp_api_user_login(const m38_http_request *request, m38_http_response *response) {
	/* Deserialize the JSON object. */
	const unsigned char *full_body = request->full_body;
	JSON_Value *body_string = json_parse_string((const char *)full_body);
	if (!body_string)
		return _api_failure(response, "Could not parse JSON object.");

	JSON_Object *new_user_object = json_value_get_object(body_string);
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

	return _log_user_in(email_address, request, response);
}

int lp_api_user_projects(const m38_http_request *request, m38_http_response *response) {
	(void)request;
	JSON_Value *root_value = json_value_init_array();
	return _api_success(response, root_value);
}

int lp_api_user(const m38_http_request *request, m38_http_response *response) {
	(void)request;
	JSON_Value *root_value = json_value_init_object();
	return _api_success(response, root_value);
}

int lp_app_main(const m38_http_request *request, m38_http_response *response) {
	(void)request;
	greshunkel_ctext *ctext = gshkl_init_context();
	return m38_render_file(ctext, "./templates/dashboard.html", response);
}

int lp_error_page(const m38_http_request *request, m38_http_response *response) {
	(void)request;
	greshunkel_ctext *ctext = gshkl_init_context();
	return m38_render_file(ctext, "./templates/error.html", response);
}

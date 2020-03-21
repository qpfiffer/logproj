// vim: noet ts=4 sw=4
#include <string.h>
#include <jwt.h>

#include "api_views.h"
#include "common-defs.h"
#include "db.h"
#include "models.h"
#include "parson.h"
#include "utils.h"

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

int _lp_api_log_user_in(const char email_address[static EMAIL_CHAR_SIZE],
				 m38_http_response *response) {
	char *jwt = _lp_get_jwt(email_address);
	if (!jwt)
		return _api_failure(response, "Could not get JWT for email address.");

	char buf[512] = {0};
	snprintf(buf, sizeof(buf), "access_token=%s; HttpOnly; Path=/", jwt);
	m38_insert_custom_header(response, "Set-Cookie", strlen("Set-Cookie"), buf, strnlen(buf, sizeof(buf)));
	jwt_free_str(jwt);

	JSON_Value *root_value = json_value_init_object();
	return _api_success(response, root_value);
}

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
	//m38_log_msg(LOG_INFO, "Hash: %s", hash);

	if (!insert_user(email_address, hash)) {
		json_value_free(body_string);
		return _api_failure(response, "Could not create user.");
	}

	json_value_free(body_string);
	return _lp_api_log_user_in(email_address, response);
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

	char hash[SCRYPT_MCF_LEN] = {0};
	int rc = get_user_pw_hash_by_email(email_address, hash);
	if (!rc) {
		json_value_free(body_string);
		return _api_failure(response, "That user does not exist.");
	}

	rc = 0;
	//m38_log_msg(LOG_INFO, "Hash: %s", hash);
	if ((rc = libscrypt_check(hash, (char *)password)) <= 0) {
		json_value_free(body_string);
		return _api_failure(response, "Incorrect password.");
	}

	json_value_free(body_string);

	return _lp_api_log_user_in(email_address, response);
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


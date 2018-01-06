// vim: noet ts=4 sw=4
#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include <38-moths/parse.h>

#include "common-defs.h"
#include "db.h"
#include "parson.h"
#include "models.h"
#include "views.h"

int index_handler(const m38_http_request *request, m38_http_response *response) {
	char *cookie_value = m38_get_header_value_request(request, "Cookie");
	free(cookie_value);

	greshunkel_ctext *ctext = gshkl_init_context();
	return m38_render_file(ctext, "./templates/index.html", response);
}

int static_handler(const m38_http_request *request, m38_http_response *response) {
	/* Remove the leading slash: */
	const char *file_path = request->resource + sizeof(char);
	return m38_mmap_file(file_path, response);
}

static int _api_failure(m38_http_response *response, greshunkel_ctext *ctext, const char *error) {
	gshkl_add_string(ctext, "SUCCESS", "false");
	gshkl_add_string(ctext, "ERROR", error);
	gshkl_add_string(ctext, "DATA", "{}");
	return m38_render_file(ctext, "./templates/response.json", response);
}
int _log_user_in(const char user_key[static MAX_KEY_SIZE], const greshunkel_ctext *ctext, m38_http_response *response) {
	/* Creates a new session object for the specified user, and returns the right Set-Cookie
	 * headers.
	 */
	/* We could also avoid a round-trip here. I don't care right now though. BREAK OLEG! */
	char uuid[UUID_CHAR_SIZE + 1] = {0};
	insert_session(user_key, uuid);

	char buf[128] = {0};
	/* UUID has some null chars in it or something */
	snprintf(buf, sizeof(buf), "sessionid=%s; HttpOnly; Path=/", uuid);
	m38_insert_custom_header(response, "Set-Cookie", strlen("Set-Cookie"), buf, strnlen(buf, sizeof(buf)));

	return m38_render_file(ctext, "./templates/response.json", response);
}

/* API HANDLERS */
int api_create_user(const m38_http_request *request, m38_http_response *response) {
	greshunkel_ctext *ctext = gshkl_init_context();

	/* Deserialize the JSON object. */
	const unsigned char *full_body = request->full_body;
	JSON_Value *body_string = json_parse_string((const char *)full_body);
	if (!body_string)
		return _api_failure(response, ctext, "Could not parse JSON object.");

	JSON_Object *new_user_object = json_value_get_object(body_string);
	if (!new_user_object) {
		json_value_free(body_string);
		return _api_failure(response, ctext, "Could not get object from JSON.");
	}

	const char *email_address = json_object_get_string(new_user_object, "email_address");
	const char *password = json_object_get_string(new_user_object, "password");

	if (!email_address || !password) {
		json_value_free(body_string);
		return _api_failure(response, ctext, "Not all required fields were present..");
	}

	/* Check DB for existing user with that email address */
	char user_key[MAX_KEY_SIZE] = {0};
	if (get_user(email_address, user_key)) {
		json_value_free(body_string);
		return _api_failure(response, ctext, "That email address is already registered.");
	}

	char hash[256] = {0};
	int rc = 0;
	if ((rc = libscrypt_hash(hash, (char *)password, SCRYPT_N, SCRYPT_r, SCRYPT_p)) == 0) {
		json_value_free(body_string);
		return _api_failure(response, ctext, "Could not hash password.");
	}
	m38_log_msg(LOG_INFO, "Hash: %s", hash);

	if (!insert_user(email_address, hash)) {
		json_value_free(body_string);
		return _api_failure(response, ctext, "Could not create user.");
	}

	json_value_free(body_string);
	gshkl_add_string(ctext, "SUCCESS", "true");
	gshkl_add_string(ctext, "ERROR", "[]");
	gshkl_add_string(ctext, "DATA", "{}");

	return _log_user_in(user_key, ctext, response);
}

int api_login_user(const m38_http_request *request, m38_http_response *response) {
	greshunkel_ctext *ctext = gshkl_init_context();

	/* Deserialize the JSON object. */
	const unsigned char *full_body = request->full_body;
	JSON_Value *body_string = json_parse_string((const char *)full_body);
	if (!body_string)
		return _api_failure(response, ctext, "Could not parse JSON object.");

	JSON_Object *new_user_object = json_value_get_object(body_string);
	if (!new_user_object) {
		json_value_free(body_string);
		return _api_failure(response, ctext, "Could not get object from JSON.");
	}

	const char *email_address = json_object_get_string(new_user_object, "email_address");
	const char *password = json_object_get_string(new_user_object, "password");

	if (!email_address || !password) {
		json_value_free(body_string);
		return _api_failure(response, ctext, "Not all required fields were present..");
	}

	/* Check DB for existing user with that email address */
	char user_key[MAX_KEY_SIZE] = {0};
	user *user = get_user(email_address, user_key);
	if (!user) {
		json_value_free(body_string);
		return _api_failure(response, ctext, "That user does not exist.");
	}

	char hash[SCRYPT_MCF_LEN] = {0};
	strncpy(hash, user->password, sizeof(hash));
	int rc = 0;
	m38_log_msg(LOG_INFO, "Hash: %s", hash);
	if ((rc = libscrypt_check(hash, (char *)password)) <= 0) {
		json_value_free(body_string);
		return _api_failure(response, ctext, "Incorrect password.");
	}

	json_value_free(body_string);
	gshkl_add_string(ctext, "SUCCESS", "true");
	gshkl_add_string(ctext, "ERROR", "[]");
	gshkl_add_string(ctext, "DATA", "{}");
	return _log_user_in(user_key, ctext, response);
}

int app_main(const m38_http_request *request, m38_http_response *response) {
	(void)request;
	greshunkel_ctext *ctext = gshkl_init_context();
	return m38_render_file(ctext, "./templates/app.html", response);
}

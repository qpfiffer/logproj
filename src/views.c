// vim: noet ts=4 sw=4
#include <stddef.h>

#include "common-defs.h"
#include "db.h"
#include "parson.h"
#include "views.h"

int index_handler(const http_request *request, http_response *response) {
	(void)request;
	greshunkel_ctext *ctext = gshkl_init_context();
	return render_file(ctext, "./templates/index.html", response);
}

int static_handler(const http_request *request, http_response *response) {
	/* Remove the leading slash: */
	const char *file_path = request->resource + sizeof(char);
	return mmap_file(file_path, response);
}

static int _api_failure(http_response *response, greshunkel_ctext *ctext, const char *error) {
	gshkl_add_string(ctext, "SUCCESS", "false");
	gshkl_add_string(ctext, "ERROR", error);
	gshkl_add_string(ctext, "DATA", "{}");
	return render_file(ctext, "./templates/response.json", response);
}

/* API HANDLERS */
int api_create_user(const http_request *request, http_response *response) {
	greshunkel_ctext *ctext = gshkl_init_context();

	/* Deserialize the JSON object. */
	const unsigned char *full_body = request->full_body;
	JSON_Value *body_string = json_parse_string((const char *)full_body);
	if (!body_string)
		return _api_failure(response, ctext, "Bad JSON.");

	JSON_Object *new_user_object = json_value_get_object(body_string);
	if (!new_user_object) {
		json_value_free(body_string);
		return _api_failure(response, ctext, "Bad JSON.");
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

	char hash[SCRYPT_MCF_LEN] = {0};
	if (!libscrypt_hash(hash, (char *)password, SCRYPT_N, SCRYPT_r, SCRYPT_p)) {
		json_value_free(body_string);
		return _api_failure(response, ctext, "Could not hash password.");
	}

	if (!insert_user(email_address, password)) {
		json_value_free(body_string);
		return _api_failure(response, ctext, "Could not create user.");
	}

	json_value_free(body_string);
	gshkl_add_string(ctext, "SUCCESS", "true");
	gshkl_add_string(ctext, "ERROR", "[]");
	gshkl_add_string(ctext, "DATA", "{}");
	return render_file(ctext, "./templates/response.json", response);
}

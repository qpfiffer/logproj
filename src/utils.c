// vim: noet ts=4 sw=4
#include <string.h>
#include <jwt.h>
#include <libpq-fe.h>

#include <38-moths/38-moths.h>

#include "db.h"
#include "models.h"
#include "utils.h"

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

	user *current_user = _lp_get_user_by_email(user_email);
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


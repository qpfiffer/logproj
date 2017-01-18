// vim: noet ts=4 sw=4
#include <stdio.h>
#include <sys/time.h>
#include <string.h>
#include <uuid/uuid.h>

#include <38-moths/logging.h>

#include "db.h"
#include "models.h"

user *get_user(const char email_address[static 128], char out_key[static MAX_KEY_SIZE]) {
	create_user_key(email_address, out_key);

	size_t json_size = 0;
	char *json = (char *)fetch_data_from_db(&oleg_conn, out_key, &json_size);
	/* log_msg(LOG_INFO, "Json from DB: %s", json); */

	if (json == NULL)
		return NULL;

	user *deserialized = deserialize_user(json);
	free(json);
	return deserialized;
}

int set_user(const user *user) {
	char key[MAX_KEY_SIZE] = {0};
	create_user_key(user->email_address, key);

	char *serialized = serialize_user(user);
	log_msg(LOG_INFO, "Serialized user: %s", serialized);

	int ret = store_data_in_db(&oleg_conn, key, (unsigned char *)serialized, strlen(serialized));
	free(serialized);

	return ret;
}

int insert_user(const char email_address[static 128], const char password[static 128]) {
	user to_insert = {
		.email_address = {0},
		.password = {0},
		.created_at = 0,
	};
	time(&to_insert.created_at);

	memcpy(to_insert.email_address, email_address, sizeof(to_insert.email_address));
	memcpy(to_insert.password, password, sizeof(to_insert.password));

	return set_user(&to_insert);
}

session *get_session(const char uuid[static UUID_CHAR_SIZE], char out_key[static MAX_KEY_SIZE]) {
	/* XXX: This should be resilient against namespace traversal
	 * attacks. Don't blindly trust that what we get back is an actual user.
	 */
	create_session_key(uuid, out_key);

	size_t json_size = 0;
	char *json = (char *)fetch_data_from_db(&oleg_conn, out_key, &json_size);
	/* log_msg(LOG_INFO, "Json from DB: %s", json); */

	if (json == NULL)
		return NULL;

	session *deserialized = deserialize_session(json);
	free(json);

	if (deserialized == NULL)
		return NULL;

	return deserialized;
}

int set_session(const struct session *session) {
	char key[MAX_KEY_SIZE] = {0};
	create_session_key(session->user_key, key);

	char *serialized = serialize_session(session);
	log_msg(LOG_INFO, "Serialized session: %s", serialized);

	int ret = store_data_in_db(&oleg_conn, key, (unsigned char *)serialized, strlen(serialized));
	free(serialized);

	return ret;
}

int insert_session(const char user_key[static MAX_KEY_SIZE]) {
	uuid_t uuid_raw = {0};
	char uuid_str[UUID_CHAR_SIZE] = {0};

	/* Generate a new UUID. */
	uuid_generate(uuid_raw);
	uuid_unparse_upper(uuid_raw, uuid_str);

	session to_insert = {
		.uuid = {0},
		.user_key = {0},
	};

	memcpy(to_insert.uuid, uuid_str, sizeof(to_insert.uuid));
	memcpy(to_insert.user_key, user_key, sizeof(to_insert.user_key));

	return set_session(&to_insert);
}

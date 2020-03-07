// vim: noet ts=4 sw=4
#include <stdio.h>
#include <string.h>

#include "db.h"
#include "models.h"
#include "parson.h"

void create_user_key(const char email_address[static EMAIL_CHAR_SIZE], char outbuf[static MAX_KEY_SIZE]) {
	snprintf(outbuf, MAX_KEY_SIZE, "%s%c%s", USER_NMSPC, SEPERATOR, email_address);
}

char *serialize_user(const user *to_serialize) {
	if (!to_serialize)
		return NULL;

	JSON_Value *root_value = json_value_init_object();
	JSON_Object *root_object = json_value_get_object(root_value);

	char *serialized_string = NULL;

	json_object_set_string(root_object, "email_address", to_serialize->email_address);
	json_object_set_string(root_object, "password", to_serialize->password);
	json_object_set_number(root_object, "created_at", to_serialize->created_at);

	serialized_string = json_serialize_to_string(root_value);
	json_value_free(root_value);
	return serialized_string;
}

user *deserialize_user_from_tuples(const PGresult *res, const unsigned int i) {
	if (!res)
		return NULL;

	if (PQntuples(res) <= 0) {
		m38_log_msg(LOG_WARN, "No tuples in PG result for deserialize_user_from_tuples.");
		return NULL;
	}

	user *to_return = calloc(1, sizeof(user));

	const int id_col = PQfnumber(res, "id");
	const int created_at_col = PQfnumber(res, "created_at");
	const int email_address_col = PQfnumber(res, "email_address");
	const int email_address_col = PQfnumber(res, "email_address");

	strncpy(to_return->file_hash, PQgetvalue(res, i, file_hash_col), sizeof(to_return->file_hash));
	to_return->size = (size_t)atol(PQgetvalue(res, i, size_col));
	to_return->post_id = (unsigned int)atol(PQgetvalue(res, i, post_id_col));
	to_return->created_at = (time_t)atol(PQgetvalue(res, i, created_at_col));
	to_return->id = (unsigned int)atol(PQgetvalue(res, i, id_col));


	return to_return;
}

user *deserialize_user(const char *json) {
	if (!json)
		return NULL;

	user *to_return = calloc(1, sizeof(user));

	JSON_Value *serialized = json_parse_string(json);
	JSON_Object *user_object = json_value_get_object(serialized);

	strncpy(to_return->email_address, json_object_get_string(user_object, "email_address"), sizeof(to_return->email_address));
	strncpy(to_return->password, json_object_get_string(user_object, "password"), sizeof(to_return->password));

	to_return->created_at = (time_t)json_object_get_number(user_object, "created_at");

	json_value_free(serialized);
	return to_return;
}

static unsigned int x_count(const char prefix[static MAX_KEY_SIZE]) {
	unsigned int num = fetch_num_matches_from_db(&oleg_conn, prefix);
	return num;
}

unsigned int user_count() {
	char prefix[MAX_KEY_SIZE] = USER_NMSPC;
	return x_count(prefix);
}

void create_session_key(const char uuid[static UUID_CHAR_SIZE], char outbuf[static MAX_KEY_SIZE]) {
	snprintf(outbuf, MAX_KEY_SIZE, "%s%c%s", SESSION_NMSPC, SEPERATOR, uuid);
}

char *serialize_session(const session *to_serialize) {
	if (!to_serialize)
		return NULL;

	JSON_Value *root_value = json_value_init_object();
	JSON_Object *root_object = json_value_get_object(root_value);

	char *serialized_string = NULL;

	json_object_set_string(root_object, "uuid", to_serialize->uuid);
	json_object_set_string(root_object, "user_key", to_serialize->user_key);
	//json_object_set_string(root_object, "data", to_serialize->data);
	//json_object_set_number(root_object, "dlen", to_serialize->dlen);

	serialized_string = json_serialize_to_string(root_value);
	json_value_free(root_value);
	return serialized_string;
}

session *deserialize_session(const char *json) {
	if (!json)
		return NULL;

	session *to_return = calloc(1, sizeof(session));

	JSON_Value *serialized = json_parse_string(json);
	JSON_Object *session_object = json_value_get_object(serialized);

	strncpy(to_return->uuid, json_object_get_string(session_object, "uuid"), sizeof(to_return->uuid));
	strncpy(to_return->user_key, json_object_get_string(session_object, "user_key"), sizeof(to_return->user_key));

	json_value_free(serialized);
	return to_return;
}

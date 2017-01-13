// vim: noet ts=4 sw=4
#include <stdio.h>
#include <string.h>

#include "db.h"
#include "models.h"
#include "parson.h"

void create_user_key(const char email_address[static 128], char outbuf[static MAX_KEY_SIZE]) {
	snprintf(outbuf, MAX_KEY_SIZE, "%s%c%s", USER_NMSPC, SEPERATOR, email_address);
}

char *serialize_user(const user *to_serialize) {
	if (!to_serialize)
		return NULL;

	JSON_Value *root_value = json_value_init_object();
	JSON_Object *root_object = json_value_get_object(root_value);

	char *serialized_string = NULL;

	json_object_set_string(root_object, "email_address", to_serialize->email_address);
	json_object_set_string(root_object, "password", to_serialize->email_address);
	json_object_set_number(root_object, "created_at", to_serialize->created_at);

	serialized_string = json_serialize_to_string(root_value);
	json_value_free(root_value);
	return serialized_string;
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

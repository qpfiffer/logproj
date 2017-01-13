// vim: noet ts=4 sw=4
#include <stdio.h>
#include <sys/time.h>
#include <string.h>

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

int set_image(const user *user) {
	char key[MAX_KEY_SIZE] = {0};
	create_user_key(user->email_address, key);

	char *serialized = serialize_user(user);
	log_msg(LOG_INFO, "Serialized user: %s", serialized);

	int ret = store_data_in_db(&oleg_conn, key, (unsigned char *)serialized, strlen(serialized));
	free(serialized);

	return ret;
}

int insert_user(const char email_address[static 128], const char password[static 128]) {
	(void)email_address;
	(void)password;

	return 0;
}

/*
static int insert(const char *file_path, const char filename[static MAX_IMAGE_FILENAME_SIZE], 
						const char image_hash[static HASH_IMAGE_STR_SIZE], const char board[static MAX_BOARD_NAME_SIZE],
						const char post_key[MAX_KEY_SIZE]) {
	time_t modified_time = get_file_creation_date(file_path);
	if (modified_time == 0) {
		log_msg(LOG_ERR, "IWMT: '%s' does not exist.", file_path);
		return 0;
	}

	size_t size = get_file_size(file_path);
	if (size == 0) {
		log_msg(LOG_ERR, "IWFS: '%s' does not exist.", file_path);
		return 0;
	}

	webm to_insert = {
		.file_hash = {0},
		.filename = {0},
		.board = {0},
		.created_at = modified_time,
		.size = size,
		.post = {0}
	};
	memcpy(to_insert.file_hash, image_hash, sizeof(to_insert.file_hash));
	memcpy(to_insert.file_path, file_path, sizeof(to_insert.file_path));
	memcpy(to_insert.filename, filename, sizeof(to_insert.filename));
	memcpy(to_insert.board, board, sizeof(to_insert.board));
	memcpy(to_insert.post, post_key, sizeof(to_insert.post));

	return set_image(&to_insert);
}
*/

// vim: noet ts=4 sw=4
#pragma once
#include <time.h>
#include "common-defs.h"

#include <38-moths/vector.h>

/* The unsigned chars in the struct are used to null terminate the
 * strings while still allowing us to use 'sizeof(webm.file_hash)'.
 */
typedef struct user {
	char email_address[128];
	unsigned char _null_term_hax_1;

	time_t created_at;
	size_t size;
} __attribute__((__packed__)) user;

void create_user_key(const char email_address[static 128], char outbuf[static MAX_KEY_SIZE]);
char *serialize_user(const user *to_serialize);
user *deserialize_user(const char *json);
unsigned int user_count();

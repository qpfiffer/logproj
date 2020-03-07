// vim: noet ts=4 sw=4
#pragma once
#include <time.h>

#include <38-moths/vector.h>

#include "common-defs.h"

/* The unsigned chars in the struct are used to null terminate the
 * strings while still allowing us to use 'sizeof(webm.file_hash)'.
 */
typedef struct user {
	char uuid[UUID_CHAR_SIZE];
	unsigned char _null_term_hax_1;

	char email_address[EMAIL_CHAR_SIZE];
	unsigned char _null_term_hax_2;

	char password[SCRYPT_MCF_LEN];
	unsigned char _null_term_hax_3;

	time_t created_at;
} __attribute__((__packed__)) user;

void create_user_key(const char email_address[static EMAIL_CHAR_SIZE], char outbuf[static MAX_KEY_SIZE]);
char *serialize_user(const user *to_serialize);
user *deserialize_user_from_tuples(const PGresult *res, const unsigned int i);
unsigned int user_count();

/* An API key is just that: A key. */
typedef struct api_key {
	/* Unique identifier. */
	char uuid[UUID_CHAR_SIZE];
	unsigned char _null_term_hax_1;

	/* Foreign key to the logged in user. */
	char user_key[MAX_KEY_SIZE];
	unsigned char _null_term_hax_2;

	/* Name of the key */
	char name[MAX_KEY_SIZE];
	unsigned char _null_term_hax_3;

	time_t created_at;
} api_key;

void create_api_key_key(const char uuid[static 36], char outbuf[static MAX_KEY_SIZE]);
char *serialize_api_key(const api_key *to_serialize);
api_key *deserialize_api_key(const char *json);

/* A session stores arbitrary data. */
typedef struct session {
	/* Unique identifier. */
	char uuid[UUID_CHAR_SIZE];
	unsigned char _null_term_hax_1;

	/* Foreign key to the logged in user. */
	char user_key[MAX_KEY_SIZE];
	unsigned char _null_term_hax_2;

	/* Arbitrary data. */
	/* char *data;
	size_t dlen;
	*/
} session;

void create_session_key(const char uuid[static 36], char outbuf[static MAX_KEY_SIZE]);
char *serialize_session(const session *to_serialize);
session *deserialize_session(const char *json);

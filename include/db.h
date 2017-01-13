// vim: noet ts=4 sw=4
#pragma once
#include <oleg-http/oleg-http.h>

#include "common-defs.h"

#define DB_HOST "localhost"
#define DB_PORT "38080"

/* Global oleg connection for this project. */
static const struct db_conn oleg_conn = {
	.host = DB_HOST,
	.port = DB_PORT,
	.db_name = LOGPROJ_NMSPC
};

/* User get/set methods */
struct user *get_user(const char email_address[static 128], char out_key[static MAX_KEY_SIZE]);
int set_user(const struct user *user);
int insert_user(const char email_address[static 128], const char password[static SCRYPT_MCF_LEN]);

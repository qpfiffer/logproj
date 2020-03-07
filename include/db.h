// vim: noet ts=4 sw=4
#pragma once
#include <libpq-fe.h>
#include "common-defs.h"

#define DB_PG_CONNECTION_INFO "postgresql:///logproj"

/* User get/set methods */
struct user *get_user_by_email(const char email_address[static EMAIL_CHAR_SIZE]);
int set_user(const struct user *user);
int insert_user(const char email_address[static EMAIL_CHAR_SIZE], const char password[static SCRYPT_MCF_LEN]);

/* Sessions */
struct session *get_session(const char uuid[static UUID_CHAR_SIZE], char out_key[static MAX_KEY_SIZE]);
int set_session(const struct session *session);
int insert_session(const char user_key[static MAX_KEY_SIZE], char out_uuid[static UUID_CHAR_SIZE]);

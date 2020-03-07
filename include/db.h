// vim: noet ts=4 sw=4
#pragma once
#include <libpq-fe.h>
#include "common-defs.h"

#define DB_PG_CONNECTION_INFO "postgresql:///logproj"

/* User get/set methods */
int user_exists(const char email_address[static EMAIL_CHAR_SIZE]);
struct user *get_user_by_email(const char email_address[static EMAIL_CHAR_SIZE]);
int get_user_pw_hash_by_email(const char email_address[static EMAIL_CHAR_SIZE], char out_hash[static SCRYPT_MCF_LEN]);
int set_user(const struct user *user);
int insert_user(const char email_address[static EMAIL_CHAR_SIZE], const char password[static SCRYPT_MCF_LEN]);

/* Sessions */
struct session *get_session(const char uuid[static UUID_CHAR_SIZE], char out_key[static MAX_KEY_SIZE]);
int set_session(const struct session *session);
int insert_new_session(const char email_address[static EMAIL_CHAR_SIZE], char out_uuid[static UUID_CHAR_SIZE]);
int delete_sessions(const char session_id[static UUID_CHAR_SIZE]);

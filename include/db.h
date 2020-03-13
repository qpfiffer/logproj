// vim: noet ts=4 sw=4
#pragma once
#include <libpq-fe.h>
#include "common-defs.h"

#define DEFAULT_DB_PG_CONNECTION_INFO "postgresql:///logproj"

extern char default_db_conn_string[];
/* This variable is set at runtime from CLI args, or the default above: */
extern char *db_conn_string;

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

PGresult *get_projects_for_user(const char *user_id);
int insert_new_project_for_user(const struct user *user, const char new_project_name[static 1]);

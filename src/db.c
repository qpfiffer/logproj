// vim: noet ts=4 sw=4
#include <stdio.h>
#include <sys/time.h>
#include <string.h>
#include <uuid/uuid.h>

#include <38-moths/logging.h>

#include "db.h"
#include "models.h"

static PGconn *_get_pg_connection() {
	PGconn *conn = PQconnectdb(DB_PG_CONNECTION_INFO);

	if (PQstatus(conn) != CONNECTION_OK) {
		m38_log_msg(LOG_ERR, "Could not connect to Postgres: %s", PQerrorMessage(conn));
		return NULL;
	}

	return conn;
}

static void _finish_pg_connection(PGconn *conn) {
	if (conn)
		PQfinish(conn);
}

static PGresult *_generic_command(const char *query_command) {
	PGresult *res = NULL;
	PGconn *conn = NULL;

	conn = _get_pg_connection();
	if (!conn)
		goto error;

	res = PQexec(conn, query_command);

	if (PQresultStatus(res) != PGRES_TUPLES_OK) {
		m38_log_msg(LOG_ERR, "SELECT failed: %s", PQerrorMessage(conn));
		goto error;
	}

	_finish_pg_connection(conn);

	return res;

error:
	if (res)
		PQclear(res);
	_finish_pg_connection(conn);
	return NULL;
}

user *get_user_by_email(const char email_address[static EMAIL_CHAR_SIZE]) {
	PGresult *res = NULL;
	PGconn *conn = NULL;

	const char *param_values[] = {email_address};

	conn = _get_pg_connection();
	if (!conn)
		goto error;

	res = PQexecParams(conn,
					  "SELECT * "
					  "FROM logproj.user "
					  "WHERE email_address = $1;",
					  1,
					  NULL,
					  param_values,
					  NULL,
					  NULL,
					  0);

	if (PQresultStatus(res) != PGRES_TUPLES_OK) {
		m38_log_msg(LOG_ERR, "SELECT failed: %s", PQerrorMessage(conn));
		goto error;
	}

	user *deserialized = deserialize_user_from_tuples(res, 0);
	if (!deserialized)
		goto error;

	PQclear(res);
	_finish_pg_connection(conn);

	return deserialized;

error:
	if (res)
		PQclear(res);
	_finish_pg_connection(conn);
	return NULL;
}

int set_user(const user *user) {
	char key[MAX_KEY_SIZE] = {0};
	create_user_key(user->email_address, key);

	char *serialized = serialize_user(user);
	m38_log_msg(LOG_INFO, "Serialized user: %s", serialized);

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
	/* m38_log_msg(LOG_INFO, "Json from DB: %s", json); */

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
	m38_log_msg(LOG_INFO, "Serialized session: %s", serialized);

	int ret = store_data_in_db(&oleg_conn, key, (unsigned char *)serialized, strlen(serialized));
	free(serialized);

	return ret;
}

int insert_session(const char user_key[static MAX_KEY_SIZE], char out_uuid[static UUID_CHAR_SIZE]) {
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
	strncpy(out_uuid, uuid_str, sizeof(to_insert.uuid));
	memcpy(to_insert.user_key, user_key, sizeof(to_insert.user_key));


	return set_session(&to_insert);
}

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

/*
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
*/

int user_exists(const char email_address[static EMAIL_CHAR_SIZE]) {
	PGresult *res = NULL;
	PGconn *conn = NULL;

	const char *param_values[] = {email_address};
	const char buf[] = "SELECT count(*) "
					   "FROM logproj.user "
					   "WHERE email_address = $1;";

	conn = _get_pg_connection();
	if (!conn)
		goto error;

	res = PQexecParams(conn,
					  buf,
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

	const int count = (int)atol(PQgetvalue(res, 0, 0));

	PQclear(res);
	_finish_pg_connection(conn);

	return count;

error:
	if (res)
		PQclear(res);
	_finish_pg_connection(conn);
	return 0;
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
	PGresult *res = NULL;
	PGconn *conn = NULL;

	const char *param_values[] = {user->email_address, user->password};

	conn = _get_pg_connection();
	if (!conn)
		goto error;

	res = PQexecParams(conn,
					  "INSERT INTO logproj.user (email_address, password) "
					  "VALUES ($1, $2) "
					  "RETURNING id;",
					  2,
					  NULL,
					  param_values,
					  NULL,
					  NULL,
					  0);

	if (PQresultStatus(res) != PGRES_TUPLES_OK) {
		m38_log_msg(LOG_ERR, "SELECT failed: %s", PQerrorMessage(conn));
		goto error;
	}

	PQclear(res);
	_finish_pg_connection(conn);

	return 1;

error:
	if (res)
		PQclear(res);
	_finish_pg_connection(conn);
	return 0;
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

	char *json = NULL;
	//char *json = (char *)fetch_data_from_db(&oleg_conn, out_key, &json_size);
	/* m38_log_msg(LOG_INFO, "Json from DB: %s", json); */

	if (json == NULL)
		return NULL;

	session *deserialized = deserialize_session(json);
	free(json);

	if (deserialized == NULL)
		return NULL;

	return deserialized;
}

int get_user_pw_hash_by_email(const char email_address[static EMAIL_CHAR_SIZE], char out_hash[static SCRYPT_MCF_LEN]) {
	PGresult *res = NULL;
	PGconn *conn = NULL;

	const char *param_values[] = {email_address};

	conn = _get_pg_connection();
	if (!conn)
		goto error;

	res = PQexecParams(conn,
					  "SELECT password "
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

	const size_t siz = SCRYPT_MCF_LEN;
	strncpy(out_hash, PQgetvalue(res, 0, 0), siz);

	PQclear(res);
	_finish_pg_connection(conn);

	return 1;

error:
	if (res)
		PQclear(res);
	_finish_pg_connection(conn);
	return 0;
}

int insert_new_session(const char email_address[static EMAIL_CHAR_SIZE], char out_session_uuid[static UUID_CHAR_SIZE]) {
	PGresult *res = NULL;
	PGconn *conn = NULL;

	const char *param_values[] = {email_address};

	conn = _get_pg_connection();
	if (!conn)
		goto error;

	res = PQexecParams(conn,
					  "INSERT INTO logproj.session (user_id) "
					  "VALUES ((SELECT id FROM logproj.user WHERE email_address = $1)) "
					  "RETURNING id;",
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

	const size_t siz = UUID_CHAR_SIZE + 1;
	strncpy(out_session_uuid, PQgetvalue(res, 0, 0), siz);

	PQclear(res);
	_finish_pg_connection(conn);

	return 1;

error:
	if (res)
		PQclear(res);
	_finish_pg_connection(conn);
	return 0;
}

int delete_sessions(const char session_id[static UUID_CHAR_SIZE]) {
	PGresult *res = NULL;
	PGconn *conn = NULL;

	const char *param_values[] = {session_id};

	conn = _get_pg_connection();
	if (!conn)
		goto error;

	
	res = PQexecParams(conn,
					  "DELETE FROM logproj.session"
					  "WHERE session_id = $1;",
					  1,
					  NULL,
					  param_values,
					  NULL,
					  NULL,
					  0);

	if (PQresultStatus(res) != PGRES_TUPLES_OK) {
		m38_log_msg(LOG_ERR, "DELETE failed: %s", PQerrorMessage(conn));
		goto error;
	}

	PQclear(res);
	_finish_pg_connection(conn);

	return 1;

error:
	if (res)
		PQclear(res);
	_finish_pg_connection(conn);
	return 0;
}

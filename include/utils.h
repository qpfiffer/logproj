// vim: noet ts=4 sw=4
#pragma once
#include <38-moths/38-moths.h>

#define JWT_ALG JWT_ALG_RS256
#define JWT_KEY_FILENAME "./rsa256.key"
#define JWT_PUB_KEY_FILENAME "./rsa256.key.pub"

struct user;

/* Fetches a user from the database based on the request object. */
struct user *_lp_get_user_from_request(const m38_http_request *request);

/* Generates a new JWT for a given email address. */
char *_lp_get_jwt(const char *email_address);

char *get_from_form_values(struct sparse_dict *dict,
		const char *value, const size_t vlen, size_t *outsiz);

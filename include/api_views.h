// vim: noet ts=4 sw=4
#pragma once
#include <38-moths/38-moths.h>

/* API handlers */
int lp_api_user(const m38_http_request *request, m38_http_response *response);
int lp_api_user_login(const m38_http_request *request, m38_http_response *response);
int lp_api_user_new_project(const m38_http_request *request, m38_http_response *response);
int lp_api_user_register(const m38_http_request *request, m38_http_response *response);

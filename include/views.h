// vim: noet ts=4 sw=4
#pragma once
#include <38-moths/38-moths.h>

int static_handler(const m38_http_request *request, m38_http_response *response);
int index_handler(const m38_http_request *request, m38_http_response *response);

/* Main logged in view */
int app_main(const m38_http_request *request, m38_http_response *response);

/* API handlers */
int api_user_register(const m38_http_request *request, m38_http_response *response);
int api_user_login(const m38_http_request *request, m38_http_response *response);
int api_user(const m38_http_request *request, m38_http_response *response);
int api_user_projects(const m38_http_request *request, m38_http_response *response);

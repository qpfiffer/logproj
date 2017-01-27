// vim: noet ts=4 sw=4
#pragma once
#include <38-moths/38-moths.h>

int static_handler(const http_request *request, http_response *response);
int index_handler(const http_request *request, http_response *response);

/* Main logged in view */
int app_main(const http_request *request, http_response *response);

/* API handlers */
int api_create_user(const http_request *request, http_response *response);
int api_login_user(const http_request *request, http_response *response);

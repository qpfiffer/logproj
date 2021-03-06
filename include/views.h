// vim: noet ts=4 sw=4
#pragma once
#include <38-moths/38-moths.h>

int lp_static_handler(const m38_http_request *request, m38_http_response *response);
int lp_index_handler(const m38_http_request *request, m38_http_response *response);
int lp_post_index_handler(const m38_http_request *request, m38_http_response *response);

/* Main logged in view */
int lp_app_logout(const m38_http_request *request, m38_http_response *response);
int lp_app_new_project(const m38_http_request *request, m38_http_response *response);
int lp_app_post_new_project(const m38_http_request *request, m38_http_response *response);
int lp_app_project(const m38_http_request *request, m38_http_response *response);
int lp_app_project_keys(const m38_http_request *request, m38_http_response *response);
int lp_app_main(const m38_http_request *request, m38_http_response *response);
int lp_error_page(const m38_http_request *request, m38_http_response *response);
int lp_404_page(const m38_http_request *request, m38_http_response *response);

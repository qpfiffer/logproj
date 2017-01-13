// vim: noet ts=4 sw=4
#include "views.h"

int index_handler(const http_request *request, http_response *response) {
	(void)request;
	greshunkel_ctext *ctext = gshkl_init_context();
	return render_file(ctext, "./templates/index.html", response);
}

int static_handler(const http_request *request, http_response *response) {
	/* Remove the leading slash: */
	const char *file_path = request->resource + sizeof(char);
	return mmap_file(file_path, response);
}


/* API HANDLERS */
int api_create_user(const http_request *request, http_response *response) {
	(void)request;

	/* 1. Check DB for user
	 * 2. Use libscrypt_hash
	 * 3. Insert user
	 * 4. Store result
	 */

	greshunkel_ctext *ctext = gshkl_init_context();
	gshkl_add_string(ctext, "SUCCESS", "true");
	gshkl_add_string(ctext, "ERROR", "[]");
	gshkl_add_string(ctext, "DATA", "{}");
	return render_file(ctext, "./templates/response.json", response);
}

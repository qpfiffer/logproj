// vim: noet ts=4 sw=4
#include "views.h"

int index_handler(const http_request *request, http_response *response) {
	(void)request;
	greshunkel_ctext *ctext = gshkl_init_context();
	return render_file(ctext, "./templates/index.html", response);
}

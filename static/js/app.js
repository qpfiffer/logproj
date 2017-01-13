// vim: noet ts=4 sw=4:
var app = null;

function create_user(email_address, password) {
}

function main() {
	app = new Vue({
		el: '#app',
		data: {
			new_user: {
				email_address: '',
				password: ''
			},
			new_user_errors: []
		},
		methods: {
			add_user: function() {
				var data = { "email_address": this.new_user.email_address,
							 "password": this.new_user.password }
				this.$http.post('/api/users').then(function(response) {
					this.new_user.name = '';
					this.new_user.email = '';
				}, function(response) {
					this.new_user_errors = response.data.errors;
				});
			}
		}
	})
}

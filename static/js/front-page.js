// vim: noet ts=4 sw=4:
var app = null;

function main() {
	app = new Vue({
		el: '#app',
		data: {
			new_user: {
				email_address: '',
				password: ''
			},
			new_user_error: ""
		},
		methods: {
			login: function() {
				var data = { "email_address": this.new_user.email_address,
							 "password": this.new_user.password }
				this.$http.post('/api/user_login.json', data).then(function(response) {
					if (response.body.success == true) {
						this.new_user.name = '';
						this.new_user.email = '';
						this.new_user_error = null;
					} else {
						this.new_user_error = response.body.error;
					}
				}, function(response) {
					this.new_user_error = "Something went wrong.";
				});
			},
			add_user: function() {
				var data = { "email_address": this.new_user.email_address,
							 "password": this.new_user.password }
				this.$http.post('/api/user_create.json', data).then(function(response) {
					if (response.body.success == true) {
						this.new_user.name = '';
						this.new_user.email = '';
						this.new_user_error = null;
						window.location.href = "/app";
					} else {
						this.new_user_error = response.body.error;
					}
				}, function(response) {
					this.new_user_error = "Something went wrong.";
				});
			}
		}
	})
}

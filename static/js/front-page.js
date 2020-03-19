// vim: set ts=2 sw=2 noet :
var store = {
	state: {
		new_user: {
			email_address: '',
			password: ''
		},
		errors: [],
		new_user_error: ""
	}
}

const app = new Vue({
	el: '#app',
	data: {
		sharedState: store,
	},
	methods: {
		_auth: function (url) {
			this.sharedState.state.errors = [];
			this.sharedState.state.new_user_error = null;

			if (!this.sharedState.state.new_user.email_address) {
				this.sharedState.state.errors.push('Email address required.');
				return
			}

			if (!this.sharedState.state.new_user.password) {
				this.sharedState.state.errors.push('Password required.');
				return
			}

			var data = {'email_address': this.sharedState.state.new_user.email_address,
									'password': this.sharedState.state.new_user.password }
			postData(url, data).then((response) => {
				if (response.success == true) {
					this.sharedState.state.new_user.name = '';
					this.sharedState.state.new_user.email = '';
					this.sharedState.state.new_user_error = null;
					window.location.href = '/app';
				} else {
					this.sharedState.state.new_user_error = response.error;
				}
			}, (response) => {
				this.sharedState.state.new_user_error = 'Something went wrong.';
			});
		},
		login: function() {
			return this._auth('/api/user/login');
		},
		register: function() {
			return this._auth('/api/user/register');
		}
	}
})

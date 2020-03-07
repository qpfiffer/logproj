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

const postData = async function(url, data) {
	const response = await fetch(url, {
		method: 'POST',
		headers: { 'Content-Type': 'application/json' },
		body: JSON.stringify(data),
	});

	return await response.json();
}

const app = new Vue({
	el: '#app',
	data: {
		sharedState: store,
	},
	methods: {
		login: function() {
			this.sharedState.state.errors = [];

			if (!this.sharedState.state.new_user.email_address) {
				this.errors.push('Email address required.');
				return
			}

			if (!this.sharedState.state.new_user.password) {
				this.errors.push('Password required.');
				return
			}

			var data = {	'email_address': this.sharedState.state.new_user.email_address,
										'password': this.sharedState.state.new_user.password }
			postData('/api/user/login', data).then((response) => {
				if (response.success == true) {
					this.sharedState.state.new_user.name = '';
					this.sharedState.state.new_user.email = '';
					this.sharedState.state.new_user_error = null;
					window.location.href = '/app';
				} else {
					this.sharedState.state.new_user_error = response.body.error;
				}
			}, function(response) {
				this.sharedState.state.new_user_error = 'Something went wrong.';
			});
		},
		add_user: function() {
			this.sharedState.state.errors = [];

			if (!this.sharedState.state.new_user.email_address) {
				this.errors.push('Email address required.');
				return
			}

			if (!this.sharedState.state.new_user.password) {
				this.errors.push('Password required.');
				return
			}

			var data = {	'email_address': this.sharedState.state.new_user.email_address,
										'password': this.sharedState.state.new_user.password }
			postData('/api/user/register', data).then((response) => {
			}, function(response) {
				if (response.success == true) {
					this.sharedState.state.new_user.name = '';
					this.sharedState.state.new_user.email = '';
					this.sharedState.state.new_user_error = null;
					window.location.href = '/app';
				} else {
					this.sharedState.state.new_user_error = response.body.error;
				}
				this.new_user_error = 'Something went wrong.';
			});
		}
	}
})

// vim: set ts=2 sw=2 noet :
var store = {
	state: {
		new_project_name: '',
		errors: [],
	}
}

const app = new Vue({
	el: '#app',
	data: {
		sharedState: store,
	},
	methods: {
		createNewProject: function() {
			this.sharedState.state.errors = [];

			if (!this.sharedState.state.new_project_name) {
				this.errors.push('Project name is required.');
				return
			}

			var data = {'new_project_name': this.sharedState.state.new_project_name, }
			postData('/api/users/new_project', data).then((response) => {
				if (response.success == true) {
					window.location.href = '/app';
				} else {
					this.sharedState.state.errors = [response.error];
				}
			}, (response) => {
				this.sharedState.state.errors = ['Something went wrong.'];
			});
			return this._auth('/api/user/register');
		}
	}
})

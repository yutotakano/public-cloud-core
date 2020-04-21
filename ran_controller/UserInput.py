from flask import Flask, render_template, url_for, redirect

app = Flask(__name__)

class UserInput():

	def __init__(self, data):
		self.app = Flask(__name__)
		self.controller_data = data

	def add_endpoint(self, endpoint=None, endpoint_name=None, handler=None):
		self.app.add_url_rule(endpoint, endpoint_name, handler)
	
	def index(self):
		templateData = {
			'ue_table' : self.ues_to_html(),
			'enb_table' : self.enbs_to_html(),
			'test' : 'TEST'
		}
		return render_template('index.html', **templateData)

	def ue_actions(self, ue_id, action):
		print('ID: ' + str(ue_id))
		print('ACTION: ' + action)

		#Do something

		return redirect(url_for('index'))

	def enb_actions(self, enb_num, action):
		print('ID: ' + str(enb_num))
		print('ACTION: ' + action)

		#Do something

		return redirect(url_for('index'))

	def run(self):
		#Main route
		self.add_endpoint(endpoint='/', endpoint_name='index', handler=self.index)
		self.add_endpoint(endpoint='/ue/<int:ue_id>/<action>', endpoint_name='ue', handler=self.ue_actions)
		self.add_endpoint(endpoint='/enb/<int:enb_num>/<action>', endpoint_name='enb', handler=self.enb_actions)
		self.app.run(port=8080, host='0.0.0.0')

	def ues_to_html(self):
			html = '<div style="overflow:scroll; height:200px; border:solid;">'
			html += '<table style="width:100%">'
			html += '<tr>'
			html += '<th>id</th>'
			html += '<th>IMSI</th>'
			html += '<th>Key</th>'
			html += '<th>OpKey</th>'
			html += '<th>Command</th>'
			html += '<th>IP</th>'
			html += '<th>eNB</th>'
			html += '<th>Status</th>'
			html += '<th>Action</th>'
			html += '</tr>'

			for ue in self.controller_data['UEs']:
				html += '<tr>'
				html += '<td>' + ue.get_id() + '</td>'
				html += '<td>' + ue.get_imsi() + '</td>'
				html += '<td>' + ue.get_key() + '</td>'
				html += '<td>' + ue.get_op_key() + '</td>'
				html += '<td>' + ue.get_command() + '</td>'
				html += '<td>' + ue.get_address_text() + '</td>'
				html += '<td>' + str(ue.get_enb_id()) + '</td>'
				html += '<td>' + ue.get_html_status() + '</td>'
				html += '<td><form action="/ue/' + ue.get_id() + '/generate"><input type="submit" value="Generate" /> </form></td>'
				html += '</tr>'
			html += '</table>'
			html += '</div>'
			return html

	def enbs_to_html(self):
			html = '<div style="overflow:scroll; height:200px; border:solid;">'
			html += '<table style="width:100%">'
			html += '<tr>'
			html += '<th>Number</th>'
			html += '<th>ID</th>'
			html += '<th>MCC</th>'
			html += '<th>MNC</th>'
			html += '<th>Status</th>'
			html += '<th>IP</th>'
			html += '<th>Action</th>'
			html += '</tr>'
			for enb in self.controller_data['eNBs']:
				html += '<tr>'
				html += '<td>' + str(enb.get_num()) + '</td>'
				html += '<td>' + enb.get_id() + '</td>'
				html += '<td>' + enb.get_mcc() + '</td>'
				html += '<td>' + enb.get_mnc() + '</td>'
				html += '<td>' + enb.get_html_status() + '</td>'
				html += '<td>' + enb.get_address_text() + '</td>'
				html += '<td><form action="/enb/' + str(enb.get_num()) + '/generate"><input type="submit" value="Generate" /> </form></td>'
				html += '</tr>'
			html += '</table>'
			html += '</div>'
			return html
		
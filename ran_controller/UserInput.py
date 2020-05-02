from flask import Flask, render_template, url_for, redirect, request
import json
import struct
import socket
from UE import UE
from eNB import eNB

app = Flask(__name__)

class UserInput():

	controller_data = {
		'eNBs': set(),
		'UEs': set()
	}

	def __init__(self, set_data_func):
		self.app = Flask(__name__)
		self.configuration = True
		self.set_data_func = set_data_func

	def add_endpoint(self, endpoint=None, endpoint_name=None, handler=None, methods=None):
		self.app.add_url_rule(endpoint, endpoint_name, handler, methods=methods)

	def generate_data(self, file, epc_ip):
		data = []
		self.epc_ip = epc_ip
		try:
			epc = struct.unpack("!I", socket.inet_aton(epc_ip))[0]
		except:
			return False
		if file:
			file_content = file.read()
			data = json.loads(file_content)
		else:
			return False

		for ue in data['ues']:
			new_ue = UE(ue['ue_id'], ue['ue_mcc'], ue['ue_mnc'], ue['ue_msin'], ue['ue_key'], ue['ue_op_key'], ue['traffic_command'], ue['enb'])
			self.controller_data['UEs'].add(new_ue)
		for enb in data['enbs']:
			new_enb = eNB(enb['enb_num'], enb['enb_id'], enb['enb_mcc'], enb['enb_mnc'])
			self.controller_data['eNBs'].add(new_enb)

		# TODO: Verify JSON

		self.set_data_func(self.controller_data, epc)

		return True
	
	def index(self):
		if self.configuration:
			return render_template('config.html')
		else:
			# If the configuration and the MME IP have been received
			templateData = {
				'ue_table' : self.ues_to_html(),
				'enb_table' : self.enbs_to_html(),
				'mme_ip' : self.epc_ip
			}
			return render_template('index.html', **templateData)

	def config(self):
		if request.method == "POST":
			mme_ip = request.form["mme_ip"]
			file = request.files['file'] if request.files.get('file') else None
			if self.generate_data(file, mme_ip):
				self.configuration = False
		return redirect(url_for('index'))


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
		#Main routes
		self.add_endpoint(endpoint='/', endpoint_name='index', handler=self.index)
		self.add_endpoint(endpoint='/config/', endpoint_name='config', handler=self.config, methods=['POST'])
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
		
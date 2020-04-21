STOPPED = 0
CONNECTED = 1
IDLE = 2
TRAFFIC = 3

class Status():

	def __init__(self):
		self.status = STOPPED

	def move_to_stopped(self):
		self.status = STOPPED

	def move_to_connected(self):
		self.status = CONNECTED

	def move_to_idle(self):
		self.status = IDLE

	def move_to_traffic(self):
		self.status = TRAFFIC

	def get_status(self):
		return self.status

	def get_html_status(self):
		if self.status == STOPPED:
			return '<font color="red">Not Running</font>'
		elif self.status == CONNECTED:
			return '<font color="green">Connected</font>'
		elif self.status == IDLE:
			return '<font color="yellow">Idle</font>'
		elif self.status == TRAFFIC:
			return '<font color="blue">Connected/Traffic</font>'
		else:
			return '<font color="gray">ERROR</font>'
		
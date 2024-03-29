STOPPED = 0
CONNECTED = 1
IDLE = 2
TRAFFIC = 3
PENDING = 4
DISCONNECTED = 5

class Status():

	def __init__(self):
		self.status: int = STOPPED

	def move_to_stopped(self):
		self.status = STOPPED

	def move_to_connected(self):
		self.status = CONNECTED

	def move_to_idle(self):
		self.status = IDLE

	def move_to_traffic(self):
		self.status = TRAFFIC

	def move_to_disconnected(self):
		self.status = DISCONNECTED

	def move_to_pending(self):
		self.status = PENDING

	def get_status(self):
		return self.status

	def get_html_status(self):
		if self.status == STOPPED:
			return '<font color="black">Not Running</font>'
		elif self.status == CONNECTED:
			return '<font color="green">Connected</font>'
		elif self.status == IDLE:
			return '<font color="orange">Idle</font>'
		elif self.status == TRAFFIC:
			return '<font color="blue">Connected/Traffic</font>'
		elif self.status == PENDING:
			return '<font color="gray">Pending</font>'
		elif self.status == DISCONNECTED:
			return '<font color="red">Disconnected</font>'
		else:
			return '<font color="black">ERROR</font>'
		

from Status import Status
import struct
import socket

class UE:
	status = -1
	def __init__(self, ue_id, mcc, mnc, msin, key, op_key, command, enb):
		self.id = ue_id
		self.mcc = mcc
		self.mnc = mnc
		self.msin = msin
		self.key = key
		self.op_key = op_key
		self.command = command.replace('{TUN}', 'tun'+str(int(self.msin)))
		self.status = Status()
		self.enb_id = enb
		self.enb = None
		self.address = None

	def printUE(self):
		print('UE ' + self.get_id() + ':')
		print('\tIMSI: ' + self.get_imsi())
		print('\tKey: ' + self.get_key())
		print('\tOperator Key: ' + self.get_op_key())
		print('\tCommand: ' + self.get_command())
		print('\tStatus: ' + str(self.get_status()))

	def get_id(self):
		return str(self.id)

	def get_id_int(self):
		return self.id

	def get_imsi(self):
		return self.mcc + self.mnc + self.msin

	def get_key(self):
		key = ''
		for i in self.key:
			key += '0x{:02x} '.format(i)
		return key

	def get_op_key(self):
		op_key = ''
		for i in self.op_key:
			op_key += '0x{:02x} '.format(i)
		return op_key

	def get_command(self):
		return self.command

	def get_status(self):
		return self.status.get_status()

	def get_html_status(self):
		return self.status.get_html_status()

	def get_enb_id(self):
		return self.enb_id

	def serialize(self, code, enb):
		data = bytearray()
		# Add Code
		data.append(code)
		# Add ID
		data.append((self.id >> 24) & 0xFF)
		data.append((self.id >> 16) & 0xFF)
		data.append((self.id >> 8) & 0xFF)
		data.append(self.id & 0xFF)
		# Add IMSI
		imsi = self.mcc + self.mnc + self.msin
		for c in imsi:
			data.append(ord(c) & 0xFF)
		# Add key
		for b in self.key:
			data.append(b & 0xFF)
		# Add op_key
		for b in self.op_key:
			data.append(b & 0xFF)
		# Add command_length
		data.append((len(self.command) >> 8) & 0xFF)
		data.append(len(self.command) & 0xFF)
		# Add command
		for c in self.command:
			data.append(ord(c) & 0xFF)
		# Add eNB IP
		ip = struct.unpack("!I", socket.inet_aton(enb.get_address()[0]))[0]
		data.append((ip >> 24) & 0xFF)
		data.append((ip >> 16) & 0xFF)
		data.append((ip >> 8) & 0xFF)
		data.append(ip & 0xFF)

		# Add eNB internal port
		port = enb.get_ue_port()
		data.append((port >> 8) & 0xFF)
		data.append(port & 0xFF)

		return data

	def get_address_text(self):
		if self.address == None:
			return 'Unknown'
		return self.address[0] + ':' + str(self.address[1])

	def set_addr(self, address):
		self.address = address

	def set_traffic(self):
		self.status.move_to_traffic()

	def set_idle(self):
		self.status.move_to_idle()


	def __eq__(self, other):
		return self.id != other.id and self.msin != other.msin

	def __hash__(self):
		return self.id
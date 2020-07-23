from Status import Status
import struct
import socket
from threading import Lock

class UE:
	status = -1

	def __init__(self, ue_id, mcc, mnc, msin, key, op_key, command, enb, control_plane):
		self.id = ue_id
		self.mcc = mcc
		self.mnc = mnc
		self.msin = msin
		self.key_plain = key
		self.key = self.processKey(key)
		self.op_key_plain = op_key
		self.op_key = self.processKey(op_key)
		self.command = command.replace('{TUN}', 'tun'+str(int(self.msin)))
		self.status = Status()
		self.enb_id = enb
		self.enb = None
		self.address = None
		self.mutex = Lock()
		self.running = False
		self.control_plane = control_plane
		self.serialized_control_plane = None

	def processKey(self, key):
		key_aux = key.replace('0x', '')
		return [int(key_aux[i:i+2], 16) for i in range(0, len(key), 2) if i < 32]

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
		return self.key_plain

	def get_op_key(self):
		return self.op_key_plain

	def get_command(self):
		return self.command

	def get_status(self):
		return self.status.get_status()

	def get_html_status(self):
		return self.status.get_html_status()

	def get_enb_id(self):
		return self.enb_id

	def serialize(self, code, enb, multiplexer, epc):
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

		# Add Multiplexer IP
		data.append((multiplexer >> 24) & 0xFF)
		data.append((multiplexer >> 16) & 0xFF)
		data.append((multiplexer >> 8) & 0xFF)
		data.append(multiplexer & 0xFF)

		# Add Multiplexer/EPC port
		if epc == multiplexer:
			data.append((2152 >> 8) & 0xFF)
			data.append(2152 & 0xFF)
		else:
			data.append((2154 >> 8) & 0xFF)
			data.append(2154 & 0xFF)

		# Add serialized_control_plane
		for i in self.serialized_control_plane:
			data.append(i)

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

	def set_pending(self):
		self.status.move_to_pending()

	def set_stopped(self):
		self.status.move_to_stopped()

	def set_disconnected(self):
		self.status.move_to_disconnected()

	def acquire(self):
		self.mutex.acquire()

	def release(self):
		self.mutex.release()

	def locked(self):
		return self.mutex.locked()

	def set_flag(self, flag):
		self.running = flag

	def get_flag(self):
		return self.running

	def get_control_plane(self):
		return self.control_plane

	def set_serialized_control_plane(self, serialized_control_plane):
		self.serialized_control_plane = serialized_control_plane


	def __eq__(self, other):
		return self.id != other.id and self.msin != other.msin

	def __hash__(self):
		return self.id
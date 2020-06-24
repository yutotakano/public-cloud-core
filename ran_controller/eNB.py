from Status import Status
from threading import Lock

class eNB:
	status = -1
	def __init__(self, enb_num, enb_id, mcc, mnc):
		self.num = enb_num
		self.id = enb_id
		self.mcc = mcc
		self.mnc = mnc
		self.status = Status()
		self.address = None
		self.mutex = Lock()

	def printENB(self):
		print('eNB ' + str(self.get_num()) + ':')
		print('\tID: ' + self.get_id())
		print('\tMCC: ' + self.get_mcc())
		print('\tMNC: ' + self.get_mnc())
		print('\tStatus: ' + str(self.get_status()))

	def get_num(self):
		return self.num

	def get_id(self):
		return '0x{:08x}'.format(self.id)

	def get_id_int(self):
		return self.id

	def get_mcc(self):
		return self.mcc

	def get_mnc(self):
		return self.mnc

	def get_status(self):
		return self.status.get_status()

	def get_html_status(self):
		return self.status.get_html_status()

	def get_address(self):
		return self.address

	def get_address_text(self):
		if self.address == None:
			return 'Unknown'
		return self.address[0] + ':' + str(self.address[1]) + ':' + str(self.ue_port)

	def get_ue_port(self):
		return self.ue_port

	def serialize(self, code, epc):
		# Add ID
		data = bytearray()
		# Add CODE
		data.append(code)
		data.append((self.id >> 24) & 0xFF)
		data.append((self.id >> 16) & 0xFF)
		data.append((self.id >> 8) & 0xFF)
		data.append(self.id & 0xFF)
		# Add MCC
		data.append(int(self.mcc[0]))
		data.append(int(self.mcc[1]))
		data.append(int(self.mcc[2]))
		# Add MNC
		data.append(int(self.mnc[0]))
		data.append(int(self.mnc[1]))
		# Add EPC IP
		data.append((epc >> 24) & 0xFF)
		data.append((epc >> 16) & 0xFF)
		data.append((epc >> 8) & 0xFF)
		data.append(epc & 0xFF)
		return data

	def set_addr(self, address):
		self.address = address

	def set_ue_port(self, port):
		self.ue_port = port

	def set_stopped(self):
		self.status.move_to_stopped()

	def set_pending(self):
		self.status.move_to_pending()

	def set_connected(self):
		self.status.move_to_connected()

	def acquire(self):
		self.mutex.acquire()

	def release(self):
		self.mutex.release()

	def locked(self):
		return self.mutex.locked()


	def __eq__(self, other):
		return self.num != other.num and self.id != other.id

	def __hash__(self):
		return self.id

		
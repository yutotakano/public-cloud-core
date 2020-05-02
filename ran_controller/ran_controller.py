import socket
import threading
from UE import UE
from eNB import eNB
from UserInput import UserInput
from queue import Queue
import Status

# Codes definitions

CODE_INIT = 0x01
CODE_OK = 0x80
CODE_ERROR = 0x00
CODE_ENB_BEHAVIOUR = 0x01
CODE_UE_BEHAVIOUR = 0x02

'''
    Bit 0 (Status)
            0:ERROR
            1:OK
    Bit 1 (Unused)
    Bit 2 (Unused)
    Bit 3 (Unused)
    Bit 4 (Unused)
    Bit 5 (Unused)
    Bit 6 (UE)
    Bit 7 (eNB)
'''

class RANControler:

	def __init__(self):
		# Init communication queues
		self.controller_queue = Queue(maxsize=0)
		self.send_queue = Queue(maxsize=0)
		self.user_queue = Queue(maxsize=0)

	def start_user_input(self):
		user_input = UserInput(self.start_controller)
		user_input.run()

		

	def start_controller(self, controller_data, epc):
		self.controller_data = controller_data
		self.epc = epc
		self.execute()

	def execute(self):
		# Create a TCP/IP socket
		self.sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

		# Bind the socket to the port
		server_address = ('0.0.0.0', 1234)
		self.sock.bind(server_address)

		#Create 3 threads
		receive_t = threading.Thread(target=self.receive_thread)
		send_t = threading.Thread(target=self.send_thread)
		controller_t = threading.Thread(target=self.controller_thread)

		#Start threads
		receive_t.start()
		send_t.start()
		controller_t.start()

	def get_enb_by_buffer(self, data):
		num = (data[0] << 24) | (data[1] << 16) | (data[2] << 8) | data[3]
		port = data[4] << 8 | data[5]
		for enb in self.controller_data['eNBs']:
			if enb.get_id_int() == num:
				enb.set_ue_port(port)
				return enb
		return None

	def get_ue_by_buffer(self, data):
		num = (data[0] << 24) | (data[1] << 16) | (data[2] << 8) | data[3]
		for ue in self.controller_data['UEs']:
			if ue.get_id_int() == num:
				return ue
		return None

	def analyze_msg(self, msg):
		if msg['type'] == 'init':
			print('New slave at ' + msg['ip'] + ':' + str(msg['port']))
			# Get the first not assigned UE or eNB
			for ue in self.controller_data['UEs']:
				if ue.get_status() == Status.STOPPED:
					# Get associated eNB
					assoc_enb = next((enb for enb in self.controller_data['eNBs'] if enb.get_num() == ue.get_enb_id()), None)
					# None assoc_enb case does not exist because Verify function guarantees that (TODO: Verify database)
					if(assoc_enb.get_status() == Status.STOPPED):
						# This slave has to be a eNB
						buf = assoc_enb.serialize(CODE_OK | CODE_ENB_BEHAVIOUR, self.epc)
						print(buf)
						self.sock.sendto(buf, (msg['ip'], msg['port']))
						break
					else:
						# This slave is a UE
						buf = ue.serialize(CODE_OK | CODE_UE_BEHAVIOUR, assoc_enb)
						print(buf)
						self.sock.sendto(buf, (msg['ip'], msg['port']))
						break


		elif msg['type'] == 'enb_run':
			# eNB slave answers with OK message
			# Get eNB num from data
			enb = self.get_enb_by_buffer(msg['data'])

			# Save eNB address
			enb.set_addr((msg['ip'], msg['port']))
			enb.set_connected() # Running/Disconnected

		elif msg['type'] == 'ue_run':
			# UE slave answers with OK message
			# Get UE num from data
			ue = self.get_ue_by_buffer(msg['data'])

			# Save UE address
			ue.set_addr((msg['ip'], msg['port']))
			ue.set_traffic()


	def generate_msg(self, data, address):
		msg = {}
		# Special code case: 0000 0001
		if data[0] == CODE_INIT:
			msg['type'] = 'init'
			msg['ip'] = address[0]
			msg['port'] = address[1]
		# Error code: 0XXX XXXX 
		elif (data[0] & CODE_OK) == CODE_ERROR:
			msg['type'] = 'error'
			msg['ip'] = address[0]
			msg['port'] = address[1]
		# eNB ok: 1000 0001
		elif data[0] == CODE_OK | CODE_ENB_BEHAVIOUR:
			msg['type'] = 'enb_run'
			msg['ip'] = address[0]
			msg['port'] = address[1]
			msg['data'] = data[1:]
		elif data[0] == CODE_OK | CODE_UE_BEHAVIOUR:
			msg['type'] = 'ue_run'
			msg['ip'] = address[0]
			msg['port'] = address[1]
			msg['data'] = data[1:]

		return msg


	def controller_thread(self):
		print('Init controller thread')
		while 1:
			msg = self.controller_queue.get()
			self.analyze_msg(msg)


	def send_thread(self):
		print('Init sender thread')
		while 1:
			msg = self.send_queue.get()
			sock.sendto(msg['data'], msg['address'])

	def receive_thread(self):
		print('Init receiver thread')
		while 1:
			data, address = self.sock.recvfrom(1024)
			msg = self.generate_msg(data, address)
			self.controller_queue.put(msg)


		

if __name__ == '__main__':
	ran_controler = RANControler()
	ran_controler.start_user_input()
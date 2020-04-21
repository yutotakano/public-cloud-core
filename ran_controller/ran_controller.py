import json
import sys
import socket
import struct
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
	controller_data = {
		'eNBs': set(),
		'UEs': set()
	}

	def __init__(self, file_path, epc_ip):
		data = []

		# Init communication queues
		self.controller_queue = Queue(maxsize=0)
		self.send_queue = Queue(maxsize=0)
		self.user_queue = Queue(maxsize=0)

		self.epc = struct.unpack("!I", socket.inet_aton(epc_ip))[0]

		with open(file_path) as json_file:
			data = json.load(json_file)
		for ue in data['ues']:
			new_ue = UE(ue['ue_id'], ue['ue_mcc'], ue['ue_mnc'], ue['ue_msin'], ue['ue_key'], ue['ue_op_key'], ue['traffic_command'], ue['enb'])
			self.controller_data['UEs'].add(new_ue)
		for enb in data['enbs']:
			new_enb = eNB(enb['enb_num'], enb['enb_id'], enb['enb_mcc'], enb['enb_mnc'])
			self.controller_data['eNBs'].add(new_enb)

	def execute(self):
		# Create a TCP/IP socket
		self.sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

		# Bind the socket to the port
		server_address = ('0.0.0.0', 1234)
		self.sock.bind(server_address)

		#Create 3 threads
		receive_t = threading.Thread(target=self.receive_thread)
		send_t = threading.Thread(target=self.send_thread)
		user_t = threading.Thread(target=self.user_thread, args=(self.controller_data, ))

		#Start threads
		receive_t.start()
		send_t.start()
		user_t.start()
		self.controller_thread()

	def get_enb_by_buffer(self, data):
		num = (data[0] << 24) | (data[1] << 16) | (data[2] << 8) | data[3]
		for enb in self.controller_data['eNBs']:
			if enb.get_id_int() == num:
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

	def user_thread(self, controller_data):
		print('Init receiver thread')
		user_input = UserInput(controller_data)
		user_input.run()


		

if __name__ == '__main__':
	if(len(sys.argv) != 3):
		print('USE: python ran_controler <config_file.json> <EPC_IP>')
		exit(1)
	ran_controler = RANControler(sys.argv[1], sys.argv[2])
	ran_controler.execute()
import socket
import threading
from UE import UE
from eNB import eNB
from UserInput import UserInput
from queue import Queue
import Status
import time
import struct
import math

from kubernetes import config, client

# Debug variable to use in local testbed
k8s = True
mobilestream = False



# Codes definitions

CODE_INIT = 0xFF
CODE_OK = 0x80
CODE_ERROR = 0x00
CODE_ENB_BEHAVIOUR = 0x01
CODE_UE_BEHAVIOUR = 0x02
CODE_UE_IDLE = 0x03
CODE_UE_DETACH = 0x04
CODE_UE_ATTACH = 0x05
CODE_UE_MOVED_TO_CONNECTED = 0x06
CODE_UE_GET_ENB = 0x07
CODE_UE_X2_HANDOVER_COMPLETED = 0x08
CODE_UE_ATTACH_TO_ENB = 0x09
CODE_UE_S1_HANDOVER_COMPLETED = 0x0A
CODE_CP_MODE_ONLY = 0x0B

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
		self.send_queue = Queue(maxsize=0)
		self.user_queue = Queue(maxsize=0)
		# Slave K8s manifest
		self.pod_manifest = {
			'apiVersion': 'v1',
			'kind': 'Pod',
			'metadata': {
				'name': 'pod-name'
			},
			'spec': {
				'containers': [{
					'image': 'j0lama/ran_slave:latest',
					'name': 'name',
					'env': [{'name': 'NODE_IP',
								'valueFrom': {'fieldRef': {'fieldPath': 'status.hostIP'}}},
							{'name': 'NODE_NAME',
								'valueFrom': {'fieldRef': {'fieldPath': 'spec.nodeName'}}}],
					'securityContext': {
						'capabilities': {
							'add': ['NET_ADMIN']},
							'privileged': False
						},
					'volumeMounts': [{
						'mountPath': '/dev/net/tun',
						'name': 'dev-tun'
					}],
					"args": [
						"/bin/sh",
						"-c",
						"python3 powder_hack.py $INTERNAL_CONTROLLER_SERVICE_HOST $NODE_NAME"
					],
				}],
				'volumes': [{
					'hostPath': {
						'path': '/dev/net/tun',
						'type': 'CharDevice'
					},
					'name': 'dev-tun'
				}]
			}
		}

	def start_user_input(self):
		self.user_input = UserInput(self.start_controller, self.kubernetes_restart)
		self.user_input.run()

		

	def start_controller(self, controller_data, docker_image, epc, multiplexer, restart, cp_mode, num_threads):
		self.controller_data = controller_data
		self.docker_image = docker_image
		self.epc = epc
		self.multiplexer = multiplexer
		self.cp_mode = cp_mode
		self.num_threads = num_threads
		self.num_ues = len(self.controller_data['UEs'])
		self.enb_ips = []
		if restart == False:
			self.execute()
		else:
			kubernetes_t = threading.Thread(target=self.kubernetes_thread)
			kubernetes_t.start()

	def execute(self):
		# Create a socket
		self.sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

		# Bind the socket to the port
		server_address = ('0.0.0.0', 1234)
		self.sock.bind(server_address)

		#Create 3 threads
		receive_t = threading.Thread(target=self.receive_thread)
		send_t = threading.Thread(target=self.send_thread)
		kubernetes_t = threading.Thread(target=self.kubernetes_thread)

		#Start threads
		receive_t.start()
		send_t.start()
		kubernetes_t.start()

	def get_enb_by_buffer(self, data):
		num = (data[0] << 24) | (data[1] << 16) | (data[2] << 8) | data[3]
		for enb in self.controller_data['eNBs']:
			if enb.get_id_int() == num:
				return enb
		return None

	def get_enb(self, data):
		num = (data[0] << 24) | (data[1] << 16) | (data[2] << 8) | data[3]
		for enb in self.controller_data['eNBs']:
			if enb.get_num() == num:
				return enb
		return None

	def get_ue_by_buffer(self, data):
		num = (data[0] << 24) | (data[1] << 16) | (data[2] << 8) | data[3]
		for ue in self.controller_data['UEs']:
			if ue.get_id_int() == num:
				return ue
		return None

	def analyze_msg(self, msg):
		print(msg['type'])
		if msg['type'] == 'init' or msg['type'] == 'cp_mode':
			print('New slave at ' + msg['ip'] + ':' + str(msg['port']))
			print('NodeIP (integer): %d' % msg['ip_node'])
			# Assign first eNBs
			for enb in self.controller_data['eNBs']:
				enb.acquire_assign()
				if enb.get_status() == Status.STOPPED and msg['ip_node'] not in self.enb_ips:
					enb.set_pending()
					# This is needed because of the MobileStream implementation
					if mobilestream == True:
						self.enb_ips.append(msg['ip_node'])
					enb.acquire()
					enb.release_assign()
					# This slave has to be a eNB
					buf = enb.serialize(CODE_OK | CODE_ENB_BEHAVIOUR, self.epc)
					self.sock.sendto(buf, (msg['ip'], msg['port']))
					return
				enb.release_assign()

			# Control Plane Only mode enabled
			if self.cp_mode == True and msg['type'] == 'init':
				print('New UE in Control-Plane Only mode')
				if self.num_ues == -1:
					# Send error message to the slave
					print('ERROR: (Control-Plane Only Mode) No role available for the slave at ' + msg['ip'] + ':' + str(msg['port']))
					buf = bytearray()
					buf.append(CODE_ERROR)
					self.sock.sendto(buf, (msg['ip'], msg['port']))
					return
				if self.num_ues < self.num_threads:
					num = self.num_ues
					self.num_ues = -1
				else:
					num = self.num_threads
					self.num_ues = self.num_ues - self.num_threads
				print('Number of UEs for this container: ' + str(num))
				buf = bytearray()
				buf.append(CODE_OK | CODE_CP_MODE_ONLY)
				buf.append((num >> 24) & 0xFF)
				buf.append((num >> 16) & 0xFF)
				buf.append((num >> 8) & 0xFF)
				buf.append(num & 0xFF)
				self.sock.sendto(buf, (msg['ip'], msg['port']))
				return

			# Get the first not assigned UE
			for ue in self.controller_data['UEs']:
				ue.acquire()
				if ue.get_status() == Status.STOPPED:
					ue.set_pending()
					ue.release()
					# At this point, only one thread can execute the following sentences

					# Get associated eNB
					assoc_enb = next((enb for enb in self.controller_data['eNBs'] if enb.get_num() == ue.get_enb_id()), None)

					# Special race condition: eNB has been assigned but it does not already answer
					while(assoc_enb.locked() == True):
						time.sleep(1)

					buf = ue.serialize(CODE_OK | CODE_UE_BEHAVIOUR, assoc_enb, self.multiplexer, self.epc)
					self.sock.sendto(buf, (msg['ip'], msg['port']))
					return
				ue.release()

			# Send error message to the slave
			print('ERROR: No role available for the slave at ' + msg['ip'] + ':' + str(msg['port']))
			buf = bytearray()
			buf.append(CODE_ERROR)
			self.sock.sendto(buf, (msg['ip'], msg['port']))
			return

		elif msg['type'] == 'enb_run':
			# eNB slave answers with OK message
			# Get eNB from data
			enb = self.get_enb_by_buffer(msg['data'])

			# Save eNB address
			enb.set_addr((msg['ip'], msg['port']))
			enb.set_connected() # Running/Disconnected
			enb.release()
			print('New eNB at ' + msg['ip'] + ':' + str(msg['port']))

		elif msg['type'] == 'enb_error_run':
			# eNB slave answers with Error message
			# Get eNB from data
			enb = self.get_enb_by_buffer(msg['data'])

			ip_node = (msg['data'][4] << 24) | (msg['data'][5] << 16) | (msg['data'][6] << 8) | msg['data'][7]
			if mobilestream == True:
				self.enb_ips.remove(ip_node)

			# Save eNB address
			enb.set_stopped() # Stopped/Not Running
			enb.release()
			print('eNB error at ' + msg['ip'] + ':' + str(msg['port']))

		elif msg['type'] == 'ue_run':
			# UE slave answers with OK message
			# Get UE from data
			ue = self.get_ue_by_buffer(msg['data'])

			# Save UE address
			ue.set_addr((msg['ip'], msg['port']))
			ue.set_traffic()
			self.controller_data['running_ues'] = self.controller_data['running_ues'] + 1
			print('New UE (' + ue.get_imsi() + ') at ' + msg['ip'] + ':' + str(msg['port']))

		elif msg['type'] == 'ue_error_run':
			# UE slave answers with Error message
			# Get UE from data
			ue = self.get_ue_by_buffer(msg['data'])

			# Save UE address
			ue.set_stopped()
			ue.set_flag(False)
			print('UE error at ' + msg['ip'] + ':' + str(msg['port']))

		elif msg['type'] == 'ue_idle':
			# UE slave informs that its new state is Idle
			# Get UE from data
			ue = self.get_ue_by_buffer(msg['data'])

			# Save UE address
			ue.set_idle()
			print('UE (' + ue.get_imsi() + ') at ' + msg['ip'] + ':' + str(msg['port']) + ' moved to Idle')

		elif msg['type'] == 'ue_detached':
			# UE slave informs that its new state is Detached
			# Get UE from data
			ue = self.get_ue_by_buffer(msg['data'])

			# Save UE address
			ue.set_disconnected()
			print('UE (' + ue.get_imsi() + ') at ' + msg['ip'] + ':' + str(msg['port']) + ' moved to Detached')

		elif msg['type'] == 'ue_attached':
			# UE slave informs that its new state is Attached/Connected
			# Get UE from data
			ue = self.get_ue_by_buffer(msg['data'])

			# Save UE address
			ue.set_traffic()
			print('UE (' + ue.get_imsi() + ') at ' + msg['ip'] + ':' + str(msg['port']) + ' moved to Attached')

		elif msg['type'] == 'moved_to_connected':
			# UE slave informs that its new state is Attached/Connected
			# Get UE from data
			ue = self.get_ue_by_buffer(msg['data'])

			# Save UE address
			ue.set_traffic()
			print('UE (' + ue.get_imsi() + ') at ' + msg['ip'] + ':' + str(msg['port']) + ' moved to Connected')

		elif msg['type'] == 'get_enb':

			# Get UE from data
			ue = self.get_ue_by_buffer(msg['data'][:4]) # First 4 bytes

			# Get eNB from data
			enb = self.get_enb(msg['data'][4:]) # Last 4 bytes

			buf = bytearray()

			if enb == None:
				# Wrong eNB case
				# Generate Error message
				print('UE (' + ue.get_imsi() + ') at ' + msg['ip'] + ':' + str(msg['port']) + ' requests a wrong eNB Number')
				buf.append(CODE_UE_GET_ENB)
			else:
				# Correct eNB case
				print('UE (' + ue.get_imsi() + ') at ' + msg['ip'] + ':' + str(msg['port']) + ' requests eNB ' + str(enb.get_num()) + ' IP address')
				buf.append(CODE_OK | CODE_UE_GET_ENB)
				# Add eNB IP
				try:
					ip = struct.unpack("!I", socket.inet_aton(enb.get_address()[0]))[0]
					buf.append((ip >> 24) & 0xFF)
					buf.append((ip >> 16) & 0xFF)
					buf.append((ip >> 8) & 0xFF)
					buf.append(ip & 0xFF)
				except:
					buf.append(0)
					buf.append(0)
					buf.append(0)
					buf.append(0)
			self.sock.sendto(buf, (msg['ip'], msg['port']))

		elif msg['type'] == 'x2_completed':
			# Get UE from data
			ue = self.get_ue_by_buffer(msg['data'][:4]) # First 4 bytes

			# Get eNB from data
			enb = self.get_enb(msg['data'][4:]) # Last 4 bytes

			# Update UE info
			ue.set_enb_id(enb.get_num())

			print('UE (' + ue.get_imsi() + ') at ' + msg['ip'] + ':' + str(msg['port']) + ' X2 Handover completed to eNB ' + str(enb.get_num()))

		elif msg['type'] == 'ue_attached_to_enb':
			# Get UE from data
			ue = self.get_ue_by_buffer(msg['data'][:4]) # First 4 bytes

			# Get eNB from data
			enb = self.get_enb(msg['data'][4:]) # Last 4 bytes

			# Update UE info
			ue.set_enb_id(enb.get_num())
			ue.set_traffic()

			print('UE (' + ue.get_imsi() + ') at ' + msg['ip'] + ':' + str(msg['port']) + ' Attached to eNB ' + str(enb.get_num()))

		elif msg['type'] == 's1_completed':
			# Get UE from data
			ue = self.get_ue_by_buffer(msg['data'][:4]) # First 4 bytes

			# Get eNB from data
			enb = self.get_enb(msg['data'][4:]) # Last 4 bytes

			# Update UE info
			ue.set_enb_id(enb.get_num())

			print('UE (' + ue.get_imsi() + ') at ' + msg['ip'] + ':' + str(msg['port']) + ' S1 Handover completed to eNB ' + str(enb.get_num()))


	def generate_msg(self, data, address):
		msg = {}

		if data[0] == CODE_INIT:
			msg['type'] = 'init'
			msg['ip'] = address[0]
			msg['port'] = address[1]
			msg['ip_node'] = (data[1] << 24) | (data[2] << 16) | (data[3] << 8) | data[4]
		elif data[0] == CODE_CP_MODE_ONLY:
			msg['type'] = 'cp_mode'
			msg['ip'] = address[0]
			msg['port'] = address[1]
			msg['data'] = data[1:]
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
		elif data[0] == CODE_UE_IDLE:
			msg['type'] = 'ue_idle'
			msg['ip'] = address[0]
			msg['port'] = address[1]
			msg['data'] = data[1:]
		elif data[0] == CODE_UE_DETACH:
			msg['type'] = 'ue_detached'
			msg['ip'] = address[0]
			msg['port'] = address[1]
			msg['data'] = data[1:]
		elif data[0] == CODE_UE_ATTACH:
			msg['type'] = 'ue_attached'
			msg['ip'] = address[0]
			msg['port'] = address[1]
			msg['data'] = data[1:]
		elif data[0] == CODE_UE_MOVED_TO_CONNECTED:
			msg['type'] = 'moved_to_connected'
			msg['ip'] = address[0]
			msg['port'] = address[1]
			msg['data'] = data[1:]
		elif data[0] == CODE_UE_GET_ENB:
			msg['type'] = 'get_enb'
			msg['ip'] = address[0]
			msg['port'] = address[1]
			msg['data'] = data[1:]
		elif data[0] == CODE_UE_X2_HANDOVER_COMPLETED:
			msg['type'] = 'x2_completed'
			msg['ip'] = address[0]
			msg['port'] = address[1]
			msg['data'] = data[1:]
		elif data[0] == CODE_UE_ATTACH_TO_ENB:
			msg['type'] = 'ue_attached_to_enb'
			msg['ip'] = address[0]
			msg['port'] = address[1]
			msg['data'] = data[1:]
		elif data[0] == CODE_UE_S1_HANDOVER_COMPLETED:
			msg['type'] = 's1_completed'
			msg['ip'] = address[0]
			msg['port'] = address[1]
			msg['data'] = data[1:]

		# Error Init cases
		elif data[0] == CODE_ENB_BEHAVIOUR:
			msg['type'] = 'enb_error_run'
			msg['ip'] = address[0]
			msg['port'] = address[1]
			msg['data'] = data[1:]

		elif data[0] == CODE_UE_BEHAVIOUR:
			msg['type'] = 'ue_error_run'
			msg['ip'] = address[0]
			msg['port'] = address[1]
			msg['data'] = data[1:]

		# Error code: 0XXX XXXX 
		elif (data[0] & CODE_OK) == CODE_ERROR:
			msg['type'] = 'error'
			msg['ip'] = address[0]
			msg['port'] = address[1]

		return msg


	def send_thread(self):
		print('Init sender thread')
		while 1:
			msg = self.send_queue.get()
			sock.sendto(msg['data'], msg['address'])

	def receive_thread(self):
		print('Init receiver thread')
		while 1:
			data, address = self.sock.recvfrom(16384)
			msg = self.generate_msg(data, address)
			worker = threading.Thread(target=self.analyze_msg, args=(msg,))
			worker.start()

	def kubernetes_thread(self):
		# Wait 2 seconds to the rest of the threads
		time.sleep(2)

		if self.cp_mode == True:
			tot_len = len(self.controller_data['eNBs']) + math.ceil(len(self.controller_data['UEs']) / self.num_threads)
		else:
			tot_len = len(self.controller_data['UEs']) + len(self.controller_data['eNBs'])

		# Init Kubernetes API connection
		if k8s == True:
			config.load_incluster_config()
			v1 = client.CoreV1Api()
		print('Staring Slave Pods...')
		for i in range(tot_len):
			# Configure POD Manifest for each slave
			self.pod_manifest['metadata']['name'] = 'slave-' + str(i)
			self.pod_manifest['spec']['containers'][0]['image'] = self.docker_image
			self.pod_manifest['spec']['containers'][0]['name'] = 'slave-' + str(i)
			# Create a POD
			if k8s == True:
				v1.create_namespaced_pod(body=self.pod_manifest, namespace='default')
		print('Slave pods started!')
		return

	def kubernetes_restart(self):
		# Remove all eNB IPs
		self.enb_ips = []
		if self.cp_mode == True:
			tot_len = len(self.controller_data['eNBs']) + math.ceil(len(self.controller_data['UEs']) / self.num_threads)
		else:
			tot_len = len(self.controller_data['UEs']) + len(self.controller_data['eNBs'])

		if k8s == True:
			core_v1 = client.CoreV1Api()
			delete_options = client.V1DeleteOptions()
			print('Removing Slave Pods')
			for i in range(tot_len):
				core_v1.delete_namespaced_pod(name='slave-' + str(i), namespace='default', body=delete_options)
		print('Pods removed!')
		return
		

if __name__ == '__main__':
	ran_controler = RANControler()
	ran_controler.start_user_input()

import requests
import sys

if(len(sys.argv) != 7):
        print('USE: python3 feeder.py <Controller IP> <MME IP> <Multiplexer IP> <Docker Image> <Refresh time> <Nervion Config file>')
        print('EXAMPLE: python3 feeder.py 192.168.4.82 192.168.4.80 192.168.4.80 j0lama/ran_slave:latest 10 test.json')
        sys.exit(1)

url = 'http://' + sys.argv[1] + ':8080/config/'

info = {'mme_ip': sys.argv[2],
        'multi_ip': sys.argv[3],
        'docker_image':sys.argv[4],
        'refresh_time':sys.argv[5]}

files = {'config': open(sys.argv[6], 'rb')}

x = requests.post(url, data=info, files=files)

print('Done!')

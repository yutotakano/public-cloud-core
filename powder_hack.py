import sys
from subprocess import call

x = '10.10.1.' + str(int(sys.argv[2].split('.')[0].replace('slave', ''))+3)

call(['./ran_emulator', sys.argv[1], x])

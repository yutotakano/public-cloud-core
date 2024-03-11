import sys
from subprocess import call

x = "192.168.4." + str(int(sys.argv[2].split(".")[0].replace("slave", "")) + 83)

call(["./ran-emulator", sys.argv[1], x])

from Status import Status
from threading import Lock
from typing import Optional


class eNB:
    def __init__(self, enb_num: int, mcc: str, mnc: str):
        self.num: int = enb_num
        self.id: int = int(enb_num) + 14680064
        self.mcc: str = mcc
        self.mnc: str = mnc
        self.status = Status()
        self.address: Optional[tuple[str, int]] = None
        self.mutex = Lock()
        self.mutex_assign = Lock()
        self.pod_name = "N/A"

    def printENB(self):
        print("eNB " + str(self.get_num()) + ":")
        print("\tID: " + self.get_id())
        print("\tMCC: " + self.get_mcc())
        print("\tMNC: " + self.get_mnc())
        print("\tStatus: " + str(self.get_status()))

    def get_num(self):
        return self.num

    def get_id(self):
        return "0x{:08x}".format(self.id)

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
            return "Unknown"
        return self.address[0] + ":" + str(self.address[1])

    def get_ue_port(self):
        return self.ue_port

    def get_pod_name(self):
        return self.pod_name

    def serialize(self, code: int, epc: int):
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

    def set_addr(self, address: tuple[str, int]):
        self.address = address

    def set_ue_port(self, port: int):
        self.ue_port = port

    def set_stopped(self):
        self.status.move_to_stopped()

    def set_pending(self):
        self.status.move_to_pending()

    def set_connected(self):
        self.status.move_to_connected()

    def set_pod_name(self, name: str):
        self.pod_name = name

    def acquire(self):
        self.mutex.acquire()

    def release(self):
        self.mutex.release()

    def locked(self):
        return self.mutex.locked()

    def acquire_assign(self):
        self.mutex_assign.acquire()

    def release_assign(self):
        self.mutex_assign.release()

    def locked_assign(self):
        return self.mutex_assign.locked()

    def __eq__(self, other: object):
        if not isinstance(other, eNB):
            return NotImplemented
        return self.num != other.num and self.id != other.id

    def __hash__(self):
        return self.id

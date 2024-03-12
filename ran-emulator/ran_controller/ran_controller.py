import socket
import threading
from UserInput import UserInput, ControllerData
import Status
import time
import struct
import math

from kubernetes.config import incluster_config
from kubernetes import client
from kubernetes.client.models import (
    V1Pod,
    V1ObjectMeta,
    V1PodSpec,
    V1Container,
    V1EnvVar,
    V1EnvVarSource,
    V1ObjectFieldSelector,
    V1SecurityContext,
    V1Capabilities,
    V1Volume,
    V1VolumeMount,
    V1HostPathVolumeSource,
)

from typing import TypedDict, Optional, Union, Literal

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

"""
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
"""


class ReceivedMessageWithIP(TypedDict):
    type: Union[Literal["init"], Literal["cp_mode"]]
    ip: str
    port: int
    ip_node: int


class ReceivedMessageWithData(TypedDict):
    type: Union[
        Literal["enb_run"],
        Literal["ue_run"],
        Literal["ue_idle"],
        Literal["ue_detached"],
        Literal["ue_attached"],
        Literal["moved_to_connected"],
        Literal["get_enb"],
        Literal["x2_completed"],
        Literal["ue_attached_to_enb"],
        Literal["s1_completed"],
        Literal["enb_error_run"],
        Literal["ue_error_run"],
    ]
    ip: str
    port: int
    data: bytes


class ReceivedMessageError(TypedDict):
    type: Literal["error"]
    ip: str
    port: int


ReceivedMessage = Union[
    ReceivedMessageWithIP, ReceivedMessageWithData, ReceivedMessageError
]


class RANControler:

    # used to notify old kubernetes threads to stop scaling when restarting,
    # so they don't keep incrementally adding pods. This flag defaults to
    # SET/ON, and is only turned off when restarting.
    should_keep_scaling = threading.Event()

    def __init__(self):
        # Initialize Kubernetes if needed
        if k8s == True:
            incluster_config.load_incluster_config()

        self.lock = threading.Lock()

        # Slave K8s manifest
        self.pod_manifest = V1Pod(
            api_version="v1",
            kind="Pod",
            metadata=V1ObjectMeta(name="pod-name", labels={"app": "ran-slave"}),
            spec=V1PodSpec(
                containers=[
                    V1Container(
                        image="ghcr.io/yutotakano/ran-slave:latest",
                        image_pull_policy="IfNotPresent",
                        name="name",
                        env=[
                            V1EnvVar(
                                name="NODE_IP",
                                value_from=V1EnvVarSource(
                                    field_ref=V1ObjectFieldSelector(
                                        field_path="status.hostIP"
                                    )
                                ),
                            ),
                            V1EnvVar(
                                name="NODE_NAME",
                                value_from=V1EnvVarSource(
                                    field_ref=V1ObjectFieldSelector(
                                        field_path="spec.nodeName"
                                    )
                                ),
                            ),
                        ],
                        security_context=V1SecurityContext(
                            capabilities=V1Capabilities(add=["NET_ADMIN"]),
                            privileged=False,
                        ),
                        volume_mounts=[
                            V1VolumeMount(mount_path="/dev/net/tun", name="dev-tun")
                        ],
                        args=[
                            "/bin/sh",
                            "-c",
                            "./ran-emulator $INTERNAL_CONTROLLER_SERVICE_HOST $(hostname -I)",
                        ],
                    )
                ],
                volumes=[
                    V1Volume(
                        host_path=V1HostPathVolumeSource(
                            path="/dev/net/tun", type="CharDevice"
                        ),
                        name="dev-tun",
                    )
                ],
            ),
        )

    def start_user_input(self):
        self.user_input = UserInput(
            self.start_controller, self.kubernetes_restart, self.kubernetes_check_pods
        )
        self.user_input.run()

    def start_controller(
        self,
        controller_data: ControllerData,
        docker_image: str,
        epc: int,
        multiplexer: int,
        restart: bool,
        cp_mode: bool,
        num_threads: int,
        scale_minutes: int,
    ):
        self.controller_data = controller_data
        self.docker_image = docker_image
        self.epc = epc
        self.multiplexer = multiplexer
        self.cp_mode = cp_mode
        self.num_threads = num_threads
        self.scale_over_minutes = scale_minutes
        self.num_ues = len(self.controller_data["UEs"])
        self.enb_ips: list[int] = []

        self.should_keep_scaling.set()

        if restart == False:
            self.execute()
        else:
            self.kubernetes_t = threading.Thread(target=self.kubernetes_thread)
            self.kubernetes_t.start()

    def execute(self):
        # Create a socket
        self.sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

        # Bind the socket to the port
        server_address = ("0.0.0.0", 1234)
        self.sock.bind(server_address)

        # Create 3 threads
        receive_t = threading.Thread(target=self.receive_thread)
        self.kubernetes_t = threading.Thread(target=self.kubernetes_thread)

        # Start threads
        receive_t.start()
        self.kubernetes_t.start()

    def get_enb_by_buffer(self, data: bytes):
        num = (data[0] << 24) | (data[1] << 16) | (data[2] << 8) | data[3]
        print("Getting eNB with ID: %d" % num)
        for enb in self.controller_data["eNBs"]:
            if enb.get_id_int() == num:
                return enb
        return None

    def get_enb(self, data: bytes):
        num = (data[0] << 24) | (data[1] << 16) | (data[2] << 8) | data[3]
        print("Getting eNB with num: %d" % num)
        for enb in self.controller_data["eNBs"]:
            if enb.get_num() == num:
                return enb
        return None

    def get_ue_by_buffer(self, data: bytes):
        num = (data[0] << 24) | (data[1] << 16) | (data[2] << 8) | data[3]
        for ue in self.controller_data["UEs"]:
            if ue.get_id_int() == num:
                return ue
        return None

    def analyze_msg(self, msg: ReceivedMessage):
        print("\n\nMessage received:", msg["type"])

        if msg["type"] == "init" or msg["type"] == "cp_mode":
            print("New slave at " + msg["ip"] + ":" + str(msg["port"]))
            friendly_ip = str(socket.inet_ntoa(struct.pack("!L", msg["ip_node"])))
            print("Node IP: " + friendly_ip)

            # Get pod name from IP using k8s API
            if k8s == True:
                core_v1 = client.CoreV1Api()
                ret = core_v1.list_pod_for_all_namespaces(watch=False)
                for i in ret.items:
                    if getattr(i.status, "pod_ip") == friendly_ip:
                        print("Pod name: " + getattr(i.metadata, "name"))
                        break
                else:
                    print("Pod not found for IP")

            ######################
            # Critical section 1 #
            # Assign first eNBs  #
            ######################
            # Acquire the Global Lock
            self.lock.acquire()
            for enb in self.controller_data["eNBs"]:
                if (
                    enb.get_status() == Status.STOPPED
                    and msg["ip_node"] not in self.enb_ips
                ):
                    # Set pending
                    enb.set_pending()
                    # This is needed because of the MobileStream implementation
                    if mobilestream == True:
                        self.enb_ips.append(msg["ip_node"])
                    # This slave has to be a eNB
                    buf = enb.serialize(CODE_OK | CODE_ENB_BEHAVIOUR, self.epc)
                    self.sock.sendto(buf, (msg["ip"], msg["port"]))
                    print(
                        "eNB role assigned to Slave at "
                        + msg["ip"]
                        + ":"
                        + str(msg["port"])
                    )
                    # Release Global Lock
                    self.lock.release()
                    return
            # Release Global Lock
            self.lock.release()
            print("This slave cannot be an eNB (No eNB role available)")
            #############################
            # End of critical section 1 #
            #############################

            ###################################
            # Critical section 2              #
            # Multi-thread UE (CP Only Mode)  #
            ###################################
            # Acquire the Global Lock
            self.lock.acquire()
            if self.cp_mode == True and msg["type"] == "init":
                print("Trying new UE in CP Only mode...")
                if self.num_ues != -1:
                    # Calculate the number of threas
                    if self.num_ues <= self.num_threads:
                        num = self.num_ues
                        self.num_ues = -1
                    else:
                        num = self.num_threads
                        self.num_ues = self.num_ues - self.num_threads

                    print("Number of UEs for this container: " + str(num))
                    buf = bytearray()
                    buf.append(CODE_OK | CODE_CP_MODE_ONLY)
                    buf.append((num >> 24) & 0xFF)
                    buf.append((num >> 16) & 0xFF)
                    buf.append((num >> 8) & 0xFF)
                    buf.append(num & 0xFF)
                    self.sock.sendto(buf, (msg["ip"], msg["port"]))
                    print(
                        "Multi-threaded UE (Control-Plane Only) role assigned to Slave at "
                        + msg["ip"]
                        + ":"
                        + str(msg["port"])
                    )
                    # Release Global Lock
                    self.lock.release()
                    return
                print("This Slave is a thread in a UE CP Only mode")
            # Release Global Lock
            self.lock.release()
            #############################
            # End of critical section 1 #
            #############################

            ######################
            # Critical Section 3 #
            # Assign UE role     #
            ######################
            # Acquire the Global Lock
            self.lock.acquire()
            # Get the first not assigned UE
            for ue in self.controller_data["UEs"]:
                if ue.get_status() == Status.STOPPED:
                    ue.set_pending()
                    # At this point, only one thread can execute the following

                    # Release Global Lock
                    self.lock.release()

                    # Get associated eNB
                    assoc_enb = next(
                        (
                            enb
                            for enb in self.controller_data["eNBs"]
                            if enb.get_num() == ue.get_enb_id()
                        ),
                        None,
                    )

                    if assoc_enb is None:
                        print("ERROR: No eNB associated to the UE")
                        return

                    # Special race condition: eNB has been assigned but it does not already answer
                    while assoc_enb.get_status() != Status.CONNECTED:
                        time.sleep(1)

                    buf = ue.serialize(
                        CODE_OK | CODE_UE_BEHAVIOUR,
                        assoc_enb,
                        self.multiplexer,
                        self.epc,
                    )
                    self.sock.sendto(buf, (msg["ip"], msg["port"]))
                    print(
                        "UE role assigned to Slave at "
                        + msg["ip"]
                        + ":"
                        + str(msg["port"])
                    )
                    return
            # Release Global Lock
            self.lock.release()

            # Send error message to the slave
            print(
                "ERROR: No role available for the slave at "
                + msg["ip"]
                + ":"
                + str(msg["port"])
            )
            buf = bytearray()
            buf.append(CODE_ERROR)
            self.sock.sendto(buf, (msg["ip"], msg["port"]))
            return

        elif msg["type"] == "enb_run":
            # eNB slave answers with OK message
            # Get eNB from data
            enb = self.get_enb_by_buffer(msg["data"])
            if enb is not None:
                # Save eNB address
                enb.set_addr((msg["ip"], msg["port"]))
                enb.set_connected()  # Running/Disconnected
                print("New eNB at " + msg["ip"] + ":" + str(msg["port"]))

        # This is considered a critical section
        elif msg["type"] == "enb_error_run":
            # Acquire the Global Lock
            self.lock.acquire()
            # eNB slave answers with Error message
            # Get eNB from data
            enb = self.get_enb_by_buffer(msg["data"])
            if enb is not None:
                if mobilestream == True:
                    ip_node = (
                        (msg["data"][4] << 24)
                        | (msg["data"][5] << 16)
                        | (msg["data"][6] << 8)
                        | msg["data"][7]
                    )
                    self.enb_ips.remove(ip_node)
                enb.set_stopped()  # Stopped/Not Running
                print("eNB error at " + msg["ip"] + ":" + str(msg["port"]))
            # Release Global Lock
            self.lock.release()

        elif msg["type"] == "ue_run":
            # UE slave answers with OK message
            # Get UE from data
            ue = self.get_ue_by_buffer(msg["data"])
            if ue is not None:
                # Save UE address
                ue.set_addr((msg["ip"], msg["port"]))
                ue.set_traffic()
                self.controller_data["running_ues"] = (
                    self.controller_data["running_ues"] + 1
                )
                print(
                    "New UE ("
                    + ue.get_imsi()
                    + ") at "
                    + msg["ip"]
                    + ":"
                    + str(msg["port"])
                )

        # This is considered a critical section
        elif msg["type"] == "ue_error_run":
            # Acquire the Global Lock
            self.lock.acquire()
            # UE slave answers with Error message
            # Get UE from data
            ue = self.get_ue_by_buffer(msg["data"])
            if ue is not None:
                # Save UE address
                ue.set_stopped()
                ue.set_flag(False)
                print("UE error at " + msg["ip"] + ":" + str(msg["port"]))
            # Release Global Lock
            self.lock.release()

        elif msg["type"] == "ue_idle":
            # UE slave informs that its new state is Idle
            # Get UE from data
            ue = self.get_ue_by_buffer(msg["data"])
            if ue is not None:
                # Save UE address
                ue.set_idle()
                print(
                    "UE ("
                    + ue.get_imsi()
                    + ") at "
                    + msg["ip"]
                    + ":"
                    + str(msg["port"])
                    + " moved to Idle"
                )

        elif msg["type"] == "ue_detached":
            # UE slave informs that its new state is Detached
            # Get UE from data
            ue = self.get_ue_by_buffer(msg["data"])
            if ue is not None:
                # Save UE address
                ue.set_disconnected()
                print(
                    "UE ("
                    + ue.get_imsi()
                    + ") at "
                    + msg["ip"]
                    + ":"
                    + str(msg["port"])
                    + " moved to Detached"
                )

        elif msg["type"] == "ue_attached":
            # UE slave informs that its new state is Attached/Connected
            # Get UE from data
            ue = self.get_ue_by_buffer(msg["data"])
            if ue is not None:
                # Save UE address
                ue.set_traffic()
                print(
                    "UE ("
                    + ue.get_imsi()
                    + ") at "
                    + msg["ip"]
                    + ":"
                    + str(msg["port"])
                    + " moved to Attached"
                )

        elif msg["type"] == "moved_to_connected":
            # UE slave informs that its new state is Attached/Connected
            # Get UE from data
            ue = self.get_ue_by_buffer(msg["data"])
            if ue is not None:
                # Save UE address
                ue.set_traffic()
                print(
                    "UE ("
                    + ue.get_imsi()
                    + ") at "
                    + msg["ip"]
                    + ":"
                    + str(msg["port"])
                    + " moved to Connected"
                )

        elif msg["type"] == "get_enb":

            # Get UE from data
            ue = self.get_ue_by_buffer(msg["data"][:4])  # First 4 bytes
            if ue is None:
                print("ERROR: UE not found")
                return
            # Get eNB from data
            enb = self.get_enb(msg["data"][4:])  # Last 4 bytes
            buf = bytearray()
            if enb == None:
                # Wrong eNB case
                # Generate Error message
                print(
                    "UE ("
                    + ue.get_imsi()
                    + ") at "
                    + msg["ip"]
                    + ":"
                    + str(msg["port"])
                    + " requests a wrong eNB Number"
                )
                buf.append(CODE_UE_GET_ENB)
            else:
                # Correct eNB case
                print(
                    "UE ("
                    + ue.get_imsi()
                    + ") at "
                    + msg["ip"]
                    + ":"
                    + str(msg["port"])
                    + " requests eNB "
                    + str(enb.get_num())
                    + " IP address"
                )
                buf.append(CODE_OK | CODE_UE_GET_ENB)
                # Add eNB IP
                try:
                    ip = struct.unpack("!I", socket.inet_aton(enb.get_address()[0]))[0]  # type: ignore
                    buf.append((ip >> 24) & 0xFF)
                    buf.append((ip >> 16) & 0xFF)
                    buf.append((ip >> 8) & 0xFF)
                    buf.append(ip & 0xFF)
                except:
                    buf.append(0)
                    buf.append(0)
                    buf.append(0)
                    buf.append(0)
            self.sock.sendto(buf, (msg["ip"], msg["port"]))

        elif msg["type"] == "x2_completed":
            # Get UE from data
            ue = self.get_ue_by_buffer(msg["data"][:4])  # First 4 bytes
            if ue is not None:
                # Get eNB from data
                enb = self.get_enb(msg["data"][4:])  # Last 4 bytes
                if enb is None:
                    print("ERROR: eNB not found")
                    return

                # Update UE info
                ue.set_enb_id(enb.get_num())
                print(
                    "UE ("
                    + ue.get_imsi()
                    + ") at "
                    + msg["ip"]
                    + ":"
                    + str(msg["port"])
                    + " X2 Handover completed to eNB "
                    + str(enb.get_num())
                )

        elif msg["type"] == "ue_attached_to_enb":
            # Get UE from data
            ue = self.get_ue_by_buffer(msg["data"][:4])  # First 4 bytes
            if ue is not None:
                # Get eNB from data
                enb = self.get_enb(msg["data"][4:])  # Last 4 bytes
                if enb is None:
                    print("ERROR: eNB not found")
                    return

                # Update UE info
                ue.set_enb_id(enb.get_num())
                ue.set_traffic()
                print(
                    "UE ("
                    + ue.get_imsi()
                    + ") at "
                    + msg["ip"]
                    + ":"
                    + str(msg["port"])
                    + " Attached to eNB "
                    + str(enb.get_num())
                )

        elif msg["type"] == "s1_completed":
            # Get UE from data
            ue = self.get_ue_by_buffer(msg["data"][:4])  # First 4 bytes
            if ue is not None:
                # Get eNB from data
                enb = self.get_enb(msg["data"][4:])  # Last 4 bytes
                if enb is None:
                    print("ERROR: eNB not found")
                    return

                # Update UE info
                ue.set_enb_id(enb.get_num())
                print(
                    "UE ("
                    + ue.get_imsi()
                    + ") at "
                    + msg["ip"]
                    + ":"
                    + str(msg["port"])
                    + " S1 Handover completed to eNB "
                    + str(enb.get_num())
                )

    def generate_msg(
        self, data: bytes, address: tuple[str, int]
    ) -> Optional[ReceivedMessage]:
        if data[0] == CODE_INIT:
            return {
                "type": "init",
                "ip": address[0],
                "port": address[1],
                "ip_node": (
                    (data[1] << 24) | (data[2] << 16) | (data[3] << 8) | data[4]
                ),
            }
        elif data[0] == CODE_CP_MODE_ONLY:
            return {
                "type": "cp_mode",
                "ip": address[0],
                "port": address[1],
                "ip_node": (
                    (data[1] << 24) | (data[2] << 16) | (data[3] << 8) | data[4]
                ),
            }
        elif data[0] == CODE_OK | CODE_ENB_BEHAVIOUR:
            return {
                "type": "enb_run",
                "ip": address[0],
                "port": address[1],
                "data": data[1:],
            }
        elif data[0] == CODE_OK | CODE_UE_BEHAVIOUR:
            return {
                "type": "ue_run",
                "ip": address[0],
                "port": address[1],
                "data": data[1:],
            }
        elif data[0] == CODE_UE_IDLE:
            return {
                "type": "ue_idle",
                "ip": address[0],
                "port": address[1],
                "data": data[1:],
            }
        elif data[0] == CODE_UE_DETACH:
            return {
                "type": "ue_detached",
                "ip": address[0],
                "port": address[1],
                "data": data[1:],
            }
        elif data[0] == CODE_UE_ATTACH:
            return {
                "type": "ue_attached",
                "ip": address[0],
                "port": address[1],
                "data": data[1:],
            }
        elif data[0] == CODE_UE_MOVED_TO_CONNECTED:
            return {
                "type": "moved_to_connected",
                "ip": address[0],
                "port": address[1],
                "data": data[1:],
            }
        elif data[0] == CODE_UE_GET_ENB:
            return {
                "type": "get_enb",
                "ip": address[0],
                "port": address[1],
                "data": data[1:],
            }
        elif data[0] == CODE_UE_X2_HANDOVER_COMPLETED:
            return {
                "type": "x2_completed",
                "ip": address[0],
                "port": address[1],
                "data": data[1:],
            }
        elif data[0] == CODE_UE_ATTACH_TO_ENB:
            return {
                "type": "ue_attached_to_enb",
                "ip": address[0],
                "port": address[1],
                "data": data[1:],
            }
        elif data[0] == CODE_UE_S1_HANDOVER_COMPLETED:
            return {
                "type": "s1_completed",
                "ip": address[0],
                "port": address[1],
                "data": data[1:],
            }

        # Error Init cases
        elif data[0] == CODE_ENB_BEHAVIOUR:
            return {
                "type": "enb_error_run",
                "ip": address[0],
                "port": address[1],
                "data": data[1:],
            }

        elif data[0] == CODE_UE_BEHAVIOUR:
            return {
                "type": "ue_error_run",
                "ip": address[0],
                "port": address[1],
                "data": data[1:],
            }

        # Error code: 0XXX XXXX
        elif (data[0] & CODE_OK) == CODE_ERROR:
            return {
                "type": "error",
                "ip": address[0],
                "port": address[1],
            }

        else:
            return None

    def receive_thread(self):
        print("Init receiver thread")
        while 1:
            data, address = self.sock.recvfrom(16384)
            msg = self.generate_msg(data, address)
            worker = threading.Thread(target=self.analyze_msg, args=(msg,))
            worker.start()

    def kubernetes_thread(self):
        # Wait 2 seconds to the rest of the threads
        time.sleep(2)

        enb_pods = len(self.controller_data["eNBs"])
        ue_pods = (
            math.ceil(len(self.controller_data["UEs"]) / self.num_threads)
            if self.cp_mode
            else len(self.controller_data["UEs"])
        )

        if not k8s:
            print("Kubernetes not enabled")
            return

        # Init Kubernetes API connection
        v1 = client.CoreV1Api()

        print("Staring eNB Slave Pods...")
        for i in range(enb_pods):
            # Configure POD Manifest for each slave
            self.pod_manifest.metadata.name = "slave-enb-" + str(i)  # type: ignore
            self.pod_manifest.spec.containers[0].image = self.docker_image  # type: ignore
            self.pod_manifest.spec.containers[0].name = "slave-enb-" + str(i)  # type: ignore
            # Create a POD
            v1.create_namespaced_pod(body=self.pod_manifest, namespace="default")

        print("Staring UE Slave Pods...")
        # Scale incrementally if required
        for i in range(ue_pods):
            if not self.should_keep_scaling.is_set():
                # Stop creating pods anymore if the experiment is restarted.
                # Promptly exit the thread
                break

            # Configure POD Manifest for each slave
            self.pod_manifest.metadata.name = "slave-ue-" + str(i)  # type: ignore
            self.pod_manifest.spec.containers[0].image = self.docker_image  # type: ignore
            self.pod_manifest.spec.containers[0].name = "slave-ue-" + str(i)  # type: ignore
            # Create a POD
            v1.create_namespaced_pod(body=self.pod_manifest, namespace="default")

            # Wait for the next increment if required
            if self.scale_over_minutes != 0:
                time.sleep(self.scale_over_minutes * 60 / ue_pods)

        self.should_keep_scaling.set()
        print("Slave pods started!")
        return

    def kubernetes_restart(self):
        if hasattr(self, "kubernetes_t") and self.kubernetes_t.is_alive():
            # If still scaling previous one, stop it
            self.should_keep_scaling.clear()

            # Wait until the thread notices and resets the signal, so we can start
            # deleting the pods without any new ones popping up
            self.should_keep_scaling.wait()
        else:
            # The scaling thread is not running, it's finished scaling. So we
            # can proceed to delete all the pods. It might not be necessary
            # but we'll reset the signal to ON just in case so the next run will
            # scale properly
            self.should_keep_scaling.set()

        # Remove all eNB IPs
        self.enb_ips = []

        # Delete all pods with the label app=ran-slave
        if k8s:
            core_v1 = client.CoreV1Api()
            delete_options = client.V1DeleteOptions()
            ret = core_v1.list_pod_for_all_namespaces(watch=False)
            for i in ret.items:
                if getattr(i.metadata, "labels").get("app", "") == "ran-slave":
                    try:
                        core_v1.delete_namespaced_pod(
                            name=getattr(i.metadata, "name"),
                            namespace="default",
                            body=delete_options,
                        )
                    except Exception as e:
                        print("Error deleting pod: ", getattr(i.metadata, "name"))
                        print("Suprressing (stack trace): ", e)

            print("Pods removed!")
        return

    def kubernetes_check_pods(self):
        slaves = 0
        if k8s == True:
            core_v1 = client.CoreV1Api()
            ret = core_v1.list_namespaced_pod("default")
            for i in ret.items:
                print(
                    "%s\t%s\t%s"
                    % (
                        getattr(i.status, "pod_ip"),
                        getattr(i.metadata, "namespace"),
                        getattr(i.metadata, "name"),
                    )
                )
                if "slave-" in getattr(i.metadata, "name"):
                    slaves += 1
        print("Number of Slave pods:", slaves)
        return slaves


if __name__ == "__main__":
    ran_controler = RANControler()
    ran_controler.start_user_input()

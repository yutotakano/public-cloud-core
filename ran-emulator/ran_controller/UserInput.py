from flask import Flask, render_template, url_for, redirect, request
from flask_socketio import SocketIO
import json
import struct
import socket
from UE import UE
from eNB import eNB
from threading import Thread, Event
import datetime
from typing import Optional, TypedDict, Callable, Any
import werkzeug


class ControllerData(TypedDict):
    eNBs: set[eNB]
    UEs: set[UE]
    running_ues: int


class UserInput:

    controller_data: ControllerData = {
        "eNBs": set(),
        "UEs": set(),
        "running_ues": 0,
    }

    actions = {
        "init": [0, 0],
        "detach": [1, 1],
        "detach_switch_off": [1, 2],
        "attach": [-1, 3],
        "move_to_idle": [3, 4],
        "move_to_connected": [-3, 5],
    }

    special_actions = {
        "x2handover": [0, 6],
        "attach": [-1, 7],
        "s1handover": [0, 8],
    }

    def __init__(
        self,
        set_data_func: Callable[
            [ControllerData, str, int, int, bool, bool, int, int], None
        ],
        restart: Callable[[], None],
        check_pods: Callable[[], int],
    ):
        self.app = Flask(__name__)
        # Disable Flask logs
        # log = logging.getLogger('werkzeug')
        # log.disabled = True
        # self.app.logger.disabled = True

        self.configuration = True
        self.restarted = False
        self.num_of_running_pods: int = 0
        self.set_data_func = set_data_func
        self.restart = restart
        # Async socket
        self.socketio = SocketIO(self.app, async_mode=None)
        # Async thread
        self.update_t = Thread()
        self.thread_stop_event = Event()
        self.refresh_time = 5  # Default refresh time
        self.scale_minutes = 0  # By default, start all UEs immediately
        self.check_pods = check_pods

    def validate_control_plane_actions(self, actions_string: str):
        a_s = actions_string.split("-")
        counter = 0
        numFlag = False

        if a_s[0] != "init":
            return False

        for a in a_s[1:]:
            if a.isdigit() or a == "inf":
                if numFlag == False:
                    numFlag = True
                else:
                    return False
            else:
                if a == "init":
                    return False
                try:
                    counter += self.actions[a][0]
                except:
                    # Handover case
                    if (
                        a.startswith("x2handover") == False
                        and a.startswith("attach") == False
                        and a.startswith("s1handover") == False
                    ):
                        return False
                    # Handover case
                    ho = a.split("_")
                    if (
                        len(ho) != 2
                        or (
                            ho[0] != "x2handover"
                            and ho[0] != "attach"
                            and ho[0] != "s1handover"
                        )
                        or ho[1].isdigit() == False
                    ):
                        return False
                    counter += self.special_actions[ho[0]][0]
                if counter != 0 and counter % 2 == 0:
                    return False
                if numFlag == False:
                    return False

                numFlag = False

        if (counter != 0 and a_s[-1] != "inf") or numFlag == False:
            return False
        return True

    def serialize_control_plane(self, actions_strings: str):
        a_s = actions_strings.split("-")
        data = bytearray()
        # Special case with INF
        for i in range(0, len(a_s), 2):
            try:
                act = self.actions[a_s[i]]
                data.append(act[1] & 0xFF)  # Append the action (1 Byte)
            except:
                # Reusing handover structure for attach_to_enb
                ho = a_s[i].split("_")
                data.append(self.special_actions[ho[0]][1])  # Append Handover
                enb_id = int(ho[1])  # Get Handover target eNB
                # Append eNB Number
                data.append((enb_id >> 16) & 0xFF)
                data.append((enb_id >> 8) & 0xFF)
                data.append(enb_id & 0xFF)
                # Append 0 to do nothing in the Control Plane FSM
                data.append(0)

            # Append the delay time (3 Bytes)
            # Special case inf: 0xFF 0xFF 0xFF
            if a_s[i + 1] == "inf":
                data.append(0xFF)
                data.append(0xFF)
                data.append(0xFF)
                break
            else:
                data.append((int(a_s[i + 1]) >> 16) & 0xFF)
                data.append((int(a_s[i + 1]) >> 8) & 0xFF)
                data.append(int(a_s[i + 1]) & 0xFF)
        return data

    def add_endpoint(
        self,
        endpoint: str,
        endpoint_name: Optional[str] = None,
        handler: Any = None,
        methods: Optional[list[str]] = None,
    ):
        self.app.add_url_rule(endpoint, endpoint_name, handler, methods=methods)

    def getENB(self, enbs: set[eNB], num: int):
        for enb in enbs:
            if enb.get_num() == num:
                return enb
        return None

    def generate_data(
        self,
        config: werkzeug.datastructures.FileStorage,
        docker_image: str,
        epc_ip: str,
        multi_ip: str,
        cp_mode: bool,
        num_threads: int,
        scale_minutes: int,
    ):
        data = []
        self.epc_ip = epc_ip
        self.multi_ip = multi_ip
        self.docker_image = docker_image
        self.scale_minutes = scale_minutes
        self.scale_end_time = datetime.datetime.now() + datetime.timedelta(
            minutes=self.scale_minutes
        )
        try:
            # Convert IPs to unsigned int 4 bytes in length
            epc: int = struct.unpack("!I", socket.inet_aton(epc_ip))[0]
            multiplexer: int = struct.unpack("!I", socket.inet_aton(multi_ip))[0]
        except:
            return False

        if config:
            file_content = config.read()
            data = json.loads(file_content)
        else:
            return False

        for enb in data["enbs"]:
            new_enb = eNB(enb["enb_num"], enb["enb_mcc"], enb["enb_mnc"])
            if self.check_if_enb_exists(enb["enb_num"]) == False:
                # Add it to the final eNB list
                self.controller_data["eNBs"].add(new_enb)

        i = 1
        for ue in data["ues"]:
            new_ue = UE(
                i,
                ue["ue_mcc"],
                ue["ue_mnc"],
                ue["ue_msin"],
                ue["ue_key"],
                ue["ue_op_key"],
                ue["traffic_command"],
                ue["enb"],
                ue["control_plane"],
            )
            # Get eNB
            enb = self.getENB(self.controller_data["eNBs"], ue["enb"])
            if enb and self.validate_control_plane_actions(new_ue.get_control_plane()):
                # Add serialized control plane to the UE
                new_ue.set_serialized_control_plane(
                    self.serialize_control_plane(new_ue.get_control_plane())
                )
                if self.check_if_ue_exists(ue["ue_msin"]) == False:
                    # Add UE to the final UE list
                    self.controller_data["UEs"].add(new_ue)
                    i = i + 1

        self.set_data_func(
            self.controller_data,
            self.docker_image,
            epc,
            multiplexer,
            self.restarted,
            cp_mode,
            num_threads,
            scale_minutes,
        )

        return True

    def check_if_enb_exists(self, num: int):
        for enb in self.controller_data["eNBs"]:
            if enb.get_num() == num:
                return True
        return False

    def check_if_ue_exists(self, msin: str):
        for ue in self.controller_data["UEs"]:
            if ue.get_msin() == msin:
                return True
        return False

    def index(self):
        # If there is a Slave pod running, print an error
        if self.num_of_running_pods > 0:
            print(
                "Error: There are "
                + str(self.num_of_running_pods)
                + " Slave pods running. Redirecting to the configuration panel"
            )
            errorMessage = {
                "onload": 'onload="alertSlaves()"',
                "code": '<script>function alertSlaves(){alert("There are '
                + str(self.num_of_running_pods)
                + ' Pods running from previous experiments. Please wait until the automatic deletion.");}</script>',
            }
            return render_template("config.html", **errorMessage)

        if self.configuration:
            return render_template("config.html")
        else:
            # If the configuration and the MME IP have been received
            templateData = {
                "ue_table": self.ues_to_html(),
                "enb_table": self.enbs_to_html(),
                "running_ues": str(self.controller_data["running_ues"]),
                "mme_ip": self.epc_ip,
                "multi_ip": self.multi_ip,
                "docker_image": self.docker_image,
            }
            return render_template("index.html", **templateData)

    def config(self):
        if request.method == "POST":
            mme_ip = request.form["mme_ip"]
            multi_ip = request.form["multi_ip"]
            if multi_ip == "":
                multi_ip = mme_ip
            config = request.files.get("config")
            docker_image = request.form["docker_image"]
            try:
                cp_mode = bool(request.form["cp_mode"])
            except:
                cp_mode = False
            num_threads = int(request.form["threads"])
            if cp_mode == True:
                print(
                    "Control-Plane Only mode: "
                    + str(num_threads)
                    + " threads per container"
                )
            scale_minutes = int(request.form["scale_minutes"])
            self.refresh_time = int(request.form["refresh_time"])
            # Get the number of Slave pods
            self.num_of_running_pods = self.check_pods()
            # If the data has been validated
            if self.generate_data(
                config,
                docker_image,
                mme_ip,
                multi_ip,
                cp_mode,
                num_threads,
                scale_minutes,
            ):
                self.configuration = False
        return redirect(url_for("index"))

    def restart_experiment(self):
        print("Restarting experiment...")
        self.restart()
        self.controller_data["UEs"] = set()
        self.controller_data["eNBs"] = set()
        self.controller_data["running_ues"] = 0
        self.configuration = True
        self.restarted = True
        return redirect(url_for("index"))

    def get_number_pods(self):
        self.num_of_running_pods = self.check_pods()
        return {
            "num_pods": self.num_of_running_pods,
        }

    def updateThread(self):
        print("Starting web user updater thread")
        while not self.thread_stop_event.isSet():
            # Generate data
            data = (
                "<h4> Number of UEs running: "
                + str(self.controller_data["running_ues"])
                + " </h4>"
            )
            data += (
                "<h4> Incremental Scaling Target: "
                + str(len(self.controller_data["UEs"]))
                + " UEs @ "
                + self.scale_end_time.strftime("%H:%M")
                + "UTC</h4>"
            )
            data += "<h3>UEs</h3>"
            data += self.ues_to_html()
            data += "<h3>eNBs/gNBs</h3>"
            data += self.enbs_to_html()

            self.socketio.emit("newdata", {"data": data}, namespace="/update")
            self.socketio.sleep(self.refresh_time)

    def update(self):
        # Create a thread that updates the web client every N seconds
        print("New web client!")
        if not self.update_t.is_alive():
            self.update_t = self.socketio.start_background_task(self.updateThread)

    def run(self):
        # Main routes
        self.add_endpoint(endpoint="/", endpoint_name="index", handler=self.index)
        self.add_endpoint(
            endpoint="/config/",
            endpoint_name="config",
            handler=self.config,
            methods=["POST"],
        )
        self.add_endpoint(
            endpoint="/restart/",
            endpoint_name="restart",
            handler=self.restart_experiment,
        )
        self.add_endpoint(
            endpoint="/pods/",
            endpoint_name="pods",
            handler=self.get_number_pods,
        )

        # Async routes
        self.socketio.on_event("connect", self.update, namespace="/update")

        #
        self.socketio.run(
            app=self.app, port=8080, host="0.0.0.0", allow_unsafe_werkzeug=True
        )

    def ues_to_html(self) -> str:
        html: str = '<div style="overflow:scroll; height:200px; border:solid;">'
        html += '<table style="width:100%">'
        html += "<tr>"
        html += "<th>id</th>"
        html += "<th>IMSI</th>"
        html += "<th>Key</th>"
        html += "<th>OpKey</th>"
        html += "<th>Control plane</th>"
        html += "<th>Command</th>"
        html += "<th>Address</th>"
        html += "<th>eNB/gNB</th>"
        html += "<th>Status</th>"
        html += "</tr>"

        for ue in self.controller_data["UEs"]:
            html += "<tr>"
            html += "<td>" + ue.get_id() + "</td>"
            html += "<td>" + ue.get_imsi() + "</td>"
            html += "<td>" + ue.get_key() + "</td>"
            html += "<td>" + ue.get_op_key() + "</td>"
            html += "<td>" + ue.get_control_plane() + "</td>"
            html += "<td>" + ue.get_command() + "</td>"
            html += "<td>" + ue.get_address_text() + "</td>"
            html += "<td>" + str(ue.get_enb_id()) + "</td>"
            html += "<td>" + ue.get_html_status() + "</td>"
            html += "</tr>"
        html += "</table>"
        html += "</div>"
        return html

    def enbs_to_html(self):
        html = '<div style="overflow:scroll; height:200px; border:solid;">'
        html += '<table style="width:100%">'
        html += "<tr>"
        html += "<th>Number</th>"
        html += "<th>ID</th>"
        html += "<th>MCC</th>"
        html += "<th>MNC</th>"
        html += "<th>Address</th>"
        html += "<th>Status</th>"
        html += "</tr>"
        for enb in self.controller_data["eNBs"]:
            html += "<tr>"
            html += "<td>" + str(enb.get_num()) + "</td>"
            html += "<td>" + enb.get_id() + "</td>"
            html += "<td>" + enb.get_mcc() + "</td>"
            html += "<td>" + enb.get_mnc() + "</td>"
            html += "<td>" + enb.get_address_text() + "</td>"
            html += "<td>" + enb.get_html_status() + "</td>"
            html += "</tr>"
        html += "</table>"
        html += "</div>"
        return html

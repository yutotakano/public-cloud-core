from prometheus_client import start_http_server, Summary

INVALID = Summary(
    "message_invalid",
    "Whether the message received was invalid",
    labelnames=("ue_id", "message_type"),
)
LATENCY = Summary(
    "message_amf_latency",
    "Latency between AMF receiving messages and finishing to send them",
    labelnames=("ue_id", "message_type"),
)
DECODE_LATENCY = Summary(
    "message_amf_decode_latency",
    "Latency between AMF receiving messages and decoding them",
    labelnames=("ue_id", "message_type"),
)
HANDLE_LATENCY = Summary(
    "message_amf_handle_latency",
    "Latency between AMF decoding messages and finishing building responses",
    labelnames=("ue_id", "message_type"),
)
ENCODE_LATENCY = Summary(
    "message_amf_encode_latency",
    "Latency between AMF finishing building responses and encoding them",
    labelnames=("ue_id", "message_type"),
)
SEND_LATENCY = Summary(
    "message_amf_send_latency",
    "Latency between AMF encoding messages and sending them",
    labelnames=("ue_id", "message_type"),
)
NUM_RESPONSES = Summary(
    "message_amf_responses",
    "Number of responses sent by AMF",
    labelnames=("ue_id", "message_type"),
)

metrics = {
    b"invalid": INVALID,
    b"latency": LATENCY,
    b"decode_latency": DECODE_LATENCY,
    b"handle_latency": HANDLE_LATENCY,
    b"encode_latency": ENCODE_LATENCY,
    b"send_latency": SEND_LATENCY,
    b"num_responses": NUM_RESPONSES,
}


def start_server():
    start_http_server(8000)
    while True:
        pass

import socket
import threading
import logging

from prometheus_client import Summary

logger = logging.getLogger(__name__)


def start_server(metrics: dict[bytes, Summary]):
    server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    server_socket.bind(("0.0.0.0", 7799))
    server_socket.listen(5)

    logger.info("Server started. Listening on port 7799...")

    while True:
        client_socket = None
        try:
            client_socket, address = server_socket.accept()
            logger.info(f"New connection from {address[0]}:{address[1]}")

            client_thread = threading.Thread(
                target=handle_client,
                args=(
                    client_socket,
                    metrics,
                ),
                daemon=True,
            )
            client_thread.start()
        except KeyboardInterrupt:
            logger.info("Stopping server...")
            if client_socket:
                client_socket.close()
            break


def handle_client(client_socket: socket.socket, metrics: dict[bytes, Summary]):
    message_buffer = b""
    while True:
        data = client_socket.recv(1024)
        if not data:
            break
        message_buffer += data

        if b"\n" in message_buffer:
            lines = message_buffer.split(b"\n")
            for line in lines[:-1]:
                handle_message(line, metrics)
            message_buffer = lines[-1]

    client_socket.close()


def handle_message(message: bytes, metrics: dict[bytes, Summary]):
    logger.info(f"Received message: {message}")
    parts = message.split(b"|")
    collected_metrics: dict[bytes, int] = {}
    for part in parts:
        key, value = part.split(b":")
        collected_metrics[key] = int(value)

    for key in metrics.keys():
        if key in collected_metrics:
            metrics[key].labels(
                ran_ue_ngap_id=collected_metrics[b"ran_ue_ngap_id"],
                amf_ue_ngap_id=collected_metrics[b"amf_ue_ngap_id"],
                message_type=collected_metrics[b"message_type"],
            ).observe(int(collected_metrics[key]))

    logger.info(f"Metrics: {metrics}")

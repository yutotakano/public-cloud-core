import logging
import threading

from core_metrics_aggregator import receiver
from core_metrics_aggregator import exporter

logging.basicConfig(
    level=logging.DEBUG,
    format="%(asctime)s %(levelname)s %(threadName)s %(message)s",
    datefmt="%Y-%m-%d %H:%M:%S",
)

if __name__ == "__main__":
    logging.debug("Starting server...")
    threading.Thread(target=exporter.start_server, daemon=True).start()
    receiver.start_server(exporter.metrics)

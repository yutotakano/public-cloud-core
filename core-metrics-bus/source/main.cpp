#include "asio.hpp"
#include "prometheus/counter.h"
#include "prometheus/exposer.h"
#include "prometheus/histogram.h"
#include "prometheus/registry.h"
#include "prometheus/summary.h"
#include <iostream>
#include <variant>

using MetricFamilyVariant = std::variant<
  prometheus::Family<prometheus::Counter> &,
  prometheus::Family<prometheus::Gauge> &,
  prometheus::Family<prometheus::Histogram> &,
  prometheus::Family<prometheus::Summary> &>;

// Forward-declare the function that will process incoming messages
void process_metric_definition(
  asio::ip::tcp::socket socket,
  std::shared_ptr<prometheus::Exposer> exposer,
  std::shared_ptr<
    std::unordered_map<std::string, std::shared_ptr<MetricFamilyVariant>>>
    metrics
);

void process_metric_batch_observation(
  asio::ip::tcp::socket socket,
  std::shared_ptr<prometheus::Exposer> exposer,
  std::shared_ptr<
    std::unordered_map<std::string, std::shared_ptr<MetricFamilyVariant>>>
    metrics
);

std::mutex registries_mutex;
void threadWorker(
  asio::ip::tcp::socket socket,
  std::shared_ptr<prometheus::Exposer> exposer,
  std::shared_ptr<
    std::unordered_map<std::string, std::shared_ptr<MetricFamilyVariant>>>
    metrics
)
{
  std::cout << "Accepted connection from " << socket.remote_endpoint()
            << std::endl;
  // For the header, read it into an unsigned array or otherwise overflows over
  // 127 will become negative
  std::array<uint8_t, 16> header_buffer;
  std::array<char, 1024> buffer;
  std::size_t bytes_transferred;
  while (true)
  {
    try
    {
      // Wait until we see a two-byte sequence of 0x99 0x99.
      // We process the bytes one by one to avoid missing the sequence by
      // reading two bytes at once and missing the sequence boundary.
      bytes_transferred = asio::read(socket, asio::buffer(header_buffer, 1));
      if (bytes_transferred == 0)
      {
        std::cout << "0 bytes transferred" << std::endl;
        continue;
      }
      if (header_buffer[0] != 0x99)
      {
        continue;
      }

      bytes_transferred = asio::read(socket, asio::buffer(header_buffer, 1));
      if (bytes_transferred == 0)
      {
        std::cout << "0 bytes transferred" << std::endl;
        continue;
      }
      if (header_buffer[0] != 0x99 && header_buffer[0] != 0x98)
      {
        continue;
      }

      // If it was 0x99 0x99, we have a metric batch observation message.
      // If it was 0x99 0x98, we have a metric definition message.
      if (header_buffer[0] == 0x98)
      {
        std::cout << "Received metric definition message" << std::endl;
        process_metric_definition(std::move(socket), exposer, metrics);
      }
      else
      {
        std::cout << "Received metric batch observation message" << std::endl;
        process_metric_batch_observation(std::move(socket), exposer, metrics);
      }
    }
    catch (std::system_error &e)
    {
      std::error_code error = e.code();
      if (error == asio::error::eof)
      {
        std::cout << "Connection closed by peer (EOF received)" << std::endl;
      }
      else
      {
        std::cerr << "Exception: " << e.what() << std::endl;
      }
      return;
    }
  }
}

void process_metric_definition(
  asio::ip::tcp::socket socket,
  std::shared_ptr<prometheus::Exposer> exposer,
  std::shared_ptr<
    std::unordered_map<std::string, std::shared_ptr<MetricFamilyVariant>>>
    metrics
)
{
  // Read length of remaining message
  size_t bytes_transferred = 0;

  std::array<uint8_t, 16> header_buffer;
  std::array<char, 1024> buffer;

  // Read the third byte of the message to get the length of the body
  bytes_transferred = asio::read(socket, asio::buffer(header_buffer, 1));
  if (bytes_transferred == 0)
  {
    std::cout << "0 bytes transferred" << std::endl;
    return;
  }

  // Parse the length of the body
  std::uint8_t body_length = static_cast<std::uint8_t>(header_buffer[0]);
  std::cout << "Body length: " << unsigned(body_length) << std::endl;

  // Read the body
  bytes_transferred = asio::read(socket, asio::buffer(buffer, body_length));
  if (bytes_transferred == 0)
  {
    std::cout << "0 bytes transferred" << std::endl;
    return;
  }
  std::cout << "Received " << bytes_transferred << " bytes" << std::endl;

  // Process the body
  std::string message(buffer.data(), bytes_transferred);
  std::istringstream message_stream(message);
  std::string line;

  // Loop through all lines of the message
  while (std::getline(message_stream, line))
  {
    std::cout << "Received line: " << line << std::endl;
    std::istringstream line_stream(line);
    std::string metric;

    std::string metric_name;
    std::string metric_description;

    // Loop through all parts within the line
    while (std::getline(line_stream, metric, '|'))
    {
      // Process each metric
      std::istringstream metric_stream(metric);
      std::string metric_name;
      std::string metric_value;
      std::getline(metric_stream, metric_name, ':');
      std::getline(metric_stream, metric_value);

      if (metric_name == "metric_name")
      {
        metric_name = metric_value;
      }
      else if (metric_name == "metric_description")
      {
        metric_description = metric_value;
      }
    }

    // Register the metric
    register_metric(metrics, exposer, metric_name, metric_description);
  }
}

void process_metric_batch_observation(
  asio::ip::tcp::socket socket,
  std::shared_ptr<prometheus::Exposer> exposer,
  std::shared_ptr<
    std::unordered_map<std::string, std::shared_ptr<MetricFamilyVariant>>>
    metrics
)
{
  // Read length of remaining message
  size_t bytes_transferred = 0;

  std::array<uint8_t, 16> header_buffer;
  std::array<char, 1024> buffer;

  // Read the third byte of the message to get the length of the header
  bytes_transferred = asio::read(socket, asio::buffer(header_buffer, 1));
  if (bytes_transferred == 0)
  {
    std::cout << "0 bytes transferred" << std::endl;
    return;
  }

  // Parse the length of header
  std::uint8_t header_length = static_cast<std::uint8_t>(header_buffer[0]);
  std::cout << "Header length: " << unsigned(header_length) << std::endl;

  // Read the header
  bytes_transferred =
    asio::read(socket, asio::buffer(header_buffer, header_length));
  if (bytes_transferred == 0)
  {
    std::cout << "0 bytes transferred" << std::endl;
    return;
  }
  std::cout << "Buffer (hex): ";
  for (std::size_t i = 0; i < header_length; i++)
  {
    std::cout << std::hex << unsigned(header_buffer[i]) << " ";
  }
  std::cout << std::dec << std::endl;

  // Parse the length of the message from the header
  std::size_t message_length = 0;
  for (std::size_t i = 0; i < header_length; i++)
  {
    message_length |= static_cast<std::size_t>(header_buffer[i]) << (i * 8);
  }
  std::cout << "Message length: " << unsigned(message_length) << std::endl;

  // Read the remaining bytes of the message
  bytes_transferred = asio::read(socket, asio::buffer(buffer, message_length));
  if (bytes_transferred == 0)
  {
    std::cout << "0 bytes transferred" << std::endl;
    return;
  }
  std::cout << "Received " << bytes_transferred << " bytes" << std::endl;

  // Process the body
  std::string message(buffer.data(), bytes_transferred);
  // Each line of the message is  metric in the format:
  // ue_id:value|message_type:value|metric_key1:metric_value1|metric_key2:metric_value2|...
  std::string message(buffer.data(), bytes_transferred);
  std::istringstream message_stream(message);
  std::string line;

  while (std::getline(message_stream, line))
  {
    std::cout << "Received line: " << line << std::endl;
    std::istringstream line_stream(line);
    std::string metric;

    std::string ue_id;
    std::string ngap_message_type;
    std::string nas_message_type;

    // Increment the uplink packet counter for every metric batch observation
    // TODO: change this to a regular metric itself
    if (auto packet_counter = std::get_if<prometheus::Family<prometheus::Counter>>(
          metrics->at("uplink_packets").get()
        ))
    {
      packet_counter->Add({}).Increment();
    }

    while (std::getline(line_stream, metric, '|'))
    {
      // Process each metric
      std::istringstream metric_stream(metric);
      std::string metric_name;
      std::string metric_value;
      std::getline(metric_stream, metric_name, ':');
      std::getline(metric_stream, metric_value);

      // Get the right counter for the metric
      if (metrics->find(metric_name) == metrics->end())
      {
        std::cerr << "Metric " << metric_name << " not found, ignoring."
                  << std::endl;
        continue;
      }

      // TODO: rename to batch_id, and allow customising what batches represent
      if (metric_name == "amf_ue_id")
      {
        ue_id = metric_value;
        std::cout << "Starting to read message from UE " << ue_id << std::endl;
      }

      if (metric_name == "amf_ngap_type")
      {
        ngap_message_type = metric_value;
      }

      if (metric_name == "amf_nas_type")
      {
        nas_message_type = metric_value;
      }

      auto family = metrics->at(metric_name);

      // Get the metric family for the metric, making sure it handles all
      // possible variant types for the prometheus family
      if (auto counter_family = std::get_if<prometheus::Family<prometheus::Counter>>(family.get()))
      {
        counter_family->Add({}).Increment(std::stod(metric_value));
      }
      else if (auto gauge_family = std::get_if<prometheus::Family<prometheus::Gauge>>(family.get()))
      {
        gauge_family->Add({}).Set(std::stod(metric_value));
      }
      else if (auto histogram_family = std::get_if<prometheus::Family<prometheus::Histogram>>(family.get()))
      {
        histogram_family->Add({}).Observe(std::stod(metric_value));
      }
      else if (auto summary_family = std::get_if<prometheus::Family<prometheus::Summary>>(family.get()))
      {
        summary_family->Add({}).Observe(std::stod(metric_value));
      }
    }
  }
}

int main(int argc, char *argv[])
{
  auto exposer = std::make_shared<prometheus::Exposer>("0.0.0.0:8080");

  auto metrics = std::shared_ptr<
    std::unordered_map<std::string, std::shared_ptr<MetricFamilyVariant>>>();

  auto quantiles =
    prometheus::Summary::Quantiles{{0.5, 0.05}, {0.7, 0.03}, {0.90, 0.01}};

  // Run a TCP receiver socket to receive messages from the core
  asio::io_context io_context;
  asio::ip::tcp::acceptor acceptor(
    io_context,
    asio::ip::tcp::endpoint(asio::ip::tcp::v4(), 7799)
  );
  std::cout << "Listening for incoming metrics on port 7799" << std::endl;

  while (true)
  {
    // Use socket_ptr to pass the socket to the worker thread
    asio::ip::tcp::socket socket(io_context);
    acceptor.accept(socket);
    std::thread(threadWorker, std::move(socket), exposer, metrics).detach();
  }

  return 0;
}

/**
 * @brief Register a new metric with the Prometheus exposer
 *
 * @param exposer
 * @param metric_name
 * @param metric_description
 */
void register_metric(
  std::shared_ptr<
    std::unordered_map<std::string, std::shared_ptr<MetricFamilyVariant>>>
    &registries,
  std::shared_ptr<prometheus::Exposer> exposer,
  std::string metric_name,
  std::string metric_description
)
{
  // Get a lock on the map since this function can be called from multiple
  // threads
  std::lock_guard<std::mutex> lock(registries_mutex);

  // Check if the metric already exists
  if (registries->find(metric_name) != registries->end())
  {
    std::cerr << "Metric " << metric_name << " already exists, ignoring."
              << std::endl;
    return;
  }

  // Create a new registry
  auto registry = std::make_shared<prometheus::Registry>();

  // Register the metric
  auto metric = std::make_shared<prometheus::Family<prometheus::Counter>>(
    prometheus::BuildCounter()
      .Name(metric_name)
      .Help(metric_description)
      .Register(*registry)
  );

  // Add the registry to the map
  registries->emplace(metric_name, metric);

  std::cout << "Registered metric " << metric_name << std::endl;

  // Ask the exposer to add the contents of the registry to the HTTP server
  exposer->RegisterCollectable(registry);
}

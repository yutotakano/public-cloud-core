#include "asio.hpp"
#include "prometheus/counter.h"
#include "prometheus/exposer.h"
#include "prometheus/registry.h"
#include "prometheus/summary.h"
#include <iostream>
#include <variant>

class MultiTypeMetricRef
{
public:
  MultiTypeMetricRef() = default;
  virtual ~MultiTypeMetricRef() = default;
  enum class Type
  {
    Counter,
    Gauge,
    Summary
  };
  prometheus::Family<prometheus::Counter> *Counter() { return m_counter; }

  prometheus::Family<prometheus::Gauge> *Gauge() { return m_gauge; }

  prometheus::Family<prometheus::Summary> *Summary() { return m_summary; }

  Type type() const { return m_type; }

  void set_type(Type type) { m_type = type; }

  MultiTypeMetricRef(prometheus::Family<prometheus::Counter> *counter)
      : m_counter(counter), m_type(Type::Counter)
  {
  }

  MultiTypeMetricRef(prometheus::Family<prometheus::Gauge> *gauge)
      : m_gauge(gauge), m_type(Type::Gauge)
  {
  }

  MultiTypeMetricRef(prometheus::Family<prometheus::Summary> *summary)
      : m_summary(summary), m_type(Type::Summary)
  {
  }

private:
  prometheus::Family<prometheus::Counter> *m_counter;
  prometheus::Family<prometheus::Gauge> *m_gauge;
  prometheus::Family<prometheus::Summary> *m_summary;
  Type m_type;
};

auto quantiles =
  prometheus::Summary::Quantiles{{0.5, 0.05}, {0.7, 0.03}, {0.90, 0.01}};

// Forward-declare the function that will process incoming messages
void process_metric_definition(
  asio::ip::tcp::socket &socket,
  std::shared_ptr<prometheus::Exposer> exposer,
  std::unordered_map<std::string, MultiTypeMetricRef> &metrics,
  std::mutex &metrics_mutex,
  std::vector<std::shared_ptr<prometheus::Registry>> &captive_registries
);

void process_metric_batch_observation(
  asio::ip::tcp::socket &socket,
  std::shared_ptr<prometheus::Exposer> exposer,
  std::unordered_map<std::string, MultiTypeMetricRef> &metrics
);

void register_metric(
  std::unordered_map<std::string, MultiTypeMetricRef> &metrics,
  std::mutex &metrics_mutex,
  std::vector<std::shared_ptr<prometheus::Registry>> &captive_registries,
  std::shared_ptr<prometheus::Exposer> exposer,
  std::string metric_name,
  std::string metric_description,
  std::string metric_type
);

void threadWorker(
  asio::ip::tcp::socket socket,
  std::shared_ptr<prometheus::Exposer> exposer,
  std::unordered_map<std::string, MultiTypeMetricRef> &metrics,
  std::mutex &metrics_mutex,
  std::vector<std::shared_ptr<prometheus::Registry>> &captive_registries
)
{
  std::cout << "Accepted connection from " << socket.remote_endpoint()
            << std::endl;
  // For the header, read it into an unsigned array or otherwise overflows over
  // 127 will become negative
  std::array<uint8_t, 16> header_buffer;
  std::size_t bytes_transferred;

  try
  {
    while (true)
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
        process_metric_definition(
          socket,
          exposer,
          metrics,
          metrics_mutex,
          captive_registries
        );
      }
      else
      {
        std::cout << "Received metric batch observation message" << std::endl;
        process_metric_batch_observation(socket, exposer, metrics);
      }
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
      std::cerr << "Exception (caught): " << e.what() << std::endl;
    }
    return;
  }
}

void process_metric_definition(
  asio::ip::tcp::socket &socket,
  std::shared_ptr<prometheus::Exposer> exposer,
  std::unordered_map<std::string, MultiTypeMetricRef> &metrics,
  std::mutex &metrics_mutex,
  std::vector<std::shared_ptr<prometheus::Registry>> &captive_registries
)
{
  // Read length of remaining message
  size_t bytes_transferred = 0;

  std::array<uint8_t, 2> header_buffer;
  std::array<char, 1024> buffer;

  // Read the third and fourth byte of the message to get the length of the body
  bytes_transferred = asio::read(socket, asio::buffer(header_buffer, 2));
  if (bytes_transferred == 0)
  {
    std::cout << "0 bytes transferred" << std::endl;
    return;
  }

  // Parse the length of the body
  std::uint16_t body_length = 0;
  body_length |= static_cast<std::uint16_t>(header_buffer[0]);
  body_length |= static_cast<std::uint16_t>(header_buffer[1]) << 8;

  // Read the body
  bytes_transferred = asio::read(socket, asio::buffer(buffer, body_length));
  if (bytes_transferred == 0)
  {
    std::cout << "0 bytes transferred" << std::endl;
    return;
  }

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
    std::string metric_type; // either counter, gauge or summary

    // Loop through all parts within the line
    while (std::getline(line_stream, metric, '|'))
    {
      // Process each metric
      std::istringstream metric_stream(metric);
      std::string key_name;
      std::string key_value;
      std::getline(metric_stream, key_name, ':');
      std::getline(metric_stream, key_value);

      if (key_name == "name")
      {
        metric_name = key_value;
      }
      else if (key_name == "description")
      {
        metric_description = key_value;
      }
      else if (key_name == "type")
      {
        metric_type = key_value;
      }
    }

    // Register the metric
    register_metric(
      metrics,
      metrics_mutex,
      captive_registries,
      exposer,
      metric_name,
      metric_description,
      metric_type
    );
  }
}

void process_metric_batch_observation(
  asio::ip::tcp::socket &socket,
  std::shared_ptr<prometheus::Exposer> exposer,
  std::unordered_map<std::string, MultiTypeMetricRef> &metrics
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

  // Read the header
  bytes_transferred =
    asio::read(socket, asio::buffer(header_buffer, header_length));
  if (bytes_transferred == 0)
  {
    std::cout << "0 bytes transferred" << std::endl;
    return;
  }

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

  // Process the body
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

    // Increment the uplink packet counter for every metric batch observaftion
    // TODO: change this to a regular metric itself
    if (metrics.find("uplink_packets") != metrics.end() && metrics.at("uplink_packets").type() == MultiTypeMetricRef::Type::Counter)
    {
      metrics.at("uplink_packets").Counter()->Add({}).Increment();
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
      if (metrics.find(metric_name) == metrics.end())
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

      if (metric_name == "amf_ngap_message_type")
      {
        ngap_message_type = metric_value;
      }

      if (metric_name == "amf_nas_message_type")
      {
        nas_message_type = metric_value;
      }

      MultiTypeMetricRef family = metrics.at(metric_name);

      // Get the metric family for the metric, making sure it handles all
      // possible variant types for the prometheus family
      if (family.type() == MultiTypeMetricRef::Type::Counter)
      {
        family.Counter()
          ->Add(
            {{"ngap_type", ngap_message_type}, {"nas_type", nas_message_type}}
          )
          .Increment(std::stod(metric_value));
      }
      else if (family.type() == MultiTypeMetricRef::Type::Gauge)
      {
        family.Gauge()
          ->Add(
            {{"ngap_type", ngap_message_type}, {"nas_type", nas_message_type}}
          )
          .Set(std::stod(metric_value));
      }
      else if (family.type() == MultiTypeMetricRef::Type::Summary)
      {
        family.Summary()
          ->Add(
            {{"ngap_type", ngap_message_type}, {"nas_type", nas_message_type}},
            quantiles
          )
          .Observe(std::stod(metric_value));
      }
    }
  }
}

int main(int argc, char *argv[])
{
  auto exposer = std::make_shared<prometheus::Exposer>("0.0.0.0:8080");

  auto metrics = std::unordered_map<std::string, MultiTypeMetricRef>();
  std::mutex metrics_mutex;

  // Create a list of registries to keep them alive while we refer to their
  // metrics by raw pointer through MultiTypeMetricRef
  std::vector<std::shared_ptr<prometheus::Registry>> captive_registries;

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
    std::thread(
      threadWorker,
      std::move(socket),
      exposer,
      std::ref(metrics),
      std::ref(metrics_mutex),
      std::ref(captive_registries)
    )
      .detach();
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
  std::unordered_map<std::string, MultiTypeMetricRef> &metrics,
  std::mutex &metrics_mutex,
  std::vector<std::shared_ptr<prometheus::Registry>> &captive_registries,
  std::shared_ptr<prometheus::Exposer> exposer,
  std::string metric_name,
  std::string metric_description,
  std::string metric_type
)
{
  // Lock the mutex to prevent adding two of the same metric at the same time
  // concurrently
  std::lock_guard<std::mutex> lock(metrics_mutex);

  // Check if the metric already exists
  // Loop through all registries to check if the metric already exists
  if (metrics.find(metric_name) != metrics.end())
  {
    std::cerr << "Metric " << metric_name << " already exists, ignoring."
              << std::endl;
    return;
  }

  // Create a new registry
  auto registry = std::make_shared<prometheus::Registry>();

  // Create a metric. The registry owns the reference to the metric, and
  // will live as long as the registry does.
  // Then, add a reference of the registry to the global map. The metric is
  // owned by the registry, so we can safely store a reference to the metric in
  // the map as long as the registry is alive.
  if (metric_type == "summary")
  {
    auto &metric = prometheus::BuildSummary()
                     .Name(metric_name)
                     .Help(metric_description)
                     .Register(*registry);
    metrics[metric_name] = MultiTypeMetricRef{&metric};
  }
  else if (metric_type == "gauge")
  {
    auto &metric = prometheus::BuildGauge()
                     .Name(metric_name)
                     .Help(metric_description)
                     .Register(*registry);
    metrics[metric_name] = MultiTypeMetricRef{&metric};
  }
  else
  {
    auto &metric = prometheus::BuildCounter()
                     .Name(metric_name)
                     .Help(metric_description)
                     .Register(*registry);
    metrics[metric_name] = MultiTypeMetricRef{&metric};
  }

  std::cout << "Registered metric " << metric_name << std::endl;

  // Ask the exposer to add the contents of the registry to the HTTP server
  exposer->RegisterCollectable(registry);

  // Keep the registry alive by storing it in a global list
  captive_registries.push_back(registry);
}

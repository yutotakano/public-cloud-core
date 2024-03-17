#include "asio.hpp"
#include "prometheus/counter.h"
#include "prometheus/exposer.h"
#include "prometheus/registry.h"
#include "prometheus/summary.h"
#include <iostream>

void threadWorker(
  asio::ip::tcp::socket socket,
  prometheus::Summary::Quantiles quantiles,
  prometheus::Family<prometheus::Counter> &packet_counter,
  prometheus::Family<prometheus::Summary> &invalid_messages_summary,
  prometheus::Family<prometheus::Summary> &latency_summary,
  prometheus::Family<prometheus::Summary> &decode_latency_summary,
  prometheus::Family<prometheus::Summary> &handle_latency_summary,
  prometheus::Family<prometheus::Summary> &encode_latency_summary,
  prometheus::Family<prometheus::Summary> &send_latency_summary,
  prometheus::Family<prometheus::Summary> &num_responses_summary
)
{
  std::cout << "Accepted connection from " << socket.remote_endpoint()
            << std::endl;
  std::array<char, 1024> buffer;
  std::size_t bytes_transferred;
  try
  {
    bytes_transferred = socket.read_some(asio::buffer(buffer));
    if (bytes_transferred == 0)
    {
      return;
    }
  }
  catch (std::system_error &e)
  {
    std::error_code error = e.code();
    if (error == asio::error::eof)
    {
      std::cout << "Connection closed by peer" << std::endl;
    }
    else
    {
      std::cerr << "Exception: " << e.what() << std::endl;
    }
    return;
  }

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

    while (std::getline(line_stream, metric, '|'))
    {
      // Process each metric
      std::istringstream metric_stream(metric);
      std::string metric_name;
      std::string metric_value;
      std::getline(metric_stream, metric_name, ':');
      std::getline(metric_stream, metric_value);

      if (metric_name == "amf_message_ue_id")
      {
        ue_id = metric_value;
        std::cout << "Starting to read message from UE " << ue_id << std::endl;
      }
      else if (metric_name == "amf_message_ngap_type")
      {
        ngap_message_type = metric_value;
      }
      else if (metric_name == "amf_message_nas_type")
      {
        nas_message_type = metric_value;
        packet_counter
          .Add(
            {{"nas_type", nas_message_type}, {"ngap_type", ngap_message_type}}
          )
          .Increment();
      }
      else if (metric_name == "amf_message_invalid")
      {
        invalid_messages_summary
          .Add(
            {{"nas_type", nas_message_type}, {"ngap_type", ngap_message_type}},
            quantiles
          )
          .Observe(std::stod(metric_value));
      }
      else if (metric_name == "amf_message_latency")
      {
        latency_summary
          .Add(
            {{"nas_type", nas_message_type}, {"ngap_type", ngap_message_type}},
            quantiles
          )
          .Observe(std::stod(metric_value));
      }
      else if (metric_name == "amf_message_decode_latency")
      {
        decode_latency_summary
          .Add(
            {{"nas_type", nas_message_type}, {"ngap_type", ngap_message_type}},
            quantiles
          )
          .Observe(std::stod(metric_value));
      }
      else if (metric_name == "amf_message_handle_latency")
      {
        handle_latency_summary
          .Add(
            {{"nas_type", nas_message_type}, {"ngap_type", ngap_message_type}},
            quantiles
          )
          .Observe(std::stod(metric_value));
      }
      else if (metric_name == "amf_message_encode_latency")
      {
        encode_latency_summary
          .Add(
            {{"nas_type", nas_message_type}, {"ngap_type", ngap_message_type}},
            quantiles
          )
          .Observe(std::stod(metric_value));
      }
      else if (metric_name == "amf_message_send_latency")
      {
        send_latency_summary
          .Add(
            {{"nas_type", nas_message_type}, {"ngap_type", ngap_message_type}},
            quantiles
          )
          .Observe(std::stod(metric_value));
      }
      else if (metric_name == "amf_message_responses")
      {
        num_responses_summary
          .Add(
            {{"nas_type", nas_message_type}, {"ngap_type", ngap_message_type}},
            quantiles
          )
          .Observe(std::stod(metric_value));
      }
    }
  }
}

int main(int argc, char *argv[])
{
  prometheus::Exposer exposer{"0.0.0.0:8080"};

  auto registry = std::make_shared<prometheus::Registry>();

  // Metric for the number of handled packets (i.e. number of messages that the
  // aggregator received from the core)
  auto &packet_counter =
    prometheus::BuildCounter()
      .Name("amf_handled_packets_total")
      .Help("Number of handled packets")
      .Register(*registry);

  // Metric for whether the received message was invalid (e.g. decode error)
  auto &invalid_messages_summary =
    prometheus::BuildSummary()
      .Name("amf_message_invalid")
      .Help("Whether the message was invalid")
      .Register(*registry);

  // Metric for the latency between AMF receiving messages and finishing to send
  // them
  auto &latency_summary =
    prometheus::BuildSummary()
      .Name("amf_message_latency")
      .Help("Latency between AMF receiving messages and finishing to send them")
      .Register(*registry);

  // Metric for the latency between AMF receiving messages and decoding them
  auto &decode_latency_summary =
    prometheus::BuildSummary()
      .Name("amf_message_decode_latency")
      .Help("Latency between AMF receiving messages and decoding them")
      .Register(*registry);

  // Metric for the latency between AMF decoding messages and finishing building
  // responses
  auto &handle_latency_summary =
    prometheus::BuildSummary()
      .Name("amf_message_handle_latency")
      .Help(
        "Latency between AMF decoding messages and finishing building responses"
      )
      .Register(*registry);

  // Metric for the latency between AMF finishing building responses and
  // encoding them
  auto &encode_latency_summary =
    prometheus::BuildSummary()
      .Name("amf_message_encode_latency")
      .Help("Latency between AMF finishing building responses and encoding them"
      )
      .Register(*registry);

  // Metric for the latency between AMF encoding messages and sending them
  auto &send_latency_summary =
    prometheus::BuildSummary()
      .Name("amf_message_send_latency")
      .Help("Latency between AMF encoding messages and sending them")
      .Register(*registry);

  // Metric for the number of responses sent by AMF
  auto &num_responses_summary =
    prometheus::BuildSummary()
      .Name("amf_message_responses")
      .Help("Number of responses sent by AMF")
      .Register(*registry);

  auto quantiles =
    prometheus::Summary::Quantiles{{0.5, 0.05}, {0.7, 0.03}, {0.90, 0.01}};

  // Ask the exposer to scrape the registry on incoming HTTP requests
  exposer.RegisterCollectable(registry);

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
      quantiles,
      std::ref(packet_counter),
      std::ref(invalid_messages_summary),
      std::ref(latency_summary),
      std::ref(decode_latency_summary),
      std::ref(handle_latency_summary),
      std::ref(encode_latency_summary),
      std::ref(send_latency_summary),
      std::ref(num_responses_summary)
    )
      .detach();
  }

  return 0;
}

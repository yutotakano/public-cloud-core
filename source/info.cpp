#include "info.h"
#include "utils.h"

InfoApp::InfoApp()
{
  logger = quill::get_logger();
  executor = Executor();
}

std::unique_ptr<argparse::ArgumentParser> InfoApp::info_arg_parser()
{
  auto parser = std::make_unique<argparse::ArgumentParser>("info");
  parser->add_description("Get information about the current deployment.");
  return parser;
}

void InfoApp::info_command_handler(argparse::ArgumentParser &parser)
{
  LOG_DEBUG(logger, "Looking for active deployment.");

  // Check if there is an active deployment
  auto contexts_str =
    executor.run({"kubectl", "config", "get-contexts", "--output=name"}, false)
      .future.get();
  auto contexts = Utils::split(contexts_str, '\n');
  if (contexts.size() == 1 && contexts[0].empty())
  {
    LOG_ERROR(logger, "No active deployment found.");
    return;
  }

  // Double-check that there is a CoreKube and Nervion deployment
  std::string corekube_name;
  std::string nervion_name;
  for (auto &context : contexts)
  {
    if (context.find("corekube") != std::string::npos)
    {
      corekube_name = context;
    }
    if (context.find("nervion") != std::string::npos)
    {
      nervion_name = context;
    }
  }
  if (corekube_name.empty() || nervion_name.empty())
  {
    LOG_ERROR(logger, "CoreKube or Nervion deployment not found.");
    return;
  }

  // Switch to the CoreKube context to get information about frontend
  executor.run({"kubectl", "config", "use-context", corekube_name}, false)
    .future.get();

  // Get the VPC-internal IP of the frontend node
  auto frontend_ip =
    executor
      .run(
        {"kubectl",
         "get",
         "nodes",
         "-o",
         "jsonpath={.items[0].status.addresses[?(@.type==\"InternalIP\")]."
         "address}",
         "-l",
         "alpha.eksctl.io/nodegroup-name=ng-corekube"},
        false
      )
      .future.get();

  // Get the public DNS of the Grafana service.
  auto ck_grafana_public_dns =
    executor
      .run(
        {"kubectl",
         "get",
         "services/corekube-grafana",
         "-n=grafana",
         "-o=jsonpath={.status.loadBalancer.ingress[0].hostname}"},
        false
      )
      .future.get();

  // Switch to the Nervion context to get information about Nervion controller
  executor.run({"kubectl", "config", "use-context", nervion_name}, false)
    .future.get();

  auto nervion_controller_public_dns =
    executor
      .run(
        {
          "kubectl",
          "get",
          "services/ran-emulator",
          "-o=jsonpath={.status.loadBalancer.ingress[0].hostname}",
        },
        false
      )
      .future.get();

  LOG_INFO(logger, "CoreKube deployment found.");
  LOG_INFO(logger, "IP (within VPC) of CK frontend node: {}", frontend_ip);
  LOG_INFO(logger, "Grafana: http://{}:3000", ck_grafana_public_dns);
  LOG_INFO(logger, "Nervion: http://{}:8080", nervion_controller_public_dns);
  LOG_INFO(logger, "Current cluster: nervion-aws-cluster");
}

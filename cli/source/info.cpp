#include "info.h"
#include "utils.h"

InfoApp::InfoApp()
{
  logger = quill::get_logger();
  executor = Executor();
  ck_app = CKApp();
  nv_app = NVApp();
}

std::unique_ptr<argparse::ArgumentParser> InfoApp::info_arg_parser()
{
  auto parser = std::make_unique<argparse::ArgumentParser>("info");
  parser->add_description("Get information about the current deployment.");
  return parser;
}

void InfoApp::info_command_handler(argparse::ArgumentParser &parser)
{
  LOG_TRACE_L3(logger, "Getting information about the current deployment.");

  auto info = get_info();
  if (!info.has_value())
  {
    LOG_ERROR(logger, "Could not get info: no active deployment found.");
    return;
  }

  LOG_INFO(logger, "CoreKube deployment found.");
  LOG_INFO(logger, "IP (within VPC) of CK frontend: {}", info->ck_frontend_ip);
  LOG_INFO(logger, "Grafana: http://{}:3000", info->ck_grafana_elb_url);
  LOG_INFO(logger, "Nervion: http://{}:8080", info->nv_controller_elb_url);
  LOG_INFO(
    logger,
    "KubeCost Prometheus: http://{}:80",
    info->ck_kubecost_prometheus_elb_url
  );
}

bool InfoApp::check_contexts_exist()
{
  LOG_TRACE_L3(logger, "Checking existing contexts");

  auto contexts_str =
    executor
      .run(
        {"kubectl",
         "config",
         "get-contexts",
         "--output=name",
         "--kubeconfig",
         "config"},
        false,
        false,
        false
      )
      .future.get();
  auto contexts = Utils::split(contexts_str, '\n');
  if (contexts.size() == 1 && contexts[0].empty())
  {
    LOG_TRACE_L3(logger, "No contexts found.");
    return false;
  }

  std::string corekube_context;
  std::string nervion_context;
  for (auto &context : contexts)
  {
    if (context.find("corekube") != std::string::npos)
    {
      corekube_context = context;
      LOG_TRACE_L3(logger, "CoreKube context found: {}", context);
    }
    if (context.find("nervion") != std::string::npos)
    {
      nervion_context = context;
      LOG_TRACE_L3(logger, "Nervion context found: {}", context);
    }
  }
  if (corekube_context.empty() || nervion_context.empty())
  {
    LOG_TRACE_L3(logger, "Either CoreKube or Nervion deployment not found.");
    return false;
  }

  return true;
}

std::optional<deployment_info_s> InfoApp::get_info()
{
  LOG_DEBUG(logger, "Looking for active deployment.");

  // Check if there is an active deployment
  if (!check_contexts_exist())
  {
    return std::nullopt;
  }

  try
  {
    // Get the VPC-internal IP of the frontend node
    LOG_TRACE_L3(logger, "Getting frontend IP");
    auto frontend_ip =
      ck_app
        .run_kubectl(
          {"get",
           "pods",
           "-n",
           "corekube-frontend",
           "-o",
           "jsonpath={.items[0].status.hostIP}"},
          false
        )
        .future.get();

    // Get the public DNS of various services
    auto ck_service_hostnames_str =
      ck_app
        .run_kubectl(
          {"get",
           "services",
           "-A",
           "-o",
           "jsonpath={.items[?(@.metadata.name==\"corekube-grafana\")].status."
           "loadBalancer.ingress[0].hostname} "
           "{.items[?(@.metadata.name==\"corekube-prometheus-loadbalancer\")]."
           "status.loadBalancer.ingress[0].hostname} "
           "{.items[?(@.metadata.name==\"kubecost-prometheus-loadbalancer\")]."
           "status.loadBalancer.ingress[0].hostname}"},
          false
        )
        .future.get();

    // Split by space
    auto ck_service_hostnames = Utils::split(ck_service_hostnames_str, ' ');

    // Get the public DNS of the CK Grafana service.
    auto ck_grafana_public_dns = ck_service_hostnames[0];
    // Get the public DNS of the CK Prometheus service.
    auto ck_prometheus_public_dns = ck_service_hostnames[1];
    auto ck_kubecost_prometheus_public_dns = ck_service_hostnames[2];

    // Get the public DNS of the Nervion controller service.
    auto nervion_controller_public_dns =
      nv_app
        .run_kubectl(
          {
            "get",
            "services/ran-emulator",
            "-o=jsonpath={.status.loadBalancer.ingress[0].hostname}",
          },
          false
        )
        .future.get();

    return deployment_info_s{
      .ck_grafana_elb_url = ck_grafana_public_dns,
      .ck_prometheus_elb_url = ck_prometheus_public_dns,
      .nv_controller_elb_url = nervion_controller_public_dns,
      .ck_kubecost_prometheus_elb_url = ck_kubecost_prometheus_public_dns,
      .ck_frontend_ip = frontend_ip,
    };
  }
  catch (const std::exception &e)
  {
    LOG_ERROR(logger, "Error getting info: {}", e.what());
    return std::nullopt;
  }
}

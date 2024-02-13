#include "env.h"

EnvApp::EnvApp()
{
  logger = quill::get_logger();
  info_app = InfoApp();
}

std::unique_ptr<argparse::ArgumentParser> EnvApp::env_arg_parser()
{
  auto parser = std::make_unique<argparse::ArgumentParser>("env");
  parser->add_description("Manage the current environment.");
  parser->add_argument("--context").help("Switch to the given context");
  return parser;
}

void EnvApp::env_command_handler(argparse::ArgumentParser &parser)
{
  if (std::optional<std::string> context =
        parser.present<std::string>("--context");
      context.has_value())
  {
    switch_context(context.value());
  }
  else
  {
    get_environment();
  }
}

void EnvApp::switch_context(std::string context)
{
  LOG_TRACE_L3(logger, "Getting current environment.");

  auto contexts = info_app.get_contexts();
  if (!contexts.has_value())
  {
    LOG_ERROR(logger, "Could not get environment: no active deployment found.");
    return;
  }

  LOG_DEBUG(logger, "Current context: {}", contexts->current_context);
  LOG_DEBUG(logger, "CoreKube context: {}", contexts->corekube_context);
  LOG_DEBUG(logger, "Nervion context: {}", contexts->nervion_context);

  if (context == "corekube")
  {
    LOG_INFO(logger, "Switching to CoreKube context.");
    info_app.switch_context(contexts->corekube_context);
  }
  else if (context == "nervion")
  {
    LOG_INFO(logger, "Switching to Nervion context.");
    info_app.switch_context(contexts->nervion_context);
  }
  else
  {
    LOG_ERROR(logger, "Invalid context: {}", context);
  }
}

void EnvApp::get_environment()
{
  LOG_TRACE_L3(logger, "Getting current environment.");

  auto contexts = info_app.get_contexts();
  if (!contexts.has_value())
  {
    LOG_ERROR(logger, "Could not get environment: no active deployment found.");
    return;
  }

  LOG_INFO(logger, "Current context: {}", contexts->current_context);
  LOG_INFO(logger, "CoreKube context: {}", contexts->corekube_context);
  LOG_INFO(logger, "Nervion context: {}", contexts->nervion_context);
}

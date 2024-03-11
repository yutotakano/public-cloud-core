#include "argparse/argparse.hpp"
#include "curl/curl.h"
#include "deploy.h"
#include "env.h"
#include "exithandler.h"
#include "info.h"
#include "loadtest.h"
#include "quill/Quill.h"
#include <iostream>

int main(int argc, char **argv)
{
  // Any global setups
  // Initialize libcurl
  curl_global_init(CURL_GLOBAL_ALL);
  // Register Ctrl-C handler
  ExitHandler::create_handlers();

  // Get the stdout file handler that the default root logger uses.
  // We'll use this to configure the format pattern based on verbosity.
  std::shared_ptr<quill::Handler> file_handler = quill::stdout_handler();

  // Set a custom formatter for this handler for use when verbosity is disabled.
  file_handler->set_pattern(
    "%(ascii_time) [%(thread)] %(level_name) - %(message)",
    "%H:%M:%S.%Qns", // timestamp format (nanoseconds precision)
    quill::Timezone::GmtTime // timestamp's timezone
  );

  quill::Config cfg;
  cfg.enable_console_colours = true;
  quill::configure(cfg);

  // Initialize the logger
  quill::start();
  quill::Logger *logger = quill::get_logger();

  // Setup the command line parser
  argparse::ArgumentParser program("publicore", "0.0.1");

  program.add_description(
    "Command-line software to setup CoreKube on the public cloud."
  );

  program.add_argument("-v", "--verbose")
    .help("Enable verbose mode")
    .default_value(false)
    .implicit_value(true);

  // Initialize all the subcommand apps
  DeployApp deploy_app;
  auto deploy_parser = deploy_app.deploy_arg_parser();
  program.add_subparser(*deploy_parser);

  InfoApp info_app;
  auto info_parser = info_app.info_arg_parser();
  program.add_subparser(*info_parser);

  LoadTestApp loadtest_app;
  auto loadtest_parser = loadtest_app.loadtest_arg_parser();
  program.add_subparser(*loadtest_parser);

  EnvApp env_app;
  auto env_parser = env_app.env_arg_parser();
  program.add_subparser(*env_parser);

  // Parse the command line arguments
  try
  {
    program.parse_args(argc, argv);
  }
  catch (const std::exception &err)
  {
    std::cerr << err.what() << std::endl;
    return 1;
  }

  // Set the log level and formatter based on verbosity
  if (program.get<bool>("--verbose"))
  {
    logger->set_log_level(quill::LogLevel::TraceL3);

    // Set a modified formatter that includes the filename and line number
    file_handler->set_pattern(
      "%(ascii_time) [%(thread)] %(level_name) %(filename):%(lineno) - "
      "%(message)",
      "%H:%M:%S.%Qns", // timestamp format (nanoseconds precision)
      quill::Timezone::GmtTime // timestamp's timezone
    );

    LOG_INFO(logger, "Verbose mode enabled");
  }

  // Execute depending on the subparser
  if (program.is_subcommand_used(*deploy_parser))
  {
    deploy_app.deploy_command_handler(*deploy_parser);
  }
  else if (program.is_subcommand_used(*info_parser))
  {
    info_app.info_command_handler(*info_parser);
  }
  else if (program.is_subcommand_used(*loadtest_parser))
  {
    loadtest_app.loadtest_command_handler(*loadtest_parser);
  }
  else if (program.is_subcommand_used(*env_parser))
  {
    env_app.env_command_handler(*env_parser);
  }
  else
  {
    std::cerr << "No subcommand used" << std::endl;
    return 1;
  }

  // Cleanup
  curl_global_cleanup();

  return 0;
}

#include <iostream>
#include "argparse/argparse.hpp"
#include "deploy.h"
#include "quill/Quill.h"

int main(int argc, char** argv)
{
  quill::start();

  quill::Logger* logger = quill::get_logger();
  logger->set_log_level(quill::LogLevel::TraceL3);
  LOG_INFO(logger, "This is a log info example {}", 5);

  // Setup the command line parser
  argparse::ArgumentParser program("publicore", "0.0.1");

  program.add_description(
    "Command-line software to setup CoreKube on the public cloud."
  );

  // Add all the subparsers
  auto deploy_parser = deploy_arg_parser();
  program.add_subparser(*deploy_parser);

  // Parse the command line arguments
  try
  {
    program.parse_args(argc, argv);
  }
  catch (const std::exception& err)
  {
    std::cerr << err.what() << std::endl;
    return 1;
  }

  // Execute depending on the subparser
  if (program.is_subcommand_used(*deploy_parser))
  {
    std::cout << "Deploying CoreKube on the public cloud" << std::endl;
  }
  return 0;
}

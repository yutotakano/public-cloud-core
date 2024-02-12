#include "deploy.h"
#include "argparse/argparse.hpp"
#include "executor.h"
#include "quill/Quill.h"
#include "subprocess_error.h"
#include "utils.h"

DeployApp::DeployApp()
{
  logger = quill::get_logger();
  executor = Executor();
}

std::unique_ptr<argparse::ArgumentParser> DeployApp::deploy_arg_parser()
{
  auto deploy_command = std::make_unique<argparse::ArgumentParser>("deploy");
  deploy_command->add_description("CLI to deploy CoreKube on the public cloud");

  deploy_command->add_argument("--type")
    .default_value(std::string{"aws-eks-fargate"})
    .help("Type of deployment")
    .choices("aws-eks-fargate", "aws-eks-ec2", "aws-eks-ec2-spot", "aws-ec2");

  deploy_command->add_argument("--ssh-key")
    .default_value(std::string{""})
    .help("SSH key to use for the deployment");

  deploy_command->add_argument("--versions")
    .default_value(false)
    .implicit_value(true)
    .help("Print the versions of the tools used by the executor");

  deploy_command->add_argument("--teardown")
    .default_value(false)
    .implicit_value(true)
    .help("Teardown the previous deployment before deploying");

  deploy_command->add_argument("--no-deploy")
    .default_value(false)
    .implicit_value(true)
    .help("Don't deploy anything, just teardown the previous deployment");

  return deploy_command;
}

void DeployApp::deploy_command_handler(argparse::ArgumentParser &parser)
{
  if (parser.get<bool>("--versions"))
  {
    LOG_TRACE_L3(logger, "Printing tool versions");
    executor.print_versions();
    return;
  }

  bool teardown = parser.get<bool>("--teardown");
  bool no_setup = parser.get<bool>("--no-deploy");

  LOG_TRACE_L3(logger, "Teardown: {}", teardown);
  LOG_TRACE_L3(logger, "No setup: {}", no_setup);
  LOG_TRACE_L3(logger, "Type: {}", parser.get<std::string>("--type"));

  if (parser.get<std::string>("--type") == "aws-eks-fargate")
  {
    if (teardown)
      teardown_aws_eks_fargate();
    if (no_setup)
      return;

    std::string public_key_path = parser.get<std::string>("--ssh-key");
    if (public_key_path.empty())
      public_key_path = get_public_key_path();

    deploy_aws_eks_fargate(public_key_path);
  }
  else if (parser.get<std::string>("--type") == "aws-eks-ec2")
  {
    if (teardown)
      teardown_aws_eks_ec2();
    if (no_setup)
      return;
    deploy_aws_eks_ec2();
  }
  else if (parser.get<std::string>("--type") == "aws-eks-ec2-spot")
  {
    if (teardown)
      teardown_aws_eks_ec2_spot();
    if (no_setup)
      return;
    deploy_aws_eks_ec2_spot();
  }
  else if (parser.get<std::string>("--type") == "aws-ec2")
  {
    if (teardown)
      teardown_aws_ec2();
    if (no_setup)
      return;
    deploy_aws_ec2();
  }
  else
  {
    LOG_ERROR(logger, "Invalid deployment type specified");
  }
}

std::string DeployApp::get_public_key_path()
{
  char *home = std::getenv("HOME");
#ifdef _WIN32
  home = std::getenv("USERPROFILE");
#endif

  // Make sure getenv didn't return a null pointer
  std::string homedir = home ? std::string(home) : ".";

  LOG_TRACE_L3(logger, "User home directory: {}", homedir);

  std::vector<std::string> public_key_paths = {
    homedir + "/.ssh/id_rsa.pub",
    homedir + "/.ssh/id_dsa.pub",
    homedir + "/.ssh/id_ecdsa.pub",
    homedir + "/.ssh/id_ed25519.pub",
  };

  for (auto &path : public_key_paths)
  {
    LOG_TRACE_L3(logger, "Checking for public key at {}", path);
    if (std::filesystem::exists(path))
    {
      return path;
    }
  }

  LOG_ERROR(logger, "No public key found in $HOME/.ssh");
  return "";
}

void DeployApp::deploy_aws_eks_fargate(std::string public_key_path)
{
  LOG_INFO(logger, "Deploying CoreKube on AWS EKS with Fargate...");

  // Create the EKS cluster (bootstrapped with private+public VPC subnets and
  // no nodegroup)
  executor
    .run(
      {"eksctl",
       "create",
       "cluster",
       "--name=corekube-aws-cluster",
       "--region=eu-north-1",
       "--version=1.28",
       "--without-nodegroup",
       "--node-private-networking",
       "--ssh-access",
       "--ssh-public-key=" + public_key_path}
    )
    .future.get();

  // Create a nodegroup to attach to the cluster - this is for the frontend
  // and database.
  auto ng_future =
    executor
      .run(
        {"eksctl",
         "create",
         "nodegroup",
         "--cluster=corekube-aws-cluster",
         "--name=ng-corekube",
         "--node-ami-family=AmazonLinux2",
         "--node-type=t3.small",
         "--nodes=1",
         "--nodes-min=1",
         "--nodes-max=1",
         "--node-volume-size=20",
         "--ssh-access",
         "--ssh-public-key=" + public_key_path,
         "--managed",
         "--asg-access"}
      )
      .future;

  // Create a Fargate profile to attach to the cluster, for the backend
  auto fp_future =
    executor
      .run(
        {"eksctl",
         "create",
         "fargateprofile",
         "--namespace=default",
         "--namespace=kube-system",
         "--namespace=grafana",
         "--namespace=prometheus",
         "--cluster=corekube-aws-cluster",
         "--name=fp-corekube"}
      )
      .future;

  // Wait for the nodegroup and Fargate profile to be created
  ng_future.get();
  fp_future.get();

  // Collect the public subnets for the Fargate profile, which we will use later
  // to attach Nervion
  auto public_subnets_tab =
    executor
      .run(
        {"aws",
         "ec2",
         "describe-subnets",
         "--filter",
         "Name=tag:alpha.eksctl.io/cluster-name,Values=corekube-aws-cluster",
         "Name=tag:aws:cloudformation:logical-id,Values=SubnetPublic*",
         "--query",
         "Subnets[*].SubnetId",
         "--output",
         "text"},
        false
      )
      .future.get();

  // The output from above is tab-separated so we replace tabs with commas
  std::string public_subnets_comma = public_subnets_tab;
  std::replace(
    public_subnets_comma.begin(),
    public_subnets_comma.end(),
    '\t',
    ','
  );

  LOG_INFO(logger, "Public subnets: {}", public_subnets_comma);

  // Collect pivate subnets for the Fargate profile
  auto private_subnets_tab =
    executor
      .run(
        {"aws",
         "ec2",
         "describe-subnets",
         "--filter",
         "Name=tag:alpha.eksctl.io/cluster-name,Values=corekube-aws-cluster",
         "Name=tag:aws:cloudformation:logical-id,Values=SubnetPrivate*",
         "--query",
         "Subnets[*].SubnetId",
         "--output",
         "text"},
        false
      )
      .future.get();

  // The output from above is tab-separated so we replace tabs with commas
  std::string private_subnets_comma = private_subnets_tab;
  std::replace(
    private_subnets_comma.begin(),
    private_subnets_comma.end(),
    '\t',
    ','
  );

  LOG_INFO(logger, "Private subnets: {}", private_subnets_comma);

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
         "alpha.eksctl.io/nodegroup-name=ng-corekube"}
      )
      .future.get();

  // Apply metrics server deployment
  executor
    .run({"kubectl", "apply", "-f", "scripts/configs/metrics-server.yaml"})
    .future.get();

  // Apply the CoreKube workers and frontend deployments
  executor
    .run(
      {"kubectl",
       "apply",
       "-f",
       "scripts/configs/5G/corekube-worker-and-db.yaml"}
    )
    .future.get();

  executor
    .run({"kubectl", "apply", "-f", "scripts/configs/5G/corekube-frontend.yaml"}
    )
    .future.get();

  // Apply prometheus
  executor.run({"kubectl", "apply", "-f", "scripts/configs/prometheus.yaml"})
    .future.get();

  // Apply opencost
  executor.run({"kubectl", "apply", "-f", "scripts/configs/opencost.yaml"})
    .future.get();

  // Apply grafana
  executor.run({"kubectl", "apply", "-f", "scripts/configs/grafana.yaml"})
    .future.get();
  executor
    .run(
      {"kubectl",
       "create",
       "configmap",
       "corekube-grafana-dashboards",
       "--namespace=grafana",
       "--from-file=scripts/configs/dashboards/"}
    )
    .future.get();
  executor
    .run(
      {"kubectl",
       "wait",
       "-n",
       "grafana",
       "--for=condition=ready",
       "pod",
       "--all",
       "--timeout=10m"}
    )
    .future.get();
  // executor
  //   .run(
  //     {"kubectl",
  //      "port-forward",
  //      "services/corekube-grafana",
  //      "-n",
  //      "grafana",
  //      "12345:3000"}
  //   )
  //   .future.get();

  // Get the CoreKube cluster security group id
  auto ck_security_group_id =
    executor
      .run(
        {"aws",
         "ec2",
         "describe-instances",
         "--filters",
         "Name=tag:eks:cluster-name,Values=corekube-aws-cluster",
         "--query",
         "Reservations[*].Instances[*].SecurityGroups[?starts_with(GroupName, "
         "`eks-`)][][].GroupId | [0]",
         "--output=text"}
      )
      .future.get();

  // # Add a new rule to the security group which allows all incoming traffic
  executor
    .run(
      {"aws",
       "ec2",
       "authorize-security-group-ingress",
       "--group-id",
       ck_security_group_id,
       "--protocol",
       "all",
       "--cidr",
       "0.0.0.0/0"}
    )
    .future.get();

  // don't create nodegroup automatically, we'll set a specific size and use
  // that also use the same subnet as the corekube because otherwise I had
  // massive problems with trying to reach the other cluster over public
  // internet
  executor
    .run(
      {"eksctl",
       "create",
       "cluster",
       "--name=nervion-aws-cluster",
       "--region=eu-north-1",
       "--version=1.28",
       "--without-nodegroup",
       "--node-private-networking",
       "--vpc-public-subnets",
       public_subnets_comma,
       "--vpc-private-subnets",
       private_subnets_comma}
    )
    .future.get();

  // eksctl create nodegroup --cluster nervion-aws-cluster --name ng-nervion
  executor
    .run(
      {"eksctl",
       "create",
       "nodegroup",
       "--cluster=nervion-aws-cluster",
       "--name=ng-nervion",
       "--node-ami-family=AmazonLinux2",
       "--node-type=t3.small",
       "--nodes=2",
       "--nodes-min=2",
       "--nodes-max=2",
       "--node-volume-size=20",
       "--ssh-access",
       "--ssh-public-key=" + public_key_path,
       "--managed",
       "--asg-access"}
    )
    .future.get();

  // Verify that we can ping the frontend node from the Nervion cluster
  executor
    .run(
      {"kubectl",
       "run",
       "-i",
       "--tty",
       "--rm",
       "debug",
       "--image=alpine",
       "--restart=Never",
       "--",
       "sh",
       "-c",
       "apk add --no-cache iputils && ping -c 1 " + frontend_ip}
    )
    .future.get();

  // Get Nervion cluster security group id
  auto nervion_security_group_id =
    executor
      .run(
        {"aws",
         "ec2",
         "describe-instances",
         "--filters",
         "Name=tag:eks:cluster-name,Values=nervion-aws-cluster",
         "--query",
         "Reservations[*].Instances[*].SecurityGroups[?starts_with(GroupName, "
         "`eks-`)][][].GroupId | [0]",
         "--output",
         "text"}
      )
      .future.get();
  nervion_security_group_id.pop_back(); // remove trailing newline

  // Add a new rule to the security group which allows all incoming traffic
  executor
    .run(
      {"aws",
       "ec2",
       "authorize-security-group-ingress",
       "--group-id",
       nervion_security_group_id,
       "--protocol",
       "all",
       "--cidr",
       "0.0.0.0/0"}
    )
    .future.get();

  // Apply Nervion deployment
  executor.run({"kubectl", "apply", "-f", "scripts/configs/nervion.yaml"})
    .future.get();
  executor
    .run(
      {"kubectl",
       "wait",
       "--for=condition=ready",
       "pod",
       "--all",
       "--timeout=10m"}
    )
    .future.get();

  LOG_INFO(logger, "CoreKube deployed successfully!");
  LOG_INFO(logger, "IP (within VPC) of CK frontend node: {}", frontend_ip);
  LOG_INFO(
    logger,
    "Port-forward not yet implemented, use kubectl port-forward"
  );
  LOG_INFO(logger, "Current cluster: nervion-aws-cluster");
}

std::string
DeployApp::get_most_recent_cloudformation_event(std::string stack_name)
{
  return executor
    .run(
      {"aws",
       "cloudformation",
       "describe-stack-events",
       "--stack-name=" + stack_name,
       "--query",
       "StackEvents[0].[ResourceType, ResourceStatus, LogicalResourceId]",
       "--output=text"},
      false
    )
    .future.get();
}

std::future<std::string> DeployApp::eksctl_delete_resource(
  std::string resource_type,
  std::vector<std::string> args
)
{
  std::vector<std::string> full_args = {"eksctl", "get", resource_type};
  full_args.insert(full_args.end(), args.begin(), args.end());

  // Check that the resource exists
  try
  {
    executor.run(full_args, false).future.get();
  }
  catch (const SubprocessError &e)
  {
    LOG_INFO(logger, "Resource does not exist - skipping");
    return std::async([]() { return std::string(); });
  }

  // Delete - eksctl has same syntax for get and delete for the most part
  full_args.at(1) = "delete";

  // If resource is nodegroup, we have to disable eviction:
  // https://github.com/eksctl-io/eksctl/issues/6287
  if (resource_type == "nodegroup")
  {
    full_args.emplace_back("--disable-eviction");
  }
  return executor.run(full_args, true).future;
}

std::future<void> DeployApp::eksctl_deletion_details(std::string stack_name)
{
  return std::async(
    [this, &stack_name]()
    {
      bool deleted = false;
      while (!deleted)
      {
        try
        {
          auto log_entry = get_most_recent_cloudformation_event(stack_name);
          LOG_INFO(logger, "Deleting Fargate Profile: {}", log_entry);
        }
        catch (const SubprocessError &e)
        {
          deleted = true;
        }
      }
    }
  );
}

void DeployApp::teardown_aws_eks_fargate()
{
  LOG_INFO(logger, "Tearing down CoreKube on AWS EKS with Fargate...");

  // Delete the Fargate profile and the nodegroups for both clusters
  eksctl_delete_resource(
    "fargateprofile",
    {
      "--cluster=corekube-aws-cluster",
      "--name=fp-corekube",
    }
  )
    .get();

  eksctl_delete_resource(
    "nodegroup",
    {
      "--cluster=corekube-aws-cluster",
      "--name=ng-corekube",
    }
  );

  eksctl_delete_resource(
    "nodegroup",
    {
      "--cluster=nervion-aws-cluster",
      "--name=ng-nervion",
    }
  );

  // The above futures will complete immediately because we don't use --wait.
  // We choose to do so because we can get more detailed information about the
  // deletion process by querying the events directly.

  // The above ones (nodegroups + fargate profile) need to be deleted before
  // attempting to delete clusters, since they have a dependency.
  eksctl_deletion_details("eksctl-corekube-aws-cluster-fargate").get();
  eksctl_deletion_details("eksctl-corekube-aws-cluster-nodegroup-ng-corekube")
    .get();
  eksctl_deletion_details("eksctl-nervion-aws-cluster-nodegroup-ng-nervion")
    .get();

  // Delete the Nervion cluster first (as it doesn't depend on anything else)
  eksctl_delete_resource("cluster", {"--name=nervion-aws-cluster"});
  eksctl_delete_resource("cluster", {"--name=corekube-aws-cluster"});

  eksctl_deletion_details("eksctl-nervion-aws-cluster-cluster").get();
  eksctl_deletion_details("eksctl-corekube-aws-cluster-cluster").get();

  LOG_INFO(logger, "CoreKube tore down successfully!");
}

void DeployApp::deploy_aws_eks_ec2() { return; }

void DeployApp::teardown_aws_eks_ec2() { return; }

void DeployApp::deploy_aws_eks_ec2_spot() { return; }

void DeployApp::teardown_aws_eks_ec2_spot() { return; }

void DeployApp::deploy_aws_ec2() { return; }

void DeployApp::teardown_aws_ec2() { return; }

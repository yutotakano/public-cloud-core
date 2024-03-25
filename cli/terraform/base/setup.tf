# Configure the AWS Provider: all subsequent resources under this provider will
# be created in the specified region.

provider "aws" {
  region = "eu-north-1"
}

# Data source to fetch only the available AWS availability zones (excluding
# local zones since EKS nodegroups don't support them)
data "aws_availability_zones" "available" {
  state = "available"

  filter {
    name   = "opt-in-status"
    values = ["opt-in-not-required"]
  }
}

# Register the machine's SSH key with AWS
resource "aws_key_pair" "personal_key" {
  key_name   = "personal_key"
  public_key = file("~/.ssh/id_ed25519.pub")
}

# Local variables
locals {
  ck_cluster_name = "corekube-eks"
  nv_cluster_name = "nervion-eks"
}

# Use the eks module to create the AWS VPC: this uses the aws provider
module "vpc" {
  source  = "terraform-aws-modules/vpc/aws"
  version = "5.0.0"

  name = "corekube-vpc"

  cidr = "192.168.0.0/16"
  azs  = slice(data.aws_availability_zones.available.names, 0, 3)

  # /24 with 256*3 addresses was too small to run experiments with 1000 UEs, so
  # we're using /22 with 1024*3 addresses
  private_subnets = ["192.168.0.0/22", "192.168.4.0/22", "192.168.8.0/22"]
  public_subnets  = ["192.168.12.0/22", "192.168.16.0/22", "192.168.20.0/22"]

  enable_nat_gateway = true
  single_nat_gateway = true
  # single_nat_gateway   = false
  # one_nat_gateway_per_az = true
  enable_dns_support   = true
  enable_dns_hostnames = true
  create_igw           = true

  # These tags are required for things like the load balancer
  public_subnet_tags = {
    "kubernetes.io/cluster/${local.ck_cluster_name}" = "shared"
    "kubernetes.io/cluster/${local.nv_cluster_name}" = "shared"
    "kubernetes.io/role/elb"                         = 1
  }

  private_subnet_tags = {
    "kubernetes.io/cluster/${local.ck_cluster_name}" = "shared"
    "kubernetes.io/cluster/${local.nv_cluster_name}" = "shared"
    "kubernetes.io/role/internal-elb"                = 1
  }
}

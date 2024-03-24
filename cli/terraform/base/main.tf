terraform {
  required_providers {
    aws = {
      source  = "hashicorp/aws"
      version = "~> 5.37"
    }
  }

  required_version = ">= 1.2.0"
}



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
  enable_dns_support = true
  enable_dns_hostnames = true
  create_igw = true

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

locals {
  default_ck_nodegroups = {
    frontend = {
      name = "corekube-ng-1"

      instance_types = ["t3.small"]

      min_size     = 1
      max_size     = 1
      desired_size = 1
      volume_size  = 20

      # SSH key pair to allow for direct node access
      key_name = "personal_key"
    }
  }
}


# Use the eks module to create the EKS cluster: uses the aws provider to create
# and the kubernetes provider to setup the EKS service
module "ck_cluster" {
  source  = "terraform-aws-modules/eks/aws"
  version = "20.2.1"

  cluster_name    = local.ck_cluster_name
  cluster_version = "1.28"

  vpc_id                                   = module.vpc.vpc_id
  subnet_ids                               = module.vpc.private_subnets
  enable_cluster_creator_admin_permissions = true
  cluster_endpoint_public_access           = true
  cluster_endpoint_private_access          = true
  enable_irsa                              = true

  # Allow all ingress traffic to the VPC's default security group. Otherwise
  # things like DNS resolution (UDP port 53) won't work, so even if pinging
  # IP might work, it can't resolve any cluster DNSes (svc.cluster.internal) or
  # domains (like the apt repo)
  node_security_group_additional_rules = {
    ingress_self_all  = {
      description      = "Allow all incoming traffic"
      type             = "ingress"
      from_port        = 0
      to_port          = 0
      protocol         = "-1"
      cidr_blocks      = ["0.0.0.0/0"]
      security_group_id = module.vpc.default_security_group_id
    }
  }

  # Use AmazonLinux2 for the EKS worker nodes
  eks_managed_node_group_defaults = {
    ami_type = "AL2_x86_64"
  }

  # Create a node group for the frontend and other kube-system pods: this isn't
  # meant to scale
  eks_managed_node_groups = merge(local.default_ck_nodegroups, var.deployment_type == "ec2" ? {
    // Define a spot worker node group for the CoreKube cluster only if the
    // deployment_type variable is set to "spot"
    spot_workers = {
      name = "corekube-ng-1"

      instance_types = [var.ec2_instance_type]

      min_size     = 1
      max_size     = 20
      desired_size = 1
      volume_size  = 20

      # SSH key pair to allow for direct node access
      key_name = "personal_key"

      capacity_type  = "SPOT"

      tags = {
        "k8s.io/cluster-autoscaler/enabled"                  = "true"
        "k8s.io/cluster-autoscaler/${local.ck_cluster_name}" = "owned"
        "kubernetes.io/cluster/${local.ck_cluster_name}"     = "owned"
      }
    }
  } : null)

  # Create a fargate profile for the actual CoreKube worker pods if type is
  # fargate
  fargate_profiles = var.deployment_type == "fargate" ? {
    app_wildcard = {
      selectors = [
        { namespace = "default" },
        { namespace = "grafana" },
        { namespace = "prometheus" }
      ]
    }
  } : {}

  # Enable Prefix Delegation for the VPC CNI, so we get more IPv4 addresses:
  # this is required to simply run more pods
  cluster_addons = {
    kube-proxy = {
      most_recent = true
    }
    vpc-cni = {
      most_recent = true # To ensure access to the latest settings provided
      configuration_values = jsonencode({
        env = {
          ENABLE_PREFIX_DELEGATION = "true"
          WARM_PREFIX_TARGET       = "1"
        }
      })
    }
  }
}

// Make sure eks-cluster-sg-corekube-eks-365405836 (the security group for the
// EKS cluster, NOT the additional security group for the cluster, nor the node
// security group) allows all incoming traffic -- this is because the Fargate
// pods inherit the security group of the EKS cluster, and while pods within
// Fargate can reach each other without any security group rules, the frontend
// being a node group cannot.
resource "aws_security_group_rule" "ingress_all" {
  count = var.deployment_type == "fargate" ? 1 : 0
  description      = "Allow all incoming traffic from nodes (inherited by Fargate pods; needed for nodegroups to reach Fargate)"
  type        = "ingress"
  from_port   = 0
  to_port     = 0
  protocol    = "-1"
  cidr_blocks = ["0.0.0.0/0"]
  security_group_id = module.ck_cluster.cluster_primary_security_group_id
}


# Create the Nervion cluster: this should use the VPC created for the CoreKube,
# and have auto-scaling groups for the worker nodes
module "nv_cluster" {
  source  = "terraform-aws-modules/eks/aws"
  version = "20.2.1"

  cluster_name    = local.nv_cluster_name
  cluster_version = "1.28"

  vpc_id                                   = module.vpc.vpc_id
  subnet_ids                               = module.vpc.private_subnets
  enable_cluster_creator_admin_permissions = true
  cluster_endpoint_public_access           = true
  cluster_endpoint_private_access          = true
  enable_irsa                              = true

  # Allow all ingress traffic to the VPC's default security group. Otherwise
  # things like DNS resolution (UDP port 53) won't work, so even if pinging
  # IP might work, it can't resolve any cluster DNSes (svc.cluster.internal) or
  # domains (like the apt repo)
  node_security_group_additional_rules = {
    ingress_self_all  = {
      description      = "Allow all incoming traffic"
      type             = "ingress"
      from_port        = 0
      to_port          = 0
      protocol         = "-1"
      cidr_blocks      = ["0.0.0.0/0"]
      security_group_id = module.vpc.default_security_group_id
    }
  }

  # Use AmazonLinux2 for the EKS worker nodes
  eks_managed_node_group_defaults = {
    ami_type = "AL2_x86_64"
  }

  # Create a node group for the frontend and other kube-system pods: this isn't
  # meant to scale
  eks_managed_node_groups = {
    nervion = {
      name = "nervion-ng-1"

      instance_types = ["t3.small"]
      capacity_type  = "ON_DEMAND"

      min_size     = 1
      max_size     = 10
      desired_size = 3
      volume_size  = 20

      # SSH key pair to allow for direct node access
      key_name = "personal_key"

      tags = {
        "k8s.io/cluster-autoscaler/enabled"                  = "true"
        "k8s.io/cluster-autoscaler/${local.nv_cluster_name}" = "owned"
        "kubernetes.io/cluster/${local.nv_cluster_name}"     = "owned"
        "k8s.io/cluster-autoscaler/node-template/label/role" = "nervion-scaling-ng"
      }
    }
  }

  # Enable Prefix Delegation for the VPC CNI, so we get more IPv4 addresses:
  # this is required to simply run more pods
  cluster_addons = {
    kube-proxy = {
      most_recent = true
    }
    vpc-cni = {
      resolve_conflicts = "OVERWRITE"
      before_compute    = true
      most_recent       = true # To ensure access to the latest settings provided
      configuration_values = jsonencode({
        env = {
          ENABLE_PREFIX_DELEGATION = "true"
          WARM_PREFIX_TARGET       = "1"
        }
      })
    }
  }
}

locals {
  nervion_scaling_ng_asg_taglist = {
    "k8s.io/cluster-autoscaler/enabled"                  = "true"
    "k8s.io/cluster-autoscaler/${local.nv_cluster_name}" = "owned"
    "kubernetes.io/cluster/${local.nv_cluster_name}"     = "owned"
    "k8s.io/cluster-autoscaler/node-template/label/role" = "nervion-scaling-ng"
  }
}

resource "aws_autoscaling_group_tag" "scaling_ng" {
  for_each               = local.nervion_scaling_ng_asg_taglist
  autoscaling_group_name = element(module.nv_cluster.eks_managed_node_groups_autoscaling_group_names, 0)

  tag {
    key                 = each.key
    value               = each.value
    propagate_at_launch = true
  }
}

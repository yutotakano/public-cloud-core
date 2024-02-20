terraform {
  required_providers {
    aws = {
      source  = "hashicorp/aws"
      version = "~> 5.37"
    }
    helm = {
      source  = "hashicorp/helm"
      version = ">= 2.11"
    }
    kubectl = {
      source  = "alekc/kubectl"
      version = ">= 2.0"
    }
  }

  required_version = ">= 1.2.0"
}

# Configure the AWS Provider: all subsequent resources under this provider will
# be created in the specified region.

provider "aws" {
  region = "eu-north-1"
}

# Register the machine's SSH key with AWS
resource "aws_key_pair" "personal_key" {
  key_name   = "personal_key"
  public_key = file("~/.ssh/id_ed25519.pub")
}

# Filter out local zones, which are not currently supported with managed node
# groups
data "aws_availability_zones" "available" {
  filter {
    name   = "opt-in-status"
    values = ["opt-in-not-required"]
  }
}

# Local variables
locals {
  ck_cluster_name = "corekube-eks-${random_string.suffix.result}"
  nv_cluster_name = "nervion-eks-${random_string.suffix.result}"
}

resource "random_string" "suffix" {
  length  = 8
  special = false
}

# Use the eks module to create the AWS VPC: this uses the aws provider
module "vpc" {
  source  = "terraform-aws-modules/vpc/aws"
  version = "5.0.0"

  name = "corekube-vpc"

  cidr = "10.0.0.0/16"
  azs  = slice(data.aws_availability_zones.available.names, 0, 3)

  private_subnets = ["10.0.1.0/24", "10.0.2.0/24", "10.0.3.0/24"]
  public_subnets  = ["10.0.4.0/24", "10.0.5.0/24", "10.0.6.0/24"]

  enable_nat_gateway   = true
  single_nat_gateway   = true
  enable_dns_hostnames = true

  # These tags are required for things like the load balancer
  public_subnet_tags = {
    "kubernetes.io/cluster/${local.ck_cluster_name}" = "shared"
    "kubernetes.io/role/elb"                         = 1
  }

  private_subnet_tags = {
    "kubernetes.io/cluster/${local.ck_cluster_name}" = "shared"
    "kubernetes.io/role/internal-elb"                = 1
  }
}

# Use the eks module to create the EKS cluster: uses the aws provider to create
# and the kubernetes provider to setup the EKS service
module "ck_cluster" {
  source  = "terraform-aws-modules/eks/aws"
  version = "19.15.3"

  cluster_name    = local.ck_cluster_name
  cluster_version = "1.29"

  vpc_id                         = module.vpc.vpc_id
  subnet_ids                     = module.vpc.private_subnets
  cluster_endpoint_public_access = true

  # Use AmazonLinux2 for the EKS worker nodes
  eks_managed_node_group_defaults = {
    ami_type = "AL2_x86_64"
  }

  # Create a node group for the frontend and other kube-system pods: this isn't
  # meant to scale
  eks_managed_node_groups = {
    frontend = {
      name = "corekube-ng-1"

      instance_types = ["t3.small"]
      capacity_type  = "ON_DEMAND"

      min_size     = 1
      max_size     = 1
      desired_size = 1
      volume_size  = 20

      # SSH key pair to allow for direct node access
      key_name = "personal_key"
    }
  }

  # Create a fargate profile for the actual CoreKube worker pods
  fargate_profiles = {
    app_wildcard = {
      selectors = [
        { namespace = "default" },
        { namespace = "grafana" },
        { namespace = "prometheus" }
      ]
    }
    kube_system = {
      name = "kube-system"
      selectors = [
        { namespace = "kube-system" }
      ]
    }
  }

  # Enable Prefix Delegation for the VPC CNI, so we get more IPv4 addresses:
  # this is required to simply run more pods
  cluster_addons = {
    vpc-cni = {
      before_compute = true
      most_recent    = true # To ensure access to the latest settings provided
      configuration_values = jsonencode({
        env = {
          ENABLE_PREFIX_DELEGATION = "true"
          WARM_PREFIX_TARGET       = "1"
        }
      })
    }
  }
}


# Create the Nervion cluster: this should use the VPC created for the CoreKube,
# and have auto-scaling groups for the worker nodes
module "nv_cluster" {
  source  = "terraform-aws-modules/eks/aws"
  version = "19.15.3"

  cluster_name    = local.nv_cluster_name
  cluster_version = "1.29"

  vpc_id                         = module.vpc.vpc_id
  subnet_ids                     = module.vpc.private_subnets
  cluster_endpoint_public_access = true

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
      desired_size = 2
      volume_size  = 20

      # SSH key pair to allow for direct node access
      key_name = "personal_key"

      tags = {
        "k8s.io/cluster-autoscaler/enabled"                  = "true"
        "k8s.io/cluster-autoscaler/${local.nv_cluster_name}" = "owned"
      }
    }
  }

  # Enable Prefix Delegation for the VPC CNI, so we get more IPv4 addresses:
  # this is required to simply run more pods
  cluster_addons = {
    vpc-cni = {
      before_compute = true
      most_recent    = true # To ensure access to the latest settings provided
      configuration_values = jsonencode({
        env = {
          ENABLE_PREFIX_DELEGATION = "true"
          WARM_PREFIX_TARGET       = "1"
        }
      })
    }
  }
}


# Data source to fetch the EKS cluster details
data "aws_eks_cluster" "corekube" {
  depends_on = [module.ck_cluster]
  name       = local.ck_cluster_name
}

data "aws_eks_cluster" "nervion" {
  depends_on = [module.nv_cluster]
  name       = local.nv_cluster_name
}

# Data source to fetch the EKS cluster authentication details
data "aws_eks_cluster_auth" "corekube" {
  depends_on = [module.ck_cluster]
  name       = local.ck_cluster_name
}

data "aws_eks_cluster_auth" "nervion" {
  depends_on = [module.nv_cluster]
  name       = local.nv_cluster_name
}


# Deploy the cluster-autoscaler to the Nervion cluster (using Helm, which means
# re-stating the Kubernetes configuration again)

provider "helm" {
  kubernetes {
    host                   = data.aws_eks_cluster.nervion.endpoint
    cluster_ca_certificate = base64decode(data.aws_eks_cluster.nervion.certificate_authority.0.data)
    token                  = data.aws_eks_cluster_auth.nervion.token
  }
}

# Data source to fetch the current AWS region
data "aws_region" "current" {}

# Deploy the cluster-autoscaler to the Nervion cluster
resource "helm_release" "cluster_autoscaler" {
  name       = "cluster-autoscaler"
  repository = "https://kubernetes.github.io/autoscaler"
  chart      = "cluster-autoscaler"
  namespace  = "kube-system"
  version    = "9.35.0"

  set {
    name  = "autoDiscovery.enabled"
    value = "true"
  }

  set {
    name  = "autoDiscovery.clusterName"
    value = local.nv_cluster_name
  }

  set {
    name  = "cloudProvider"
    value = "aws"
  }

  set {
    name  = "awsRegion"
    value = data.aws_region.current.name
  }

  set {
    name  = "rbac.create"
    value = "true"
  }

  set {
    name  = "sslCertPath"
    value = "/etc/ssl/certs/ca-bundle.crt"
  }
}

# Deploy CoreKube resources to the CoreKube cluster
# module "corekube" {
#   source  = "./corekube"
#   depends_on = [module.ck_cluster]
#   cluster_name = local.ck_cluster_name
#   kubeconfig = module.ck_kubeconfig.kubeconfig
# }

provider "kubectl" {
  apply_retry_count      = 5
  load_config_file       = false
  host                   = data.aws_eks_cluster.corekube.endpoint
  cluster_ca_certificate = base64decode(data.aws_eks_cluster.corekube.certificate_authority.0.data)
  token                  = data.aws_eks_cluster_auth.corekube.token
  alias                  = "corekube"
}

resource "kubectl_manifest" "corekube_metrics_server" {
  yaml_body = file("${path.module}/scripts/configs/metrics-server.yaml")
  provider  = kubectl.corekube
}

resource "kubectl_manifest" "corekube_grafana" {
  yaml_body = file("${path.module}/scripts/configs/grafana.yaml")
  provider  = kubectl.corekube
}

resource "kubectl_manifest" "corekube_prometheus" {
  yaml_body = file("${path.module}/scripts/configs/prometheus.yaml")
  provider  = kubectl.corekube
}

resource "kubectl_manifest" "corekube_opencost" {
  yaml_body = file("${path.module}/scripts/configs/opencost.yaml")
  provider  = kubectl.corekube
}

resource "kubectl_manifest" "corekube_frontend" {
  yaml_body = file("${path.module}/scripts/configs/5G/corekube-frontend.yaml")
  provider  = kubectl.corekube
}

resource "kubectl_manifest" "corekube_workers" {
  yaml_body = file("${path.module}/scripts/configs/5G/corekube-worker-and-db.yaml")
  provider  = kubectl.corekube
}

provider "kubectl" {
  apply_retry_count      = 5
  load_config_file       = false
  host                   = data.aws_eks_cluster.nervion.endpoint
  cluster_ca_certificate = base64decode(data.aws_eks_cluster.nervion.certificate_authority.0.data)
  token                  = data.aws_eks_cluster_auth.nervion.token
  alias                  = "nervion"
}

resource "kubectl_manifest" "nervion_metrics_server" {
  yaml_body = file("${path.module}/scripts/configs/metrics-server.yaml")
  provider  = kubectl.nervion
}

resource "kubectl_manifest" "nervion_ran_controller" {
  yaml_body = file("${path.module}/scripts/configs/nervion.yaml")
  provider  = kubectl.nervion
}

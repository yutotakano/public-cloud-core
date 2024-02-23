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

  required_version = ">= 1.7.0"
}

locals {
  ck_cluster_name = "corekube-eks"
  nv_cluster_name = "nervion-eks"
}

# Data source to fetch the EKS cluster details
data "aws_eks_cluster" "corekube" {
  name       = local.ck_cluster_name
}

data "aws_eks_cluster" "nervion" {
  name       = local.nv_cluster_name
}

# Data source to fetch the EKS cluster authentication details
data "aws_eks_cluster_auth" "corekube" {
  name       = local.ck_cluster_name
}

data "aws_eks_cluster_auth" "nervion" {
  name       = local.nv_cluster_name
}

# Data source to fetch the current AWS region
data "aws_region" "current" {}

# Deploy the cluster-autoscaler to the Nervion cluster (using Helm, which means
# re-stating the Kubernetes configuration again)

provider "helm" {
  kubernetes {
    host                   = data.aws_eks_cluster.nervion.endpoint
    cluster_ca_certificate = base64decode(data.aws_eks_cluster.nervion.certificate_authority.0.data)
    token                  = data.aws_eks_cluster_auth.nervion.token
  }
}

# Deploy the cluster-autoscaler to the Nervion cluster
resource "helm_release" "nv_cluster_autoscaler" {
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
    name  = "rbac.serviceAccount.name"
    value = "cluster-autoscaler-aws"
  }

  set {
    name  = "rbac.serviceAccount.annotations.eks\\.amazonaws\\.com/role-arn"
    value = module.nv_cluster_autoscaler_irsa.iam_role_arn
    type  = "string"
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

data "aws_iam_openid_connect_provider" "nv_cluster_oidc" {
  url = data.aws_eks_cluster.nervion.identity.0.oidc.0.issuer
}

# Create an IAM role for the cluster-autoscaler to use
module "nv_cluster_autoscaler_irsa" {
  source     = "terraform-aws-modules/iam/aws//modules/iam-role-for-service-accounts-eks"
  version    = "~> 5.34"

  role_name_prefix = "cluster-autoscaler"
  role_description = "EKS Cluster ${local.nv_cluster_name} Autoscaler"

  attach_cluster_autoscaler_policy = true
  cluster_autoscaler_cluster_names   = [local.nv_cluster_name]

  oidc_providers = {
    main = {
      provider_arn               = data.aws_iam_openid_connect_provider.nv_cluster_oidc.arn
      namespace_service_accounts = ["kube-system:cluster-autoscaler-aws"]
    }
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
  apply_retry_count      = 3
  load_config_file       = false
  # host                   = data.aws_eks_cluster.corekube.endpoint
  # cluster_ca_certificate = base64decode(data.aws_eks_cluster.corekube.certificate_authority[0].data)
  # token                  = data.aws_eks_cluster_auth.corekube.token

  host                   = data.aws_eks_cluster.corekube.endpoint
  cluster_ca_certificate = base64decode(data.aws_eks_cluster.corekube.certificate_authority[0].data)
  exec {
    api_version = "client.authentication.k8s.io/v1beta1"
    args        = ["eks", "get-token", "--cluster-name", data.aws_eks_cluster.corekube.name, "--region", data.aws_region.current.name, "--output", "json"]
    command     = "aws"
  }


  alias = "corekube"
}

resource "kubectl_manifest" "corekube_metrics_server" {
  yaml_body  = file("${path.module}/../../scripts/configs/metrics-server.yaml")
  provider   = kubectl.corekube
}

resource "kubectl_manifest" "corekube_grafana" {
  yaml_body  = file("${path.module}/../../scripts/configs/grafana.yaml")
  provider   = kubectl.corekube
}

resource "kubectl_manifest" "corekube_prometheus" {
  yaml_body  = file("${path.module}/../../scripts/configs/prometheus.yaml")
  provider   = kubectl.corekube
}

resource "kubectl_manifest" "corekube_opencost" {
  yaml_body  = file("${path.module}/../../scripts/configs/opencost.yaml")
  provider   = kubectl.corekube
}

resource "kubectl_manifest" "corekube_frontend" {
  yaml_body  = file("${path.module}/../../scripts/configs/5G/corekube-frontend.yaml")
  provider   = kubectl.corekube
}

resource "kubectl_manifest" "corekube_workers" {
  yaml_body  = file("${path.module}/../../scripts/configs/5G/corekube-worker-and-db.yaml")
  provider   = kubectl.corekube
}

provider "kubectl" {
  apply_retry_count      = 3
  load_config_file       = false
  host                   = data.aws_eks_cluster.nervion.endpoint
  cluster_ca_certificate = base64decode(data.aws_eks_cluster.nervion.certificate_authority.0.data)
  token = data.aws_eks_cluster_auth.nervion.token
  alias = "nervion"
}

resource "kubectl_manifest" "nervion_metrics_server" {
  yaml_body  = file("${path.module}/../../scripts/configs/metrics-server.yaml")
  provider   = kubectl.nervion
}

resource "kubectl_manifest" "nervion_ran_controller" {
  yaml_body  = file("${path.module}/../../scripts/configs/nervion.yaml")
  provider   = kubectl.nervion
}

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
    kubernetes = {
      source  = "hashicorp/kubernetes"
      version = ">= 2.26"
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
    # host                   = data.aws_eks_cluster.nervion.endpoint
    # cluster_ca_certificate = base64decode(data.aws_eks_cluster.nervion.certificate_authority.0.data)
    # token                  = data.aws_eks_cluster_auth.nervion.token

    host                   = data.aws_eks_cluster.nervion.endpoint
    cluster_ca_certificate = base64decode(data.aws_eks_cluster.nervion.certificate_authority[0].data)
    exec {
      api_version = "client.authentication.k8s.io/v1beta1"
      args        = ["eks", "get-token", "--cluster-name", data.aws_eks_cluster.nervion.name, "--region", data.aws_region.current.name, "--output", "json"]
      command     = "aws"
    }
  }
  alias = "nervion"
}

# Deploy the cluster-autoscaler to the Nervion cluster
resource "helm_release" "nv_cluster_autoscaler" {
  provider   = helm.nervion
  name       = "nv-cluster-autoscaler-release"
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
    name  = "rbac.serviceAccount.name"
    value = "nv-cluster-autoscaler"
  }

  set {
    name  = "rbac.serviceAccount.annotations.eks\\.amazonaws\\.com/role-arn"
    value = module.nv_cluster_autoscaler_irsa.iam_role_arn
    type  = "string"
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

  role_name = "nv-cluster-autoscaler"
  role_description = "EKS Cluster ${local.nv_cluster_name} Autoscaler"

  attach_cluster_autoscaler_policy = true
  cluster_autoscaler_cluster_names   = [local.nv_cluster_name]

  oidc_providers = {
    main = {
      provider_arn               = data.aws_iam_openid_connect_provider.nv_cluster_oidc.arn
      namespace_service_accounts = ["kube-system:nv-cluster-autoscaler"]
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

data "kubectl_path_documents" "common_manifest" {
  pattern = "${path.module}/../../scripts/configs/common/*.yaml"
}

data "kubectl_path_documents" "corekube_manifest" {
  pattern = "${path.module}/../../scripts/configs/corekube/*.yaml"
}

data "kubectl_path_documents" "nervion_manifest" {
  pattern = "${path.module}/../../scripts/configs/nervion/*.yaml"
}

resource "kubectl_manifest" "corekube_common_setup" {
  # Need to keep AWS Load Balancer Controller activated while we destroy the
  # deployments so this dependency is explicitly stated
  depends_on = [ helm_release.alb-controller, module.ck_loadbalancer_irsa ]
  for_each  = data.kubectl_path_documents.common_manifest.manifests
  yaml_body = each.value
  wait = true
  provider   = kubectl.corekube
}

resource "kubectl_manifest" "corekube_setup" {
  # Need to keep AWS Load Balancer Controller activated while we destroy the
  # deployments so this dependency is explicitly stated
  depends_on = [ helm_release.alb-controller, module.ck_loadbalancer_irsa ]
  for_each  = data.kubectl_path_documents.corekube_manifest.manifests
  yaml_body = each.value
  wait = true
  provider   = kubectl.corekube
}

provider "kubernetes" {
  host                   = data.aws_eks_cluster.corekube.endpoint
  cluster_ca_certificate = base64decode(data.aws_eks_cluster.corekube.certificate_authority[0].data)
  exec {
    api_version = "client.authentication.k8s.io/v1beta1"
    args        = ["eks", "get-token", "--cluster-name", data.aws_eks_cluster.corekube.name, "--region", data.aws_region.current.name, "--output", "json"]
    command     = "aws"
  }
}

resource "kubernetes_config_map" "corekube_dashboard_configmap" {
  metadata {
    name      = "corekube-grafana-dashboards"
  }

  data = {
    "main-dashboard.json" = file("${path.module}/../../scripts/configs/corekube/dashboards/main-dashboard.json")
  }
}

provider "kubectl" {
  apply_retry_count      = 3
  load_config_file       = false
  # host                   = data.aws_eks_cluster.nervion.endpoint
  # cluster_ca_certificate = base64decode(data.aws_eks_cluster.nervion.certificate_authority.0.data)
  # token = data.aws_eks_cluster_auth.nervion.token

  host                   = data.aws_eks_cluster.nervion.endpoint
  cluster_ca_certificate = base64decode(data.aws_eks_cluster.nervion.certificate_authority[0].data)
  exec {
    api_version = "client.authentication.k8s.io/v1beta1"
    args        = ["eks", "get-token", "--cluster-name", data.aws_eks_cluster.nervion.name, "--region", data.aws_region.current.name, "--output", "json"]
    command     = "aws"
  }
  alias = "nervion"
}

resource "kubectl_manifest" "nervion_common_setup" {
  # Need to keep AWS Load Balancer Controller activated while we destroy the
  # deployments so this dependency is explicitly stated
  depends_on = [ helm_release.nv_alb_controller, module.nv_loadbalancer_irsa ]
  wait = true
  for_each  = data.kubectl_path_documents.common_manifest.manifests
  yaml_body = each.value
  provider   = kubectl.nervion
}

resource "kubectl_manifest" "nervion_setup" {
  # Need to keep AWS Load Balancer Controller activated while we destroy the
  # deployments so this dependency is explicitly stated
  depends_on = [ helm_release.nv_alb_controller, module.nv_loadbalancer_irsa ]
  wait = true
  for_each  = data.kubectl_path_documents.nervion_manifest.manifests
  yaml_body = each.value
  provider   = kubectl.nervion
}


## Use AWS Load Balancer Controller (set up role and install) since Fargate
## doesn't support the classic LB

data "aws_iam_openid_connect_provider" "ck_cluster_oidc" {
  url = data.aws_eks_cluster.corekube.identity.0.oidc.0.issuer
}

module "ck_loadbalancer_irsa" {
  source     = "terraform-aws-modules/iam/aws//modules/iam-role-for-service-accounts-eks"
  version    = "~> 5.34"

  role_name = "load-balancer-controller"
  role_description = "EKS Cluster ${local.ck_cluster_name} Load Balancer Controller"

  attach_load_balancer_controller_policy = true

  oidc_providers = {
    main = {
      provider_arn               = data.aws_iam_openid_connect_provider.ck_cluster_oidc.arn
      namespace_service_accounts = ["kube-system:aws-load-balancer-controller"]
    }
  }
}


# Install the AWS Load Balancer Controller

provider "helm" {
  kubernetes {
    host                   = data.aws_eks_cluster.corekube.endpoint
    cluster_ca_certificate = base64decode(data.aws_eks_cluster.corekube.certificate_authority[0].data)
    exec {
      api_version = "client.authentication.k8s.io/v1beta1"
      args        = ["eks", "get-token", "--cluster-name", data.aws_eks_cluster.corekube.name, "--region", data.aws_region.current.name, "--output", "json"]
      command     = "aws"
    }
  }
  alias = "corekube"
}

data "aws_vpc" "corekube_vpc" {
  tags = {
    "Name" = "corekube-vpc"
  }
}

resource "helm_release" "alb-controller" {
  provider   = helm.corekube
  name       = "aws-load-balancer-controller"
  repository = "https://aws.github.io/eks-charts"
  chart      = "aws-load-balancer-controller"
  namespace  = "kube-system"

  set {
    name  = "region"
    value = data.aws_region.current.name
  }

  set {
    name  = "vpcId"
    value = data.aws_vpc.corekube_vpc.id
  }

  set {
    name  = "serviceAccount.create"
    value = "true"
  }

  set {
    name  = "serviceAccount.name"
    value = "aws-load-balancer-controller"
  }

  set {
    name  = "serviceAccount.annotations.eks\\.amazonaws\\.com/role-arn"
    value = module.ck_loadbalancer_irsa.iam_role_arn
  }

  set {
    name  = "clusterName"
    value = local.ck_cluster_name
  }

  // This is required for the AWS Load Balancer Controller since otherwise
  // the Helm chart when destroyed will not delete the security groups (bug?)
  // and the VPC will not be deleted due to this hanging resource
  set {
    name = "enableBackendSecurityGroup"
    value = "false"
  }
}


module "ck_kubestatemetrics_irsa" {
  source     = "terraform-aws-modules/iam/aws//modules/iam-role-for-service-accounts-eks"
  version    = "~> 5.34"

  role_name = "kube-state-metrics-role"
  role_description = "EKS Cluster ${local.ck_cluster_name} Kube State Metrics Role"

  oidc_providers = {
    main = {
      provider_arn               = data.aws_iam_openid_connect_provider.ck_cluster_oidc.arn
      namespace_service_accounts = ["kube-system:kube-state-metrics-sa"]
    }
  }
}

# kube-state-metrics to export pod/node metrics to Prometheus for CoreKube

resource "helm_release" "kube-state-metrics" {
  provider   = helm.corekube
  name       = "kube-state-metrics"
  repository = "https://prometheus-community.github.io/helm-charts"
  chart      = "kube-state-metrics"
  namespace  = "kube-system"

  set {
    name = "podAnnotations.prometheus\\.io/scrape"
    value = "true"
    type = "string"
  }

  set {
    name = "podAnnotations.prometheus\\.io/path"
    value = "/metrics"
  }

  set {
    name = "podAnnotations.prometheus\\.io/port"
    value = "8080"
    type = "string"
  }

  set {
    name  = "serviceAccount.create"
    value = "true"
  }

  set {
    name  = "serviceAccount.name"
    value = "kube-state-metrics-sa"
  }

  set {
    name  = "serviceAccount.annotations.eks\\.amazonaws\\.com/role-arn"
    value = module.ck_kubestatemetrics_irsa.iam_role_arn
  }
}


# Deploy the cluster-autoscaler to the CoreKube cluster
resource "helm_release" "ck_cluster_autoscaler" {
  provider   = helm.corekube
  name       = "ck-cluster-autoscaler-release"
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
    value = local.ck_cluster_name
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
    name  = "rbac.serviceAccount.name"
    value = "ck-cluster-autoscaler"
  }

  set {
    name  = "rbac.serviceAccount.annotations.eks\\.amazonaws\\.com/role-arn"
    value = module.ck_cluster_autoscaler_irsa.iam_role_arn
    type  = "string"
  }

  set {
    name  = "sslCertPath"
    value = "/etc/ssl/certs/ca-bundle.crt"
  }
}

# Create an IAM role for the cluster-autoscaler to use
module "ck_cluster_autoscaler_irsa" {
  source     = "terraform-aws-modules/iam/aws//modules/iam-role-for-service-accounts-eks"
  version    = "~> 5.34"

  role_name = "ck-cluster-autoscaler"
  role_description = "EKS Cluster ${local.ck_cluster_name} Autoscaler"

  attach_cluster_autoscaler_policy = true
  cluster_autoscaler_cluster_names   = [local.ck_cluster_name]

  oidc_providers = {
    main = {
      provider_arn               = data.aws_iam_openid_connect_provider.ck_cluster_oidc.arn
      namespace_service_accounts = ["kube-system:ck-cluster-autoscaler"]
    }
  }
}

# Nervion

module "nv_loadbalancer_irsa" {
  source     = "terraform-aws-modules/iam/aws//modules/iam-role-for-service-accounts-eks"
  version    = "~> 5.34"

  role_name = "nv-load-balancer-controller"
  role_description = "EKS Cluster ${local.nv_cluster_name} Load Balancer Controller"

  attach_load_balancer_controller_policy = true

  oidc_providers = {
    main = {
      provider_arn               = data.aws_iam_openid_connect_provider.nv_cluster_oidc.arn
      namespace_service_accounts = ["kube-system:aws-nv-load-balancer-controller"]
    }
  }
}

# Install the AWS Load Balancer Controller
resource "helm_release" "nv_alb_controller" {
  provider   = helm.nervion
  name       = "aws-load-balancer-controller"
  repository = "https://aws.github.io/eks-charts"
  chart      = "aws-load-balancer-controller"
  namespace  = "kube-system"

  set {
    name  = "region"
    value = data.aws_region.current.name
  }

  set {
    name  = "vpcId"
    value = data.aws_vpc.corekube_vpc.id
  }

  set {
    name  = "serviceAccount.create"
    value = "true"
  }

  set {
    name  = "serviceAccount.name"
    value = "aws-nv-load-balancer-controller"
  }

  set {
    name  = "serviceAccount.annotations.eks\\.amazonaws\\.com/role-arn"
    value = module.nv_loadbalancer_irsa.iam_role_arn
  }

  set {
    name  = "clusterName"
    value = local.nv_cluster_name
  }

  set {
    name = "enableBackendSecurityGroup"
    value = "false"
  }
}

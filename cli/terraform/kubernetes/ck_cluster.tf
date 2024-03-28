# Data source to fetch the EKS cluster details
data "aws_eks_cluster" "corekube" {
  name = local.ck_cluster_name
}

# Data source to fetch the EKS cluster authentication details
data "aws_eks_cluster_auth" "corekube" {
  name = local.ck_cluster_name
}

data "aws_iam_openid_connect_provider" "ck_cluster_oidc" {
  url = data.aws_eks_cluster.corekube.identity.0.oidc.0.issuer
}

provider "kubectl" {
  apply_retry_count = 3
  load_config_file  = false

  host                   = data.aws_eks_cluster.corekube.endpoint
  cluster_ca_certificate = base64decode(data.aws_eks_cluster.corekube.certificate_authority[0].data)
  exec {
    api_version = "client.authentication.k8s.io/v1beta1"
    args        = ["eks", "get-token", "--cluster-name", data.aws_eks_cluster.corekube.name, "--region", data.aws_region.current.name, "--output", "json"]
    command     = "aws"
  }


  alias = "corekube"
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

resource "kubectl_manifest" "corekube_common_setup" {
  # Need to keep AWS Load Balancer Controller activated while we destroy the
  # deployments so this dependency is explicitly stated
  depends_on = [helm_release.alb-controller, module.ck_loadbalancer_irsa]
  for_each   = data.kubectl_path_documents.common_manifest.manifests
  yaml_body  = each.value
  wait       = true
  provider   = kubectl.corekube
}

resource "kubectl_manifest" "corekube_setup" {
  # Need to keep AWS Load Balancer Controller activated while we destroy the
  # deployments so this dependency is explicitly stated
  depends_on = [helm_release.alb-controller, module.ck_loadbalancer_irsa]
  for_each   = data.kubectl_path_documents.corekube_manifest.manifests
  yaml_body  = each.value
  wait       = true
  provider   = kubectl.corekube
}

resource "kubernetes_config_map" "corekube_dashboard_configmap" {
  metadata {
    name = "corekube-grafana-dashboards"
  }

  data = {
    "main-dashboard.json" = file("${path.module}/../../scripts/configs/corekube/dashboards/main-dashboard.json")
  }
}

## Use AWS Load Balancer Controller (set up role and install) since Fargate
## doesn't support the classic LB, i.e. the ALB is the only option to get any
## traffic into the cluster
module "ck_loadbalancer_irsa" {
  source  = "terraform-aws-modules/iam/aws//modules/iam-role-for-service-accounts-eks"
  version = "~> 5.34"

  role_name        = "load-balancer-controller"
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
    name  = "enableBackendSecurityGroup"
    value = "false"
  }
}


# Create an IAM role for the kube-state-metrics to use
module "ck_kubestatemetrics_irsa" {
  source  = "terraform-aws-modules/iam/aws//modules/iam-role-for-service-accounts-eks"
  version = "~> 5.34"

  role_name        = "kube-state-metrics-role"
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
    name  = "podAnnotations.prometheus\\.io/scrape"
    value = "true"
    type  = "string"
  }

  set {
    name  = "podAnnotations.prometheus\\.io/path"
    value = "/metrics"
  }

  set {
    name  = "podAnnotations.prometheus\\.io/port"
    value = "8080"
    type  = "string"
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

# Create an IAM role for the cluster-autoscaler to use
module "ck_cluster_autoscaler_irsa" {
  source  = "terraform-aws-modules/iam/aws//modules/iam-role-for-service-accounts-eks"
  version = "~> 5.34"

  role_name        = "ck-cluster-autoscaler"
  role_description = "EKS Cluster ${local.ck_cluster_name} Autoscaler"

  attach_cluster_autoscaler_policy = true
  cluster_autoscaler_cluster_names = [local.ck_cluster_name]

  oidc_providers = {
    main = {
      provider_arn               = data.aws_iam_openid_connect_provider.ck_cluster_oidc.arn
      namespace_service_accounts = ["kube-system:ck-cluster-autoscaler"]
    }
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

  
  set {
    name  = "podAnnotations.prometheus\\.io/scrape"
    value = "true"
    type  = "string"
  }

  set {
    name  = "podAnnotations.prometheus\\.io/path"
    value = "/metrics"
  }

  set {
    name  = "podAnnotations.prometheus\\.io/port"
    value = "8085"
    type  = "string"
  }
}


# Install AWS-tweaked KubeCost, following https://docs.aws.amazon.com/eks/latest/userguide/cost-monitoring.html
resource "helm_release" "kubecost-release" {
  provider   = helm.corekube
  name       = "aws-kubecost"

  repository      = "oci://public.ecr.aws/kubecost"
  chart = "cost-analyzer"
  version    = "2.1.1"
  namespace  = "kubecost"

  values = [
    "${file("${path.module}/values-eks-cost-monitoring.yaml")}"
  ]

  create_namespace = true
}

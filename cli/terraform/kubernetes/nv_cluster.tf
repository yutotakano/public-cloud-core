# Data source to fetch the EKS cluster details
data "aws_eks_cluster" "nervion" {
  name = local.nv_cluster_name
}

# Data source to fetch the EKS cluster authentication details
data "aws_eks_cluster_auth" "nervion" {
  name = local.nv_cluster_name
}

# Deploy the cluster-autoscaler to the Nervion cluster (using Helm, which means
# re-stating the Kubernetes configuration again)

provider "helm" {
  kubernetes {
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
  source  = "terraform-aws-modules/iam/aws//modules/iam-role-for-service-accounts-eks"
  version = "~> 5.34"

  role_name        = "nv-cluster-autoscaler"
  role_description = "EKS Cluster ${local.nv_cluster_name} Autoscaler"

  attach_cluster_autoscaler_policy = true
  cluster_autoscaler_cluster_names = [local.nv_cluster_name]

  oidc_providers = {
    main = {
      provider_arn               = data.aws_iam_openid_connect_provider.nv_cluster_oidc.arn
      namespace_service_accounts = ["kube-system:nv-cluster-autoscaler"]
    }
  }
}

provider "kubectl" {
  apply_retry_count = 3
  load_config_file  = false

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
  depends_on = [helm_release.nv_alb_controller, module.nv_loadbalancer_irsa]
  wait       = true
  for_each   = data.kubectl_path_documents.common_manifest.manifests
  yaml_body  = each.value
  provider   = kubectl.nervion
}

resource "kubectl_manifest" "nervion_setup" {
  # Need to keep AWS Load Balancer Controller activated while we destroy the
  # deployments so this dependency is explicitly stated
  depends_on = [helm_release.nv_alb_controller, module.nv_loadbalancer_irsa]
  wait       = true
  for_each   = data.kubectl_path_documents.nervion_manifest.manifests
  yaml_body  = each.value
  provider   = kubectl.nervion
}

# Create an IAM role for the load-balancer-controller to use
module "nv_loadbalancer_irsa" {
  source  = "terraform-aws-modules/iam/aws//modules/iam-role-for-service-accounts-eks"
  version = "~> 5.34"

  role_name        = "nv-load-balancer-controller"
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
    name  = "enableBackendSecurityGroup"
    value = "false"
  }
}

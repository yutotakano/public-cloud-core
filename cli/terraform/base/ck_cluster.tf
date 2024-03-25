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
    ingress_self_all = {
      description       = "Allow all incoming traffic"
      type              = "ingress"
      from_port         = 0
      to_port           = 0
      protocol          = "-1"
      cidr_blocks       = ["0.0.0.0/0"]
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

      capacity_type = "SPOT"

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
    aws-ebs-csi-driver = {
      service_account_role_arn = module.ebs_csi_driver_irsa.iam_role_arn
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
  count             = var.deployment_type == "fargate" ? 1 : 0
  description       = "Allow all incoming traffic from nodes (inherited by Fargate pods; needed for nodegroups to reach Fargate)"
  type              = "ingress"
  from_port         = 0
  to_port           = 0
  protocol          = "-1"
  cidr_blocks       = ["0.0.0.0/0"]
  security_group_id = module.ck_cluster.cluster_primary_security_group_id
}

# For the AWS KubeCost integration, we need t oenable the EBS CSI addon for the
# cluster, which requires an IAM role for the service account as per this example:
# https://github.com/clowdhaus/eks-reference-architecture/blob/main/ephemeral-vol-test/eks.tf
# and as per these docs:
# https://docs.aws.amazon.com/eks/latest/userguide/cost-monitoring.html
module "ebs_csi_driver_irsa" {
  source  = "terraform-aws-modules/iam/aws//modules/iam-role-for-service-accounts-eks"
  version = "~> 5.20"

  # create_role      = false
  role_name_prefix = "corekube-ebs-csi-driver-"

  attach_ebs_csi_policy = true

  oidc_providers = {
    main = {
      provider_arn               = module.ck_cluster.oidc_provider_arn
      namespace_service_accounts = ["kube-system:ebs-csi-controller-sa"]
    }
  }
}

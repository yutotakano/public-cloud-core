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

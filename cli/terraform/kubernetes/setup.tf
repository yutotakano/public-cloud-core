locals {
  ck_cluster_name = "corekube-eks"
  nv_cluster_name = "nervion-eks"
}

# Data source to fetch the current AWS region
data "aws_region" "current" {}

data "aws_vpc" "corekube_vpc" {
  tags = {
    "Name" = "corekube-vpc"
  }
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

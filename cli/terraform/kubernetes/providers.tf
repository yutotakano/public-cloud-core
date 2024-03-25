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

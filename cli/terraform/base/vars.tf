variable "deployment_type" {
  description = "The type of deployment to create: either 'ec2' or 'fargate'"
  type        = string
  default     = "ec2"

  validation {
    condition     = contains(["ec2", "fargate"], var.deployment_type)
    error_message = "deployment_type must be either 'ec2' or 'fargate'"
  }
}

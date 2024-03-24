variable "deployment_type" {
  description = "The type of deployment to create: either 'ec2' or 'fargate'"
  type        = string
  default     = "ec2"

  validation {
    condition     = contains(["ec2", "fargate"], var.deployment_type)
    error_message = "deployment_type must be either 'ec2' or 'fargate'"
  }
}

variable "ec2_instance_type" {
  description = "The instance type to use for the EC2 worker nodes"
  type        = string
  default     = "c5.large"
}

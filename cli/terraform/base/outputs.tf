// Output the deployment type so that it's stored as a Terraform output and
// can be used for e.g. destroying
output "deployment_type" {
  value = var.deployment_type
}

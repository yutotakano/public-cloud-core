#!/bin/bash

eksctl create cluster --name corekube-aws-cluster --region eu-north-1 --version 1.28 --without-nodegroup --node-private-networking --ssh-access --ssh-public-key "$USERPROFILE/.ssh/id_ed25519.pub"

# add just a frontend node
eksctl create nodegroup --cluster corekube-aws-cluster --name ng-corekube --node-ami-family AmazonLinux2 --node-type t3.small --nodes 1 --nodes-min 1 --nodes-max 1 --node-volume-size 20 --ssh-access --ssh-public-key "$USERPROFILE/.ssh/id_ed25519.pub" --managed --asg-access

# Use a custom fargate profile instead of the one supplied by --fargate on
# eksctl create cluster. This is because otherwise grafana and prometheus
# namespaces won't use fargate.
eksctl create fargateprofile --namespace default --namespace kube-system --namespace grafana --namespace prometheus --cluster corekube-aws-cluster --name fp-corekube

# Get the public subnets (separated by tab) that were created using eksctl
# create cluster
PUBLIC_SUBNETS=$(aws ec2 describe-subnets  --filter "Name=tag:alpha.eksctl.io/cluster-name,Values=corekube-aws-cluster" "Name=tag:aws:cloudformation:logical-id,Values=SubnetPublic*" --query "Subnets[*].SubnetId" | jq -r ". | join(\",\")")

# Get the private subnets (separated by tab) that were created using eksctl
# create cluster
PRIVATE_SUBNETS=$(aws ec2 describe-subnets  --filter "Name=tag:alpha.eksctl.io/cluster-name,Values=corekube-aws-cluster" "Name=tag:aws:cloudformation:logical-id,Values=SubnetPrivate*" --query "Subnets[*].SubnetId" | jq -r ". | join(\",\")")

# Get the internal IP of the frontend node (found with the nodegroup name)
FRONTEND_NODE_INTERNAL_IP=$(kubectl get nodes -o "jsonpath={.items[0].status.addresses[?(@.type==\"InternalIP\")].address}" -l alpha.eksctl.io/nodegroup-name=ng-corekube)

kubectl apply -f ../nervion-powder/config/test/metrics-server.yaml
kubectl apply -f ../nervion-powder/config/test/5G/deployment-excluding-frontend.yaml
kubectl apply -f ../nervion-powder/config/test/5G/deployment-frontend-only.yaml

# Patch the frontend deployment command to use the internal IP of the frontend node
# kubectl get deployment corekube-frontend -n corekube-frontend -o json | jq '.spec.template.spec.containers[0].command[1] = "'"$FRONTEND_NODE_INTERNAL_IP"'"' | kubectl apply -f -

kubectl create -f ../nervion-powder/config/test/prometheus.yaml

# kubectl create secret generic opencost-csp-secret --from-file=opencost-csp.json -n opencost
kubectl apply --namespace opencost -f opencost.yaml

# Deploy Grafana
kubectl create -f ../nervion-powder/config/test/grafana.yaml
kubectl create configmap corekube-grafana-dashboards --namespace=grafana --from-file=../nervion-powder/config/test/dashboards/
kubectl wait -n grafana --for=condition=ready pod --all --timeout=10m
kubectl port-forward services/corekube-grafana -n grafana 12345:3000 > /dev/null 2>&1 &

# Get the CoreKube cluster security group id
COREKUBE_SECURITY_GROUP_ID=$(aws ec2 describe-instances --filters "Name=tag:eks:cluster-name,Values=corekube-aws-cluster" --query "Reservations[*].Instances[*].SecurityGroups[?starts_with(GroupName, \`eks-\`)].GroupId | [0]" --output=text)

# Add a new rule to the security group which allows all incoming traffic from all IPv4 addresses
aws ec2 authorize-security-group-ingress --group-id "$COREKUBE_SECURITY_GROUP_ID" --protocol all --cidr "0.0.0.0/0"

# don't create nodegroup automatically, we'll set a specific size and use that
# also use the same subnet as the corekube because otherwise I had massive
# problems with trying to reach the other cluster over public internet
eksctl create cluster --name nervion-aws-cluster --region eu-north-1 --version 1.28 --without-nodegroup --node-private-networking --vpc-public-subnets "$PUBLIC_SUBNETS" --vpc-private-subnets "$PRIVATE_SUBNETS"

eksctl create nodegroup --cluster nervion-aws-cluster --name ng-nervion --node-ami-family AmazonLinux2 --node-type t3.small --nodes 2 --nodes-min 2 --nodes-max 2 --node-volume-size 20 --ssh-access --ssh-public-key "$USERPROFILE/.ssh/id_ed25519.pub" --managed --asg-access

# Verify that we can ping the frontend node from the Nervion cluster
kubectl run -i --tty --rm debug --image=alpine --restart=Never -- sh -c "apk add --no-cache iputils && ping -c 1 $FRONTEND_NODE_INTERNAL_IP"

# Get the Nervion cluster security group id
NERVION_SECURITY_GROUP_ID=$(aws ec2 describe-instances --filters "Name=tag:eks:cluster-name,Values=nervion-aws-cluster" --query "Reservations[*].Instances[*].SecurityGroups[?starts_with(GroupName, \`eks-\`)].GroupId | [0]" --output=text)

# Add a new rule to the security group which allows all incoming traffic from all IPv4 addresses
aws ec2 authorize-security-group-ingress --group-id "$NERVION_SECURITY_GROUP_ID" --protocol all --cidr "0.0.0.0/0"

# Deploy Nervion
kubectl apply -f ../nervion-powder/config/deployment.yaml
kubectl wait --for=condition=ready pod --all --timeout=10m
kubectl port-forward services/ran-emulator 3000:8080 > /dev/null 2>&1 &

# Final echos
echo "IP (within VPC) of the cluster frontend node: $FRONTEND_NODE_INTERNAL_IP"
echo "Nervion at http://localhost:3000"
echo "CoreKube Grafana at http://localhost:12345"
echo "Current cluster: nervion-aws-cluster (change with kubectl config use-context <context-name>)"

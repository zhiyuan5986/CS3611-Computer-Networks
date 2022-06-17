#!/bin/sh
sudo ovs-ofctl add-flow s2 "in_port=s2-eth2,actions=output:s2-eth3,s2-eth4"
sudo ovs-ofctl add-flow s3 "in_port=s3-eth2,actions=output:s3-eth3,s3-eth4"

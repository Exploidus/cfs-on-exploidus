#!/usr/bin/env bash
set -euo pipefail

echo "================================================================="
echo " Building NASA cFS on Exploidus OS Port & Demo                   "
echo "================================================================="

make clean
make all

echo ""
echo "================================================================="
echo " Running NASA cFE Flight Simulation on Exploidus Native OSAL     "
echo "================================================================="

./cfs_exploidus_demo

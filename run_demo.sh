#!/usr/bin/env bash
set -euo pipefail

echo "================================================================="
echo " Building NASA cFS on Exploidus OS Port & Demo                   "
echo "================================================================="

make clean
make all

echo ""
echo "================================================================="
echo " Running NASA cFE Flight Simulation (Milestone 1)                "
echo "================================================================="
./cfs_exploidus_demo

echo ""
echo "================================================================="
echo " Running NASA cFE Services (Milestone 2: TIME, TBL, SCH)         "
echo "================================================================="
./cfs_exploidus_m2

echo ""
echo "================================================================="
echo " Running NASA Flight Stack (Milestone 3: CI, TO, HK, CS, SEU)   "
echo "================================================================="
./cfs_exploidus_m3

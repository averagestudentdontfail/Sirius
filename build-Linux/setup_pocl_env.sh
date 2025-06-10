#!/bin/bash
# POCL Environment Setup Script
export OPENCL_VENDOR_PATH="/home/send2/.projects/Sirius/build-Linux/etc/OpenCL/vendors"
export LD_LIBRARY_PATH="/home/send2/.projects/Sirius/build-Linux/pocl-install/lib:$LD_LIBRARY_PATH"
export POCL_DEVICES=cpu
echo "✅ POCL environment configured!"
echo "📍 OpenCL Vendor Path: $OPENCL_VENDOR_PATH"
echo "📚 Library Path: /home/send2/.projects/Sirius/build-Linux/pocl-install/lib"
echo "🎯 Ready to run: ./Sirius"

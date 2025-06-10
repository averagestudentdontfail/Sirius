
#ifndef POCL_CONFIG_H
#define POCL_CONFIG_H


/* #undef BUILD_HSA */
/* #undef BUILD_CUDA */
#define BUILD_BASIC
/* #undef BUILD_TBB */
#define BUILD_PTHREAD
/* #undef BUILD_ALMAIF */
/* #undef BUILD_VULKAN */
/* #undef BUILD_LEVEL0 */
/* #undef BUILD_REMOTE_SERVER */
/* #undef BUILD_REMOTE_CLIENT */
/* #undef BUILD_PROXY */

#define BUILDDIR "/home/send2/.projects/Sirius/build-linux/pocl-build"

/* "Build with ICD" */
/* #undef BUILD_ICD */

#define CMAKE_BUILD_TYPE "Release"

/* #undef DEVELOPER_MODE */

/* #undef HAVE_CLSPV */
#define CLSPV ""
#define CLSPV_REFLECTION ""

/* #undef ENABLE_ASAN */
/* #undef ENABLE_LSAN */
/* #undef ENABLE_TSAN */
/* #undef ENABLE_UBSAN */

/* #undef ENABLE_EXTRA_VALIDITY_CHECKS */

/* #undef ENABLE_CONFORMANCE */

/* #undef ENABLE_RDMA */

#define ENABLE_HWLOC

#define ENABLE_HOST_CPU_DEVICES

#define ENABLE_POCL_BUILDING

/* #undef ENABLE_PTHREAD_FINISH_FN */

/* #undef ENABLE_LLVM_PLATFORM_SUPPORT */

#define ENABLE_PRINTF_IMMEDIATE_FLUSH

#define ENABLE_SIGFPE_HANDLER

#define ENABLE_SIGUSR2_HANDLER

/* #undef ENABLE_REMOTE_DISCOVERY_AVAHI */

/* #undef ENABLE_REMOTE_DISCOVERY_DHT */

/* #undef ENABLE_REMOTE_DISCOVERY_ANDROID */

/* #undef ENABLE_REMOTE_ADVERTISEMENT_AVAHI */

/* #undef ENABLE_REMOTE_ADVERTISEMENT_DHT */

#define ENABLE_RELOCATION

/* #undef ENABLE_EGL_INTEROP */
/* #undef ENABLE_OPENGL_INTEROP */

#ifdef ENABLE_OPENGL_INTEROP
/* #undef ENABLE_CL_GET_GL_CONTEXT */
#endif

#define ENABLE_SLEEF

/* #undef ENABLE_SPIRV */

/* #undef ENABLE_VALGRIND */

#define HAVE_DLFCN_H

#define HAVE_FORK

#define HAVE_VFORK

#define HAVE_LINUX_VSOCK_H

#define HAVE_CLOCK_GETTIME

#define HOST_COMPILER_SUPPORTS_FLOAT16

#define HAVE_FDATASYNC

#define HAVE_FSYNC

#define HAVE_GETRLIMIT

#define HAVE_MKOSTEMPS

#define HAVE_MKSTEMPS

#define HAVE_MKDTEMP

#define HAVE_FUTIMENS

/* #undef HAVE_LTTNG_UST */

/* #undef HAVE_LIBXSMM */

/* #undef HAVE_LIBJPEG_TURBO */

/* #undef HAVE_ONNXRT */

/* #undef HAVE_OPENCV */

#define HAVE_TREE_SITTER 0

/* #undef HAVE_OCL_ICD */
#define HAVE_OCL_ICD_30_COMPATIBLE

#define HAVE_POSIX_MEMALIGN

#define HAVE_SLEEP

#define HAVE_UTIME

/* #undef HAVE_XRT */

#define ENABLE_LLVM

#define ENABLE_LOADABLE_DRIVERS

/* this is used all over the runtime code */
#define HOST_CPU_CACHELINE_SIZE 64

#if defined(BUILD_CUDA)

#define CUDA_DEVICE_EXTENSIONS ""

#define CUDA_DEVICE_FEATURES_30 ""

#endif

#if defined(ENABLE_HOST_CPU_DEVICES)

/* #undef ENABLE_HOST_CPU_DEVICES_OPENMP */

#define HOST_AS_FLAGS  " "

#define HOST_CLANG_FLAGS  "--target=x86_64-pc-linux-gnu "

#define HOST_DEVICE_EXTENSIONS "cl_khr_byte_addressable_store cl_khr_global_int32_base_atomics cl_khr_global_int32_extended_atomics cl_khr_local_int32_base_atomics cl_khr_local_int32_extended_atomics cl_khr_3d_image_writes cl_ext_float_atomics cl_intel_unified_shared_memory cl_ext_buffer_device_address       cl_pocl_svm_rect cl_pocl_command_buffer_svm       cl_khr_command_buffer cl_khr_command_buffer_multi_device cl_khr_command_buffer_mutable_dispatch       cl_pocl_command_buffer_host_buffer cl_khr_subgroups cl_khr_subgroup_ballot cl_khr_subgroup_shuffle cl_intel_subgroups cl_intel_subgroups_short cl_intel_subgroups_char cl_intel_required_subgroup_size cl_khr_fp64 cl_khr_int64_base_atomics cl_khr_int64_extended_atomics"

#define HOST_DEVICE_FEATURES_30 "__opencl_c_3d_image_writes  __opencl_c_images __opencl_c_atomic_order_acq_rel __opencl_c_atomic_order_seq_cst __opencl_c_atomic_scope_device __opencl_c_atomic_scope_all_devices __opencl_c_generic_address_space __opencl_c_work_group_collective_functions __opencl_c_subgroups __opencl_c_read_write_images __opencl_c_program_scope_global_variables __opencl_c_subgroups __opencl_c_fp64 __opencl_c_ext_fp32_global_atomic_add __opencl_c_ext_fp32_local_atomic_add __opencl_c_ext_fp32_global_atomic_min_max __opencl_c_ext_fp32_local_atomic_min_max __opencl_c_ext_fp64_global_atomic_add __opencl_c_ext_fp64_local_atomic_add __opencl_c_ext_fp64_global_atomic_min_max __opencl_c_ext_fp64_local_atomic_min_max __opencl_c_int64"

/* #undef HOST_CPU_FORCED */

#define HOST_CPU_ENABLE_DENORMS

/* #undef HOST_CPU_ENABLE_STACK_SIZE_CHECK */

/* #undef HOST_CPU_ENABLE_SPIRV */

/* #undef ENABLE_HOST_CPU_VECTORIZE_BUILTINS */

/* #undef ENABLE_HOST_CPU_VECTORIZE_LIBMVEC */

/* #undef ENABLE_HOST_CPU_VECTORIZE_SLEEF */

/* #undef ENABLE_HOST_CPU_VECTORIZE_SVML */

#define HOST_LD_FLAGS  "-shared -nostartfiles "

#define HOST_LLC_FLAGS  "-relocation-model=pic -mtriple=x86_64-pc-linux-gnu "

#define HOST_CPU_TARGET_ABI ""

#endif

#define HOST_DEVICE_BUILD_HASH "x86_64-pc-linux-gnu"

#define DEFAULT_DEVICE_EXTENSIONS ""

#ifdef BUILD_HSA

/* #undef HAVE_HSA_EXT_AMD_H */

#define AMD_HSA 

#define HSA_DEVICE_EXTENSIONS ""

#define HSAIL_ASM ""

#define HSAIL_ENABLED 

#endif


#define CMAKE_BUILD_TYPE "Release"



#ifdef BUILD_LEVEL0

/* #undef ENABLE_NPU */

/* #undef ENABLE_LEVEL0_EXTRA_FEATURES */

/* #undef ENABLE_HEADER_BUNDLING */

#endif



#ifdef ENABLE_LLVM

#define KERNELLIB_HOST_CPU_VARIANTS "native"

/* #undef KERNELLIB_HOST_DISTRO_VARIANTS */

#define CLANGCC "/usr/lib/llvm-16/bin/clang"

#define CLANG_RESOURCE_DIR "/usr/lib/llvm-16/lib/clang/16"

#define CLANGXX "/usr/lib/llvm-16/bin/clang++"

#define CLANG_MARCH_FLAG "-march="

#define LLVM_LLC "/usr/lib/llvm-16/bin/llc"

/* #undef HAVE_LLVM_SPIRV */
#define LLVM_SPIRV "LLVM_SPIRV-NOTFOUND"

#define HAVE_LLVM_OPT
#define LLVM_OPT "/usr/lib/llvm-16/bin/opt"

/* #undef HAVE_LLVM_SPIRV_LIB */
#define LLVM_SPIRV_LIB_MAXVER 66048

#define LLVM_LINK "/usr/lib/llvm-16/bin/llvm-link"

/* #undef HAVE_SPIRV_LINK */
#define SPIRV_LINK "SPIRV_LINK-NOTFOUND"

/* #undef SPIRV_LINK_HAS_USE_HIGHEST_VERSION */

#define LLVM_MAJOR 16

/* #undef LLVM_BUILD_MODE_DEBUG */

#ifndef LLVM_VERSION
#define LLVM_VERSION "16.0.6"
#endif

/* #undef USE_LLVM_SPIRV_TARGET */

#define LLVM_VERIFY_MODULE_DEFAULT 0

#define LLVM_OPAQUE_POINTERS

#endif



#define PRINTF_BUFFER_SIZE 16384

/* used in lib/CL/devices/basic */
#define OCL_KERNEL_TARGET  "x86_64-pc-linux-gnu"
#define OCL_KERNEL_TARGET_CPU  "znver4"

#define POCL_KERNEL_CACHE_DEFAULT 1

#define HOST_DEVICE_ADDRESS_BITS 64

#define POCL_DEBUG_MESSAGES

#define POCL_INSTALL_PRIVATE_HEADER_DIR "/home/send2/.projects/Sirius/build-linux/pocl-install/share/pocl/include"

#define POCL_INSTALL_PRIVATE_DATADIR "/home/send2/.projects/Sirius/build-linux/pocl-install/share/pocl"

#define POCL_INSTALL_FROM_LIB_TO_PRIVATE_DATADIR "../share/pocl"

#define POCL_INSTALL_PRIVATE_LIBDIR "/home/send2/.projects/Sirius/build-linux/pocl-install/lib/pocl"

#define POCL_INSTALL_LIBDIR "/home/send2/.projects/Sirius/build-linux/pocl-install/lib"

#define POCL_INSTALL_FROM_LIB_TO_PRIVATE_LIBDIR "pocl"

/* #undef POCL_ASSERTS_BUILD */

/* these are *host* values */

/* used in tce_common.c & pocl_llvm_api.cc  */
#define SRCDIR  "/home/send2/.projects/Sirius/deps/pocl"

/* #undef TCEMC_AVAILABLE */

/* #undef TCE_AVAILABLE */

#define TCE_DEVICE_EXTENSIONS ""

#define OACC_EXECUTABLE "TCECC-NOTFOUND"

/* Defined on big endian systems */
#define WORDS_BIGENDIAN 0

/* platform version */
#define POCL_PLATFORM_VERSION_MAJOR 3
#define POCL_PLATFORM_VERSION_MINOR 0
#define POCL_PLATFORM_VERSION_PATCH 0

#define HSA_DEVICE_CL_VERSION_MAJOR 1
#define HSA_DEVICE_CL_VERSION_MINOR 2

#define CUDA_DEVICE_CL_VERSION_MAJOR 
#define CUDA_DEVICE_CL_VERSION_MINOR 

#define HOST_DEVICE_CL_VERSION_MAJOR 3
#define HOST_DEVICE_CL_VERSION_MINOR 0

#define TCE_DEVICE_CL_VERSION_MAJOR 1
#define TCE_DEVICE_CL_VERSION_MINOR 2


/* #undef USE_POCL_MEMMANAGER */

/* #undef RENAME_POCL */

/* #undef KERNEL_TRIPLE_TARGETS_MSVC_TOOLCHAIN */

#define CMAKE_COMMAND "/usr/bin/cmake"

#endif

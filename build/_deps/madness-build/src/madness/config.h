/* Input file cmake/config.h.in. */

/* Define of package information */
#define MADNESS_PACKAGE_NAME "MADNESS"
#define MADNESS_PACKAGE_URL "https://github.com/m-a-d-n-e-s-s/madness"
#define MADNESS_PACKAGE_VERSION "0.10.1"
#define MADNESS_VERSION "0.10.1"
#define MADNESS_MAJOR_VERSION 0
#define MADNESS_MINOR_VERSION 0
#define MADNESS_MICRO_VERSION 0
#define MADNESS_REVISION "e8bf851c6c830ec1287d1273e35db2cb2b94771c"

/* Configured information */
#define MADNESS_CONFIGURATION_CXX "/usr/bin/g++"
#define MADNESS_CONFIGURATION_CXXFLAGS "-std=c++17"
#define HOST_SYSTEM "Linux"
#define MADNESS_CONFIGURATION_DATE "2021-08-19T01:39:14"
#define MADNESS_CONFIGURATION_HOST "dolphin"
#define MADNESS_CONFIGURATION_USER "joseph"

/* Target for tuning mtxmq kernels */
/* #undef AMD_QUADCORE_TUNE */

/* Fortran-C linking convention type and integer size */
/* #undef FORTRAN_LINKAGE_LC */
/* #undef FORTRAN_LINKAGE_LCU */
/* #undef FORTRAN_LINKAGE_LCUU */
/* #undef FORTRAN_LINKAGE_UC */
/* #undef FORTRAN_LINKAGE_UCU */
#define MADNESS_FORTRAN_DEFAULT_INTEGER_SIZE 4

/* Defined system specific macros */
/* #undef HAVE_CRAYXE */
/* #undef HAVE_CRAYXT */
/* #undef HAVE_IBMBGP */
/* #undef HAVE_IBMBGQ */
/* #undef ON_A_MAC */
#define MADNESS_CXX_ABI_GenericItanium 1
#define MADNESS_CXX_ABI_GenericARM 2
#define MADNESS_CXX_ABI_Microsoft 3
#define MADNESS_CXX_ABI 1

/* Define type macros. */
#define HAVE_INT64_T 1
#define HAVE_LONG_LONG 1
/* Define to `int' if <sys/types.h> does not define. */
#define SYS_TYPES_H_HAS_PID_T 1
#if !defined(SYS_TYPES_H_HAS_PID_T)
# define pid_t int
#endif

/* Define MADNESS has access to the library. */
/* #undef HAVE_ACML */
/* #undef HAVE_INTEL_TBB */
/* #undef MADNESS_CAN_USE_TBB_PRIORITY */
#define HAVE_PARSEC 1
/* #undef HAVE_INTEL_MKL */
/* #undef HAVE_PAPI */
/* #undef MADNESS_HAS_PCM */
/* #undef MADNESS_HAS_LIBXC */
/* #undef MADNESS_HAS_BOOST */
/* #undef MADNESS_HAS_ELEMENTAL */
/* #undef MADNESS_HAS_ELEMENTAL_EMBEDDED */
/* #undef MADNESS_HAS_GOOGLE_PERF_MINIMAL */
#define MADNESS_HAS_GOOGLE_TEST 1
#define MADNESS_HAS_LIBUNWIND 1

/* Define has access to function. */
/* #undef HAVE_FORK */
#define HAVE_MEMSET 1
#define HAVE_POSIX_MEMALIGN 1
/* #undef MISSING_POSIX_MEMALIGN_PROTO */
/* #undef HAVE_POW */
#define HAVE_RANDOM 1
#define HAVE_SLEEP 1
#define HAVE_STD_ABS_LONG 1
#define HAVE_STRCHR 1

/* Define if header files are available. */
#ifndef HAVE_SYS_STAT_H
#define HAVE_SYS_STAT_H 1
#endif
#ifndef HAVE_SYS_TYPES_H
#define HAVE_SYS_TYPES_H
#endif
#ifndef HAVE_UNISTD_H
#define HAVE_UNISTD_H 1
#endif
/* #undef HAVE_ELEMENTAL_H */
/* #undef HAVE_EL_H */

/* Set if compiler will instantiate static templates */
#define HAVE_UNQUALIFIED_STATIC_DECL 1

/* Set MADNESS assertions behavior */
/* #undef MADNESS_ASSERTIONS_ABORT */
/* #undef MADNESS_ASSERTIONS_ASSERT */
#define MADNESS_ASSERTIONS_DISABLE 1
/* #undef MADNESS_ASSERTIONS_THROW */

/* Thread-safety level requested from MPI by MADNESS */
#define MADNESS_MPI_THREAD_LEVEL MPI_THREAD_MULTIPLE
/* #undef STUBOUTMPI */
#ifndef MADNESS_MPI_HEADER
# define MADNESS_MPI_HEADER "/usr/local/include/mpi.h"
#endif

/* The default binding for threads */
#define MAD_BIND_DEFAULT "-1 -1 -1"

/* Define to enable MADNESS features */
/* #undef MADNESS_TASK_PROFILING */
#define MADNESS_USE_BSEND_ACKS 1
/* #undef NEVER_SPIN */
/* #undef TENSOR_BOUNDS_CHECKING */
/* #undef TENSOR_INSTANCE_COUNT */
#define USE_SPINLOCKS 1
/* #undef WORLD_GATHER_MEM_STATS */
/* #undef WORLD_MEM_PROFILE_ENABLE */
/* #undef WORLD_PROFILE_ENABLE */
/* #undef MADNESS_TASK_DEBUG_TRACE */
/* #undef MADNESS_LINALG_USE_LAPACKE */
#define MADNESS_DQ_USE_PREBUF 1
#define MADNESS_DQ_PREBUF_SIZE 20
/* #undef MADNESS_ASSUMES_ASLR_DISABLED */

/* Define to the equivalent of the C99 'restrict' keyword, or to
   nothing if this is not supported.  Do not define if restrict is
   supported directly.  */
#define MADNESS_RESTRICT __restrict
/* Work around a bug in Sun C++: it does not support _Restrict or
   __restrict__, even though the corresponding Sun C compiler ends up with
   "#define restrict _Restrict" or "#define restrict __restrict__" in the
   previous line.  Perhaps some future version of Sun C++ will work with
   restrict; if so, hopefully it defines __RESTRICT like Sun C does.  */
#if defined __SUNPRO_CC && !defined __RESTRICT
# define _Restrict
# define __restrict__
#endif


/* Define the thread_local key word. */
/* #undef THREAD_LOCAL_KEYWORD */
#if defined(THREAD_LOCAL_KEYWORD)
# define thread_local THREAD_LOCAL_KEYWORD
#endif

/* Define to the application path if available */
#define HAVE_XTERM 1
#define XTERM_EXECUTABLE "/usr/bin/xterm"
#define HAVE_GDB 1
#define GDB_EXECUTABLE "/usr/bin/gdb"

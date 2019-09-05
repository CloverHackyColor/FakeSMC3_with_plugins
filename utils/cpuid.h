/*
 *  cpuid.h
 *
 */

#ifndef M_CPUID_H
#define M_CPUID_H

#include <machine/machine_routines.h>
#include <pexpert/pexpert.h>
#include <i386/proc_reg.h>
#include <string.h>

#include <stdint.h>
#include <mach/mach_types.h>
#include <kern/kern_types.h>
#include <mach/machine.h>



#define  CPUID_VID_INTEL    "GenuineIntel"
#define  CPUID_VID_AMD    "AuthenticAMD"
#define CPU_VENDOR_INTEL  0x756E6547
#define CPU_VENDOR_AMD    0x68747541


#define CPUID_STRING_UNKNOWN    "Unknown CPU Typ"

/* Known MSR registers */ //exclude redefined but remain as comments
//#define MSR_IA32_PLATFORM_ID        0x0017
//#define MSR_CORE_THREAD_COUNT       0x0035   /* limited use - not for Penryn or older  */
//#define MSR_IA32_BIOS_SIGN_ID       0x008B   /* microcode version */
#define MSR_FSB_FREQ                  0x00CD   /* limited use - not for i7            */
//#define  MSR_PLATFORM_INFO           0x00CE   /* limited use - MinRatio for i7 but Max for Yonah  */
/* turbo for penryn */
#define MSR_IA32_EXT_CONFIG           0x00EE   /* limited use - not for i7            */
//#define MSR_FLEX_RATIO              0x0194   /* limited use - not for Penryn or older      */
//see no value on most CPUs
//#define  MSR_IA32_PERF_STATUS        0x0198
#define MSR_IA32_PERF_CONTROL         0x0199
//#define MSR_IA32_CLOCK_MODULATION   0x019A
#define MSR_THERMAL_STATUS            0x019C
//#define MSR_IA32_MISC_ENABLE        0x01A0
#define MSR_THERMAL_TARGET            0x01A2   /* limited use - not for Penryn or older      */
#define MSR_TURBO_RATIO_LIMIT         0x01AD   /* limited use - not for Penryn or older      */

//AMD
#define K8_FIDVID_STATUS              0xC0010042
#define K10_COFVID_STATUS             0xC0010071
#define DEFAULT_FSB                   100000    /* for now, hardcoding 100MHz for old CPUs */


#define _Bit(n)      (1ULL << n)
#define _HBit(n)    (1ULL << ((n)+32))

/*
 * The CPUID_FEATURE_XXX values define 64-bit values
 * returned in %ecx:%edx to a CPUID request with %eax of 1:
 */
#define  CPUID_FEATURE_FPU    _Bit(0)  /* Floating point unit on-chip */
#define  CPUID_FEATURE_VME    _Bit(1)  /* Virtual Mode Extension */
#define  CPUID_FEATURE_DE     _Bit(2)  /* Debugging Extension */
#define  CPUID_FEATURE_PSE    _Bit(3)  /* Page Size Extension */
#define  CPUID_FEATURE_TSC    _Bit(4)  /* Time Stamp Counter */
#define  CPUID_FEATURE_MSR    _Bit(5)  /* Model Specific Registers */
#define CPUID_FEATURE_PAE     _Bit(6)  /* Physical Address Extension */
#define  CPUID_FEATURE_MCE    _Bit(7)  /* Machine Check Exception */
#define  CPUID_FEATURE_CX8    _Bit(8)  /* CMPXCHG8B */
#define  CPUID_FEATURE_APIC   _Bit(9)  /* On-chip APIC */
#define CPUID_FEATURE_SEP     _Bit(11)  /* Fast System Call */
#define  CPUID_FEATURE_MTRR   _Bit(12)  /* Memory Type Range Register */
#define  CPUID_FEATURE_PGE    _Bit(13)  /* Page Global Enable */
#define  CPUID_FEATURE_MCA    _Bit(14)  /* Machine Check Architecture */
#define  CPUID_FEATURE_CMOV   _Bit(15)  /* Conditional Move Instruction */
#define CPUID_FEATURE_PAT     _Bit(16)  /* Page Attribute Table */
#define CPUID_FEATURE_PSE36   _Bit(17)  /* 36-bit Page Size Extension */
#define CPUID_FEATURE_PSN     _Bit(18)  /* Processor Serial Number */
#define CPUID_FEATURE_CLFSH   _Bit(19)  /* CLFLUSH Instruction supported */
#define CPUID_FEATURE_DS      _Bit(21)  /* Debug Store */
#define CPUID_FEATURE_ACPI    _Bit(22)  /* Thermal monitor and Clock Ctrl */
#define CPUID_FEATURE_MMX     _Bit(23)  /* MMX supported */
#define CPUID_FEATURE_FXSR    _Bit(24)  /* Fast floating pt save/restore */
#define CPUID_FEATURE_SSE     _Bit(25)  /* Streaming SIMD extensions */
#define CPUID_FEATURE_SSE2    _Bit(26)  /* Streaming SIMD extensions 2 */
#define CPUID_FEATURE_SS      _Bit(27)  /* Self-Snoop */
#define CPUID_FEATURE_HTT     _Bit(28)  /* Hyper-Threading Technology */
#define CPUID_FEATURE_TM      _Bit(29)  /* Thermal Monitor (TM1) */
#define CPUID_FEATURE_PBE     _Bit(31)  /* Pend Break Enable */

#define CPUID_FEATURE_SSE3    _HBit(0)  /* Streaming SIMD extensions 3 */
#define CPUID_FEATURE_PCLMULQDQ _HBit(1) /* PCLMULQDQ Instruction */

#define CPUID_FEATURE_MONITOR _HBit(3)  /* Monitor/mwait */
#define CPUID_FEATURE_DSCPL   _HBit(4)  /* Debug Store CPL */
#define CPUID_FEATURE_VMX     _HBit(5)  /* VMX */
#define CPUID_FEATURE_SMX     _HBit(6)  /* SMX */
#define CPUID_FEATURE_EST     _HBit(7)  /* Enhanced SpeedsTep (GV3) */
#define CPUID_FEATURE_TM2     _HBit(8)  /* Thermal Monitor 2 */
#define CPUID_FEATURE_SSSE3   _HBit(9)  /* Supplemental SSE3 instructions */
#define CPUID_FEATURE_CID     _HBit(10)  /* L1 Context ID */

#define CPUID_FEATURE_CX16    _HBit(13)  /* CmpXchg16b instruction */
#define CPUID_FEATURE_xTPR    _HBit(14)  /* Send Task PRiority msgs */
#define CPUID_FEATURE_PDCM    _HBit(15)  /* Perf/Debug Capability MSR */

#define CPUID_FEATURE_DCA     _HBit(18)  /* Direct Cache Access */
#define CPUID_FEATURE_SSE4_1  _HBit(19)  /* Streaming SIMD extensions 4.1 */
#define CPUID_FEATURE_SSE4_2  _HBit(20)  /* Streaming SIMD extensions 4.2 */
#define CPUID_FEATURE_xAPIC   _HBit(21)  /* Extended APIC Mode */
#define CPUID_FEATURE_POPCNT  _HBit(23)  /* POPCNT instruction */
#define CPUID_FEATURE_AES     _HBit(25)  /* AES instructions */
#define CPUID_FEATURE_VMM     _HBit(31)  /* VMM (Hypervisor) present */

/*
 * The CPUID_EXTFEATURE_XXX values define 64-bit values
 * returned in %ecx:%edx to a CPUID request with %eax of 0x80000001:
 */
#define CPUID_EXTFEATURE_SYSCALL  _Bit(11)  /* SYSCALL/sysret */
#define CPUID_EXTFEATURE_XD       _Bit(20)  /* eXecute Disable */
#define CPUID_EXTFEATURE_1GBPAGE  _Bit(26)     /* 1G-Byte Page support */
#define CPUID_EXTFEATURE_RDTSCP   _Bit(27)  /* RDTSCP */
#define CPUID_EXTFEATURE_EM64T    _Bit(29)  /* Extended Mem 64 Technology */

//#define CPUID_EXTFEATURE_LAHF    _HBit(20)  /* LAFH/SAHF instructions */
// New definition with Snow kernel
#define CPUID_EXTFEATURE_LAHF     _HBit(0)  /* LAHF/SAHF instructions */
/*
 * The CPUID_EXTFEATURE_XXX values define 64-bit values
 * returned in %ecx:%edx to a CPUID request with %eax of 0x80000007:
 */
#define CPUID_EXTFEATURE_TSCI     _Bit(8)  /* TSC Invariant */

#define  CPUID_CACHE_SIZE  16  /* Number of descriptor values */

#define CPUID_MWAIT_EXTENSION     _Bit(0)  /* enumeration of WMAIT extensions */
#define CPUID_MWAIT_BREAK         _Bit(1)  /* interrupts are break events     */

#define CPU_MODEL_PENTIUM_M     0x0D
#define CPU_MODEL_YONAH         0x0E
#define CPU_MODEL_MEROM         0x0F
#define CPU_MODEL_CELERON       0x16
#define CPU_MODEL_PENRYN        0x17
#define CPU_MODEL_NEHALEM       0x1A
#define CPU_MODEL_ATOM          0x1C
#define CPU_MODEL_XEON_MP       0x1D  /* MP 7400 */
#define CPU_MODEL_FIELDS        0x1E  /* Lynnfield, Clarksfield, Jasper */
#define CPU_MODEL_DALES         0x1F  /* Havendale, Auburndale */
#define CPU_MODEL_DALES_32NM    0x25  /* Clarkdale, Arrandale */
#define CPU_MODEL_ATOM_SAN      0x26
#define CPU_MODEL_LINCROFT      0x27
#define CPU_MODEL_SANDY_BRIDGE  0x2A
#define CPU_MODEL_WESTMERE      0x2C  /* Gulftown, Westmere-EP, Westmere-WS */
#define CPU_MODEL_JAKETOWN      0x2D
#define CPU_MODEL_NEHALEM_EX    0x2E
#define CPU_MODEL_WESTMERE_EX   0x2F
#define CPU_MODEL_ATOM_2000     0x36
#define CPU_MODEL_ATOM_3700     0x37  /* Bay Trail */
#define CPU_MODEL_IVY_BRIDGE    0x3A
#define CPU_MODEL_HASWELL       0x3C
#define CPU_MODEL_HASWELL_U5    0x3D  /* Haswell U5  5th generation Broadwell*/
#define CPU_MODEL_IVY_BRIDGE_E5 0x3E  /* Ivy Bridge Xeon */
#define CPU_MODEL_HASWELL_MB    0x3F  /* Haswell MB */
//#define CPU_MODEL_HASWELL_H    0x??  // Haswell H
#define CPU_MODEL_HASWELL_ULT    0x45  /* Haswell ULT */
#define CPU_MODEL_HASWELL_ULX    0x46  /* Haswell ULX */
#define CPU_MODEL_BROADWELL_HQ  0x47
#define CPU_MODEL_AIRMONT       0x4C
#define CPU_MODEL_AVOTON        0x4D
#define CPU_MODEL_SKYLAKE_U     0x4E
#define CPU_MODEL_BROADWELL_E5  0x4F
#define CPU_MODEL_SKYLAKE_X     0x55  /* Intel® Core™ X-series Processors */
#define CPU_MODEL_BROADWELL_DE  0x56
#define CPU_MODEL_KNIGHT        0x57
#define CPU_MODEL_MOOREFIELD    0x5A
#define CPU_MODEL_GOLDMONT      0x5C
#define CPU_MODEL_ATOM_X3       0x5D
#define CPU_MODEL_SKYLAKE_D     0x5E /* Skylake Desktop */
#define CPU_MODEL_CANNONLAKE    0x66
#define CPU_MODEL_XEON_MILL     0x85  /* Knights Mill */
#define CPU_MODEL_KABYLAKE1     0x8E /* Kabylake Mobile */
#define CPU_MODEL_KABYLAKE2     0x9E /* Kabylake Dektop */



typedef enum { eax, ebx, ecx, edx } cpuid_register_t;
static inline void
cpuid(uint32_t *data) {
  __asm__ volatile("cpuid"
                   : "=a" (data[eax]),
                   "=b" (data[ebx]),
                   "=c" (data[ecx]),
                   "=d" (data[edx])
                   : "a" (data[eax]),
                   "b"  (data[ebx]),
                   "c"  (data[ecx]),
                   "d"  (data[edx]));
}

static inline void
do_cpuid(uint32_t selector, uint32_t *data) {
  __asm__ volatile("cpuid"
                   : "=a" (data[0]),
                   "=b" (data[1]),
                   "=c" (data[2]),
                   "=d" (data[3])
                   : "a"(selector));
}

/*
 * Cache ID descriptor structure, used to parse CPUID leaf 2.
 * Note: not used in kernel.
 */
typedef enum { Lnone, L1I, L1D, L2U, L3U, LCACHE_MAX } cache_type_t ;
typedef struct {
  unsigned char  value;       /* Descriptor value */
  cache_type_t   type;        /* Cache type */
  unsigned int   size;        /* Cache size */
  unsigned int   linesize;    /* Cache line size */
  const char    *description; /* Cache description */
} cpuid_cache_desc_t;


#define CACHE_DESC(value,type,size,linesize,text) \
{ value, type, size, linesize, text }

#define _Bit(n)         (1ULL << n)
//#define _HBit(n)        (1ULL << ((n) + 32))

#define min(a,b)        ((a) < (b) ? (a) : (b))
#define quad(hi,lo)     (((uint64_t)(hi)) << 32 | (lo))
#define bit(n)          (1UL << (n))
#define bitmask(h,l)    ((bit(h) | (bit(h) - 1UL)) & ~(bit(l) - 1UL))
#define bitfield(x,h,l) (((x) & bitmask(h, l)) >> l)

/* Physical CPU info - this is exported out of the kernel (kexts), so be wary of changes */
typedef struct {
  char        cpuid_vendor[16];
  char        cpuid_brand_string[48];
  const char  *cpuid_model_string;
  
  cpu_type_t  cpuid_type;          /* this is *not* a cpu_type_t in our <mach/machine.h> */
  uint8_t     cpuid_family;
  uint8_t     cpuid_model;
  uint8_t     cpuid_extmodel;
  uint8_t     cpuid_extfamily;
  uint8_t     cpuid_stepping;
  uint64_t    cpuid_features;
  uint64_t    cpuid_extfeatures;
  uint32_t    cpuid_signature;
  uint8_t     cpuid_brand;
  uint8_t     cpuid_processor_flag;
  
  uint32_t    cache_size[LCACHE_MAX];
  uint32_t    cache_linesize;
  
  uint8_t     cache_info[64];    /* list of cache descriptors */
  
  uint32_t    cpuid_cores_per_package;
  uint32_t    cpuid_logical_per_package;
  uint32_t    cache_sharing[LCACHE_MAX];
  uint32_t    cache_partitions[LCACHE_MAX];
  
  cpu_type_t  cpuid_cpu_type;      /* <mach/machine.h> */
  cpu_subtype_t  cpuid_cpu_subtype;    /* <mach/machine.h> */
  
  /* Monitor/mwait Leaf: */
  uint32_t  cpuid_mwait_linesize_min;
  uint32_t  cpuid_mwait_linesize_max;
  uint32_t  cpuid_mwait_extensions;
  uint32_t  cpuid_mwait_sub_Cstates;
  
  /* Thermal and Power Management Leaf: */
  boolean_t cpuid_thermal_sensor;
  boolean_t cpuid_thermal_dynamic_acceleration;
  uint32_t  cpuid_thermal_thresholds;
  boolean_t cpuid_thermal_ACNT_MCNT;
  
  /* Architectural Performance Monitoring Leaf: */
  uint8_t   cpuid_arch_perf_version;
  uint8_t   cpuid_arch_perf_number;
  uint8_t   cpuid_arch_perf_width;
  uint8_t   cpuid_arch_perf_events_number;
  uint32_t  cpuid_arch_perf_events;
  uint8_t   cpuid_arch_perf_fixed_number;
  uint8_t   cpuid_arch_perf_fixed_width;
  
  /* Cache details: */
  uint32_t  cpuid_cache_linesize;
  uint32_t  cpuid_cache_L2_associativity;
  uint32_t  cpuid_cache_size;
  
  /* Virtual and physical address aize: */
  uint32_t  cpuid_address_bits_physical;
  uint32_t  cpuid_address_bits_virtual;
  uint32_t  cpuid_microcode_version;
  
  /* Numbers of tlbs per processor [i|d, small|large, level0|level1] */
  uint32_t  cpuid_tlb[2][2][2];
#define  TLB_INST  0
#define  TLB_DATA  1
#define  TLB_SMALL  0
#define  TLB_LARGE  1
  uint32_t  cpuid_stlb;
  
  uint32_t  core_count;
  uint32_t  thread_count;
  
  /* Max leaf ids available from CPUID */
  uint32_t  cpuid_max_basic;
  uint32_t  cpuid_max_ext;
} i386_cpu_info_t;

#ifdef __cplusplus
extern "C" {
#endif
  
  i386_cpu_info_t  *cpuid_info(void);
  
  extern void cpuid_update_generic_info(void);
#ifdef __cplusplus
};
#endif
/*
i386_cpu_info_t	*cpuid_info(void);

void cpuid_update_generic_info();
*/
  
#endif /*M_CPUID_H*/

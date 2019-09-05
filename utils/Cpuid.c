//
//  Cpuid.c
//  HWSensors
//
//  Created by Sergey on 20.03.13.
//  Copyright (c) 2013 slice. All rights reserved.
//

//#include <stdio.h>
#include <machine/machine_routines.h>
#include <pexpert/pexpert.h>
//#include <i386/proc_reg.h>
#include <string.h>
#include "cpuid.h"

static i386_cpu_info_t cpuid_cpu_info;

i386_cpu_info_t  *cpuid_info(void) {
  return   &cpuid_cpu_info;
}

void cpuid_update_generic_info() {
  uint32_t cpuid_reg[4];
  uint32_t max_extid;
  char     str[128];
  
  i386_cpu_info_t* info_p = cpuid_info();
  uint64_t msr;
  
  /* Get vendor */
  do_cpuid(0, cpuid_reg);
  bcopy((char *)&cpuid_reg[ebx], &info_p->cpuid_vendor[0], 4); /* ug */
  bcopy((char *)&cpuid_reg[ecx], &info_p->cpuid_vendor[8], 4);
  bcopy((char *)&cpuid_reg[edx], &info_p->cpuid_vendor[4], 4);
  info_p->cpuid_vendor[12] = 0;
  
  /* Get extended CPUID results */
  do_cpuid(0x80000000, cpuid_reg);
  max_extid = cpuid_reg[eax];
  
  /* Check to see if we can get the brand string */
  if (max_extid >= 0x80000004) {
    char*    p = NULL;
    /*
     * The brand string is up to 48 bytes and is guaranteed to be
     * NUL terminated.
     */
    do_cpuid(0x80000002, cpuid_reg);
    bcopy((char *)cpuid_reg, &str[0], 16);
    do_cpuid(0x80000003, cpuid_reg);
    bcopy((char *)cpuid_reg, &str[16], 16);
    do_cpuid(0x80000004, cpuid_reg);
    bcopy((char *)cpuid_reg, &str[32], 16);
    for (p = str; *p != '\0'; p++) {
      if (*p != ' ') break;
    }
    strncpy(info_p->cpuid_brand_string, p, sizeof(info_p->cpuid_brand_string));
    
    if (!strncmp(info_p->cpuid_brand_string, CPUID_STRING_UNKNOWN,
                 min(sizeof(info_p->cpuid_brand_string),
                     strlen(CPUID_STRING_UNKNOWN) + 1))) {
                   /*
                    * This string means we have a firmware-programmable brand string,
                    * and the firmware couldn't figure out what sort of CPU we have.
                    */
                   info_p->cpuid_brand_string[0] = '\0';
                 }
  }
  
  /* Get cache and addressing info */
  if (max_extid >= 0x80000006) {
    do_cpuid(0x80000006, cpuid_reg);
    info_p->cpuid_cache_linesize = (uint32_t)bitfield(cpuid_reg[ecx], 7, 0);
    info_p->cpuid_cache_L2_associativity = (uint32_t)bitfield(cpuid_reg[ecx], 15, 12);
    info_p->cpuid_cache_size = (uint32_t)bitfield(cpuid_reg[ecx], 31, 16);
    do_cpuid(0x80000008, cpuid_reg);
    info_p->cpuid_address_bits_physical = (uint32_t)bitfield(cpuid_reg[eax], 7, 0);
    info_p->cpuid_address_bits_virtual = (uint32_t)bitfield(cpuid_reg[eax], 15, 8);
  }
  /*
   * Get processor signature and decode
   * and bracket this with the approved procedure for reading the
   * the microcode version number a.k.a. signature a.k.a. BIOS ID
   */
  if (strcmp(info_p->cpuid_vendor, CPUID_VID_INTEL) == 0) {
    wrmsr64(MSR_IA32_BIOS_SIGN_ID, 0);
  }
  
  do_cpuid(1, cpuid_reg);
  
  if (strcmp(info_p->cpuid_vendor, CPUID_VID_INTEL) == 0) {
    info_p->cpuid_microcode_version = (uint32_t) (rdmsr64(MSR_IA32_BIOS_SIGN_ID) >> 32);
  }
  
  /* Get processor signature and decode */
  
  info_p->cpuid_signature = cpuid_reg[eax];
  info_p->cpuid_stepping  = (uint32_t)bitfield(cpuid_reg[eax],  3,  0);
  info_p->cpuid_model     = (uint32_t)bitfield(cpuid_reg[eax],  7,  4);
  info_p->cpuid_family    = (uint32_t)bitfield(cpuid_reg[eax], 11,  8);
  info_p->cpuid_type      = (uint32_t)bitfield(cpuid_reg[eax], 13, 12);
  info_p->cpuid_extmodel  = (uint32_t)bitfield(cpuid_reg[eax], 19, 16);
  info_p->cpuid_extfamily = (uint32_t)bitfield(cpuid_reg[eax], 27, 20);
  info_p->cpuid_brand     = (uint32_t)bitfield(cpuid_reg[ebx],  7,  0);
  info_p->cpuid_features  = quad(cpuid_reg[ecx], cpuid_reg[edx]);
  
  /* Get "processor flag"; necessary for microcode update matching */
  if (strcmp(info_p->cpuid_vendor, CPUID_VID_INTEL) == 0) {
    info_p->cpuid_processor_flag = (rdmsr64(MSR_IA32_PLATFORM_ID)>> 50) & 3;
  } else {
    info_p->cpuid_processor_flag = 0;
  }
  /* Fold extensions into family/model */
  if (info_p->cpuid_family == 0x0f) {
    info_p->cpuid_family += info_p->cpuid_extfamily;
  }
  
  if (info_p->cpuid_family == 0x0f || info_p->cpuid_family== 0x06) {
    info_p->cpuid_model += (info_p->cpuid_extmodel << 4);
  }
  
  if (info_p->cpuid_features & CPUID_FEATURE_HTT) {
    info_p->cpuid_logical_per_package = (uint32_t)bitfield(cpuid_reg[ebx], 23, 16);
  } else {
    info_p->cpuid_logical_per_package = 1;
  }
  
  if (max_extid >= 0x80000001) {
    do_cpuid(0x80000001, cpuid_reg);
    info_p->cpuid_extfeatures = quad(cpuid_reg[ecx], cpuid_reg[edx]);
  }
  
  /* Fold in the Invariant TSC feature bit, if present */
  if (info_p->cpuid_max_ext >= 0x80000007) {
    do_cpuid(0x80000007, cpuid_reg);
    info_p->cpuid_extfeatures |=
    cpuid_reg[edx] & (uint32_t)CPUID_EXTFEATURE_TSCI;
  }
  
  if (info_p->cpuid_extfeatures & CPUID_FEATURE_MONITOR) {
    if (info_p->cpuid_max_ext >= 5) {
      do_cpuid(5, cpuid_reg);
      info_p->cpuid_mwait_linesize_min = cpuid_reg[eax];
      info_p->cpuid_mwait_linesize_max = cpuid_reg[ebx];
      info_p->cpuid_mwait_extensions   = cpuid_reg[ecx];
      info_p->cpuid_mwait_sub_Cstates  = cpuid_reg[edx];
    }
    
    if (info_p->cpuid_max_ext >= 6) {
      do_cpuid(6, cpuid_reg);
      info_p->cpuid_thermal_sensor = bitfield((uint32_t)cpuid_reg[eax], 0, 0);
      info_p->cpuid_thermal_dynamic_acceleration =
      bitfield((uint32_t)cpuid_reg[eax], 1, 1);
      info_p->cpuid_thermal_thresholds = bitfield((uint32_t)cpuid_reg[ebx], 3, 0);
      info_p->cpuid_thermal_ACNT_MCNT = bitfield((uint32_t)cpuid_reg[ecx], 0, 0);
    }
    
    if (info_p->cpuid_max_ext >= 0xa) {
      do_cpuid(0xa, cpuid_reg);
      info_p->cpuid_arch_perf_version = (uint32_t)bitfield(cpuid_reg[eax], 7, 0);
      info_p->cpuid_arch_perf_number = (uint32_t)bitfield(cpuid_reg[eax],15, 8);
      info_p->cpuid_arch_perf_width = (uint32_t)bitfield(cpuid_reg[eax],23,16);
      info_p->cpuid_arch_perf_events_number = (uint32_t)bitfield(cpuid_reg[eax],31,24);
      info_p->cpuid_arch_perf_events = cpuid_reg[ebx];
      info_p->cpuid_arch_perf_fixed_number = (uint32_t)bitfield(cpuid_reg[edx], 4, 0);
      info_p->cpuid_arch_perf_fixed_width = (uint32_t)bitfield(cpuid_reg[edx],12, 5);
    }
  }
  
  if (strcmp(info_p->cpuid_vendor, CPUID_VID_INTEL) == 0) {
    do_cpuid(4, cpuid_reg);
    info_p->cpuid_cores_per_package = bitfield((uint32_t)cpuid_reg[eax], 31, 26) + 1;
  } else {
    do_cpuid(0x80000008, cpuid_reg);
    info_p->cpuid_cores_per_package =  (cpuid_reg[ecx] & 0xFF) + 1;
  }
  
  if (info_p->cpuid_cores_per_package == 0) {
    info_p->cpuid_cores_per_package = 1;
  }
  
  if (strcmp(info_p->cpuid_vendor, CPUID_VID_INTEL) == 0) {
    switch (info_p->cpuid_model) {
      case CPU_MODEL_NEHALEM: // Intel Core i7 LGA1366 (45nm)
      case CPU_MODEL_FIELDS: // Intel Core i5, i7 LGA1156 (45nm)
      case CPU_MODEL_DALES_32NM: // Intel Core i3, i5, i7 LGA1156 (32nm)
      case CPU_MODEL_SANDY_BRIDGE:
      case CPU_MODEL_JAKETOWN:
      case CPU_MODEL_NEHALEM_EX:
      case CPU_MODEL_IVY_BRIDGE:
      case CPU_MODEL_HASWELL:
      case CPU_MODEL_HASWELL_U5:
      case CPU_MODEL_HASWELL_MB:
      case CPU_MODEL_HASWELL_ULT:
      case CPU_MODEL_HASWELL_ULX:
      case CPU_MODEL_IVY_BRIDGE_E5:
      case CPU_MODEL_BROADWELL_HQ:
      case CPU_MODEL_AIRMONT:
      case CPU_MODEL_AVOTON:
      case CPU_MODEL_SKYLAKE_U:
      case CPU_MODEL_BROADWELL_E5:
      case CPU_MODEL_BROADWELL_DE:
      case CPU_MODEL_KNIGHT:
      case CPU_MODEL_MOOREFIELD:
      case CPU_MODEL_GOLDMONT:
      case CPU_MODEL_ATOM_X3:
      case CPU_MODEL_SKYLAKE_D:
      case CPU_MODEL_SKYLAKE_X:
      case CPU_MODEL_CANNONLAKE:
      case CPU_MODEL_XEON_MILL:
      case CPU_MODEL_KABYLAKE1:
      case CPU_MODEL_KABYLAKE2:
        
        msr = rdmsr64(MSR_CORE_THREAD_COUNT);
        info_p->core_count   = bitfield((uint32_t)msr, 31, 16);
        info_p->thread_count = bitfield((uint32_t)msr, 15,  0);
        break;
      case CPU_MODEL_WESTMERE: // Intel Core i7 LGA1366 (32nm) 6 Core
      case CPU_MODEL_DALES:
      case CPU_MODEL_WESTMERE_EX:
        msr = rdmsr64(MSR_CORE_THREAD_COUNT);
        info_p->core_count   = bitfield((uint32_t)msr, 19, 16);
        info_p->thread_count = bitfield((uint32_t)msr, 15,  0);
        break;
      case CPU_MODEL_ATOM_3700:
        info_p->core_count   = 4;
        info_p->thread_count = 4;
        break;
      default:
        do_cpuid(1, cpuid_reg);
        info_p->core_count = bitfield(cpuid_reg[1], 23, 16);
        info_p->thread_count = info_p->cpuid_logical_per_package;
        //workaround for N270. I don't know why it detected wrong
        if ((info_p->cpuid_model == CPU_MODEL_ATOM) &&
            (info_p->cpuid_stepping == 2)) {
          info_p->core_count = 1;
        }
        break;
    }
  }
  if (info_p->core_count == 0) {
    info_p->core_count   = info_p->cpuid_cores_per_package;
    info_p->thread_count = info_p->cpuid_logical_per_package;
  }
}


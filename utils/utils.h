/*
 *  utils.h
 *  HWSensors
 *
 *  Created by Kozel Rogati on 05.04.11.
 *  Copyright 2011 mozodojo. All rights reserved.
 *
 */

static inline UInt16 swap_value(UInt16 value)
{
	return ((value & 0xff00) >> 8) | ((value & 0xff) << 8);
}
//we will use fpNM for voltages taking into account that input data in milliVolts while output in Volts
static inline UInt16 encode_fp2e(UInt16 value) {
    UInt32 tmp = value;
    tmp = (tmp << 14) / 1000;
    value = (UInt16)(tmp & 0xffff);
    return swap_value(value);
}

static inline UInt16 encode_sp4b(UInt16 value) {
  UInt32 tmp = (value < 0x8000)?value:(~value);
  tmp = (tmp << 11) / 1000;
  value = (UInt16)(tmp & 0xffff);
  return swap_value(value);
}


/*
static inline UInt16 encode_fp3d(UInt16 value) {
  UInt32 tmp = value;
  tmp = (tmp << 13) / 1000;
  value = (UInt16)(tmp & 0xffff);
  return swap_value(value);
}

static inline UInt16 encode_fp4c(UInt16 value) {
    UInt32 tmp = value;
    tmp = (tmp << 12) / 1000;
    value = (UInt16)(tmp & 0xffff);
    return swap_value(value);
}

static inline UInt16 encode_fp5b(UInt16 value) {
  UInt32 tmp = value;
  tmp = (tmp << 11) / 1000;
  value = (UInt16)(tmp & 0xffff);
  return swap_value(value);
}

static inline UInt16 encode_sp5a(UInt16 value) {
  UInt32 tmp = (value < 0x8000)?value:(~value);
  tmp = (tmp << 10) / 1000;
  tmp = tmp & 0x7fff;
  if (value > 0x8000) {
    value = (UInt16)tmp | 0x8000;
  } else {
    value = (UInt16)tmp;
  }

  return swap_value(value);
}
*/
static inline UInt16 encode_fpe2(UInt16 value) {
	return swap_value(value << 2);
}

static inline UInt16 decode_fpe2(UInt16 value) {
	return (swap_value(value) >> 2);
}

// https://stackoverflow.com/questions/8377412/ceil-function-how-can-we-implement-it-ourselves
static inline float hw_ceil(float f) {
  unsigned input;
  memcpy(&input, &f, 4);
  int exponent = ((input >> 23) & 255) - 127;
  if (exponent < 0) return (f > 0);
  // small numbers get rounded to 0 or 1, depending on their sign
  
  int fractional_bits = 23 - exponent;
  if (fractional_bits <= 0) return f;
  // numbers without fractional bits are mapped to themselves
  
  unsigned integral_mask = 0xffffffff << fractional_bits;
  unsigned output = input & integral_mask;
  // round the number down by masking out the fractional bits
  
  memcpy(&f, &output, 4);
  if (f > 0 && output != input) ++f;
  // positive numbers need to be rounded up, not down
  
  return f;
}

static inline int hw_round(float fl) {
  return fl < 0 ? fl - 0.5 : fl + 0.5;
}

static inline const char * hw_strstr(const char *str, const char *prefix) {
  char c, sc;
  size_t len;
  
  if ((c = *prefix++) != 0) {
    len = strlen(prefix);
    do {
      do {
        if ((sc = *str++) == 0) {
          return (NULL);
        }
      } while (sc != c);
    } while (strncmp(str, prefix, len) != 0);
    str--;
  }
  return str;
}

static inline bool isModelREVLess(const char * model) {
  /*
   don't add 'REV ', 'RBr ' and EPCI if for these models and newer:
   MacBookPro15,1
   MacBookAir8,1
   Macmini8,1
   iMacPro1,1
   // the list is going to increase?
   */
  bool result = false;
  long maj = 0;
  long rev = 0;
  
  const char *family[7] = { /* order is important */
    "MacPro",
    "MacBookAir",
    "MacBookPro",
    "MacBook",
    "Macmini",
    "iMacPro",
    "iMac"
  };
  
  if (!model) {
    return result;
  }
  
  for (int i = 0; i < 7; i++) {
    if (hw_strstr(model, family[i])) {
      char suffix[6], majStr[3], revStr[3]; // 15,10 (never seen but possible + null)
      
      // get the numeric part into a string
      snprintf(suffix, (strlen(model) - strlen(family[i])) + 1, "%s", model + strlen(family[i]));
      size_t commaIndex = 0;
      
      // get the comma position to split the suffix into a major and revision version:
      char c = suffix[commaIndex];
      do {
        if (c == ',') {
          break;
        }
        commaIndex++;
        c = suffix[commaIndex];
      } while (c != '\0');
      
      if (commaIndex == 0) {
        return result; // failure
      }
      // populate the two strings with major and revision version:
      snprintf(majStr, commaIndex + 1, "%s", suffix);
      snprintf(revStr, (strlen(suffix) - commaIndex) + 1, "%s", suffix + commaIndex + 1);
      
      // make it numbers:
      maj = strtol(majStr, (char **)NULL, 10);
      rev = strtol(revStr, (char **)NULL, 10);
      
      // Apple never took 0
      if (maj && rev) {
        // looks for a mach
        if (strncmp(family[i], "iMacPro", strlen("iMacPro")) == 0 &&
            ((maj == 1 && rev >= 1) || maj >= 1)) {
          result = true;
        } else if (strncmp(family[i], "Macmini", strlen("Macmini")) == 0 &&
                   ((maj == 8 && rev >= 1) || maj >= 8)) {
          result = true;
        } else if (strncmp(family[i], "MacBookAir", strlen("MacBookAir")) == 0 &&
                   ((maj == 8 && rev >= 1) || maj >= 8)) {
          result = true;
        } else if (strncmp(family[i], "MacBookPro", strlen("MacBookPro")) == 0 &&
                   ((maj == 15 && rev >= 1) || maj >= 15)) {
          result = true;
        }
      }
      break;
    }
  }
  
  return result;
}

static inline bool process_sensor_entry(OSObject *object,
                                        OSString **name,
                                        long *Ri,
                                        long *Rf,
                                        long *Vf) {
  *Rf=1;
  *Ri=0;
  *Vf=0;
  if ((*name = OSDynamicCast(OSString, object))) {
    return true;
  } else if (OSDictionary *dictionary = OSDynamicCast(OSDictionary, object)) {
    if ((*name = OSDynamicCast(OSString, dictionary->getObject("Name")))) {
      if (OSNumber *number = OSDynamicCast(OSNumber, dictionary->getObject("VRef"))) {
        *Vf = (long)number->unsigned64BitValue();
      }
      
      if (OSNumber *number = OSDynamicCast(OSNumber, dictionary->getObject("Ri"))) {
        *Ri = (long)number->unsigned64BitValue();
      }
      
      if (OSNumber *number = OSDynamicCast(OSNumber, dictionary->getObject("Rf"))) {
        *Rf = (long)number->unsigned64BitValue();
        if (*Rf == 0) {
          *Rf = 1;
        }
      }
      return true;
    }
  }
  
  return false;
}

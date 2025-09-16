#include "conversion.h"

/*
 Observation:

 Problem:
    There are duplicate functions like byteToKB() and byteToKB_64u() that do the same thing but with different types.
    
    If udiv64() is just a wrapper for division, there's no need to duplicate the logic.
    
    Improvement:
    Consolidate into a single function using uint64_t as the base type.
    
    If the environment doesn't allow overloading, use clearer names like Convert_BytesToKB64().

  2. Nested division in "byteToMB": "return (bytes / 1024) / 1024"
     - Improvement:
        Replace with bytes / (1024 * 1024) for clarity and efficiency.

  3. Naming consistency
        Issue:
           Mixing camelCase and snake_case (byteToKB_64u vs kBToMB).
        
           The _64u suffix is ​​uncommon and can be confusing.
        
        Improvement:
            Use consistent names like convert_bytes_to_kb64() or simply bytes_to_kb() if the context is clear.

*/

int byteToKB(int bytes) {
    return bytes / 1024;
}
int byteToMB(int bytes) {
    return (bytes / 1024) / 1024;
}
int kBToMB(int kB) {
    return kB / 1024;
}

int byteToKB_64u(uint64_t bytes) {
    return udiv64(bytes, 1024);
}
int byteToMB_64u(uint64_t bytes) {
    return udiv64(udiv64(bytes, 1024), 1024);
}
int kBToMB_64u(uint64_t kB) {
    return udiv64(kB, 1024);
}

/*
as it should:


// Convert bytes to kilobytes (rounded down)
uint64_t bytes_to_kb(uint64_t bytes) {
    return bytes / 1024;
}

// Convert bytes to megabytes (rounded down)
uint64_t bytes_to_mb(uint64_t bytes) {
    return bytes / (1024 * 1024);
}

// Convert kilobytes to megabytes (rounded down)
uint64_t kb_to_mb(uint64_t kb) {
    return kb / 1024;
}

*/

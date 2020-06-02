/*
Copyright (c) 2015-2020, Intel Corporation
Copyright (c) 2019-2020, University of Utah

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

     http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

//////////////////////////////////////////////////////////////////////////////
#include <iostream>

//TODO: should be disabled for Windows
static void __cpuid(int cpuid_regs[4], int category) {
    __asm__ __volatile__ ("cpuid"
                          : "=a" (cpuid_regs[0]), "=b" (cpuid_regs[1]), "=c" (cpuid_regs[2]), "=d" (cpuid_regs[3])
                          : "0" (category));
}

static void __cpuidex(int feature_regs[4], int level, int count) {
    //Save ebx in esi (it can be PIC register)
    __asm__ __volatile__ ("xchg{l}\t{%%}ebx, %1\n\t"
                          "cpuid\n\t"
                          "xchg{l}\t{%%}ebx, %1\n\t"
                          : "=a" (feature_regs[0]), "=r" (feature_regs[1]), "=c" (feature_regs[2]), "=d" (feature_regs[3])
                          : "0" (level), "2" (count));
}

static int call_xgetbv() {
    // Call xgetbv. We use byte sequence in order to support older assemblers.
    int eax, edx;
    __asm__ __volatile__ (".byte 0x0f, 0x01, 0xd0" : "=a" (eax), "=d" (edx) : "c" (0));
    return eax;
}

static bool has_avx512(int cpuid_regs[4], int feature_regs[4]) {
        return ((cpuid_regs  [2] & (1 << 27)) != 0 && // OSXSAVE
                (feature_regs[1] & (1 << 16)) != 0 && // AVX512 F
                // See https://software.intel.com/sites/default/files/managed/26/40/319433-026.pdf chapter 2.1
                // XCR0[7:5] = '111b' and at XCR0[2:1] = '11b'
                ((call_xgetbv() & 0xE6) == 0xE6));
}

static bool has_avx(int cpuid_regs [4]) {
    return ((cpuid_regs[2] & (1 << 27)) != 0 && // OSXSAVE
            (cpuid_regs[2] & (1 << 28)) != 0 && // AVX
            ((call_xgetbv() & 6) == 6));
}

std::string getSystemISA () {
    int cpuid_regs[4];
    __cpuid(cpuid_regs, 1);

    int feature_regs[4];
    // Get info about extended features: cpuid with eax=7, ecx=0
    __cpuidex(feature_regs, 7, 0);

    if (has_avx512(cpuid_regs, feature_regs)) {
        if ((feature_regs[1] & (1 << 17)) != 0 && // AVX512 DQ
            (feature_regs[1] & (1 << 28)) != 0 && // AVX512 CDI
            (feature_regs[1] & (1 << 30)) != 0 && // AVX512 BW
            (feature_regs[1] & (1 << 31)) != 0) { // AVX512 VL
            return "skx";
        }
        else if ((feature_regs[1] & (1 << 26)) != 0 && // AVX512 PF
                 (feature_regs[1] & (1 << 27)) != 0 && // AVX512 ER
                 (feature_regs[1] & (1 << 28)) != 0) { // AVX512 CDI
            return "knl";
        }
        // If it is unknown AVX512, fall back to one of previous ISA
    }

    if (has_avx(cpuid_regs)) {
        // It Already has AVX1 at least.
        // Ivy Bridge?
        if ((cpuid_regs[2] & (1 << 29)) != 0 &&  // F16C
            (cpuid_regs[2] & (1 << 30)) != 0) {  // RDRAND
            // AVX2?
            if ((feature_regs[1] & (1 << 5)) != 0)
                return "hsw"; // AVX2 Haswell
            else 
                return "ivb"; // AVX1.1 Ivy Bridge
        }
        // Regular AVX
        return "snb"; // AVX - Sandy Bridge
    }
    else if ((cpuid_regs[2] & (1 << 19)) != 0)
        return "pnr"; // SSE4 - Penryn
    else if ((cpuid_regs[3] & (1 << 26)) != 0)
        return "p4"; // SSE2 - Pentium4
    else 
        return "Error";
}

int main () {
    std::cout << getSystemISA () << std::endl;
    return 0;
}

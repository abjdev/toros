// Copyright (C) 2021  ilobilo

#pragma once
#include <stdint.h>

namespace kernel::system::cpu::gdt {

struct GDTDescriptor {
    uint16_t Size;
    uint64_t Offset;
} __attribute__((packed));

struct GDTEntry {
    uint16_t Limit0;
    uint16_t Base0;
    uint8_t Base1;
    uint8_t AccessByte;
    uint8_t Limit1_Flags;
    uint8_t Base2;
} __attribute__((packed));

struct GDT {
    GDTEntry Null;
    GDTEntry _16Code;
    GDTEntry _16Data;
    GDTEntry _32Code;
    GDTEntry _32Data;
    GDTEntry _64Code;
    GDTEntry _64Data;
    GDTEntry UserData;
    GDTEntry UserCode;
    GDTEntry Tss;
} __attribute__((packed))
__attribute((aligned(0x1000)));

struct TSS {
    uint32_t Reserved0;
    uint64_t RSP[3];
    uint64_t Reserved1;
    uint64_t IST[7];
    uint64_t Reserved2;
    uint16_t Reserved3;
    uint16_t IOPBOffset;
} __attribute__((packed));

extern GDT DefaultGDT;
extern bool initialised;
extern TSS *tss;

extern "C" void LoadGDT(GDTDescriptor *gdtDescriptor);
extern "C" void LoadTSS();

void reloadall(int cpu);
void reloadgdt();
void reloadtss();
void init();

void TSS_write(uint64_t RSP0, uint64_t RSP1);

void set_stack(uint64_t cpu, uintptr_t stack);
uint64_t get_stack(uint64_t cpu);
}
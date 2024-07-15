
#ifndef LAPIC_H
#define LAPIC_H
#include "kernel.h"

#define MSR_IA32_APIC_BASE 0x1B

static const uint64_t lapic_pa = 0xFEE00000;

// APIC register IDs
enum apic_registers {
  APIC_REG_ID = 0x02,
  APIC_REG_TPR = 0x08,
  APIC_REG_SVR = 0x0F,
  APIC_REG_ISR = 0x10,
  APIC_REG_TMR = 0x18,
  APIC_REG_IRR = 0x20,
  APIC_REG_ESR = 0x28,
  APIC_REG_EOI = 0x0B,
  APIC_REG_CMCI = 0x2F,
  APIC_REG_LVT_TIMER = 0x32,
  APIC_REG_LVT_LINT0 = 0x35,
  APIC_REG_LVT_LINT1 = 0x36,
  APIC_REG_LVT_ERROR = 0x37,
  APIC_REG_ICR_LOW = 0x30,
  APIC_REG_ICR_HIGH = 0x31,
  APIC_REG_TIMER_INITIAL_COUNT = 0x38,
  APIC_REG_TIMER_CURRENT_COUNT = 0x39,
  APIC_REG_TIMER_DIVIDE = 0x3E
};
// IPI types
enum ipi_type { IPI_INIT = 0x500, IPI_STARTUP = 0x600 };
// IPI delivery status and levels
enum ipi_delivery {
  IPI_DELIVERY_STATUS = 0x1000,
  IPI_LEVEL_ASSERT = 0x4000,
  IPI_TRIGGER_LEVEL = 0x8000,
  IPI_GIVEN = 0,
  IPI_SELF = 0x40000,
  IPI_ALL = 0x80000,
  IPI_ALL_EXCLUDING_SELF = 0xC0000
};

// Timer settings
enum timer_settings { TIMER_DIVIDE_1 = 0x0B, TIMER_PERIODIC = 0x20000 };

// LVT settings
enum lvt_settings { LVT_MASKED = 0x10000 };

struct apic_reg {
  uint32_t v;
  uint32_t padding[3];
};

typedef struct lapicstate {
  volatile struct apic_reg reg[0x40];
} lapicstate_t;

void lapic_enable(lapicstate_t* lapic, int vector);

uint32_t lapic_read(lapicstate_t* lapic, int reg);

void lapic_write(lapicstate_t* lapic, int reg, uint32_t vector);

void lapic_disable(lapicstate_t* lapic);

uint32_t lapic_error(lapicstate_t* lapic);

void lapic_ack(lapicstate_t* lapic);

// lapic_get
//      Get the CPUs APIC device
lapicstate_t* lapic_get();

#endif // LAPIC_H

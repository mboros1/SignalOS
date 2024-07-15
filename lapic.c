#include "lapic.h"

void lapic_enable(lapicstate_t *lapic, int vector) {
  lapic_write(lapic, APIC_REG_SVR,
              (lapic_read(lapic, APIC_REG_SVR) & ~0xFF) | 0x100 | vector);
}

uint32_t lapic_read(lapicstate_t *lapic, int reg) { return lapic->reg[reg].v; }

void lapic_write(lapicstate_t *lapic, int reg, uint32_t v) { lapic->reg[reg].v = v; }

uint32_t lapic_error(lapicstate_t *lapic) {
  // TODO: is this correct? setting the reg to 0 then reading it?
  lapic_write(lapic, APIC_REG_ESR, 0);
  return lapic_read(lapic, APIC_REG_ESR);
}

void lapic_ack(lapicstate_t* lapic) {
    lapic_write(lapic, APIC_REG_EOI, 0);
}

lapicstate_t* lapic_get() {
    return (lapicstate_t*)lapic_pa;
}


#ifndef LAPIC_H
#define LAPIC_H
#include "kernel.h"

#define MSR_IA32_APIC_BASE 0x1B

typedef struct lapicstate {
    // APIC register IDs
    enum {
        reg_id = 0x02,
        reg_tpr = 0x08,
        reg_svr = 0x0F,
        reg_isr = 0x10,
        reg_tmr = 0x18,
        reg_irr = 0x20,
        reg_esr = 0x28,
        reg_eoi = 0x0B,
        reg_cmci = 0x2F,
        reg_lvt_timer = 0x32,
        reg_lvt_lint0 = 0x35,
        reg_lvt_lint1 = 0x36,
        reg_lvt_error = 0x37,
        reg_icr_low = 0x30,
        reg_icr_high = 0x31,
        reg_timer_initial_count = 0x38,
        reg_timer_current_count = 0x39,
        reg_timer_divide = 0x3E
    };

    enum ipi_type_t {
        ipi_init = 0x500,
        ipi_startup = 0x600
    };

    enum {
        ipi_delivery_status = 0x1000,
        ipi_level_assert = 0x4000,
        ipi_trigger_level = 0x8000,
        ipi_given = 0,
        ipi_self = 0x40000,
        ipi_all = 0x80000,
        ipi_all_excluding_self = 0xC0000
    };

    enum {
        timer_divide_1 = 0x0B,
        timer_periodic = 0x20000
    };

    enum {
        lvt_masked = 0x10000
    };

} lapicstate_t;

#endif // LAPIC_H

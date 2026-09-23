#ifndef PC_SIDE_H
#define PC_SIDE_H

/* Host-only API shared by main.c, pc_hal.c and pc_side_host.c.
 * No firmware TU includes this file. */

#include <stdint.h>

/* ---- UDP CAN transport (pc_side_host.c) -------------------------------- */
int  pc_can_init(unsigned short recv_port, unsigned short send_port);
void pc_can_shutdown(void);
int  pc_can_send(uint16_t id, const uint8_t *data, int len);
int  pc_can_rx_level(void);
int  pc_can_rx_pop(uint16_t *id, uint8_t *data, int *len);

/* ---- RTI thread (pc_side_host.c) --------------------------------------- */
void         rti_thread_start(void);
void         rti_thread_stop(void);
unsigned int pc_rti_ticks(void);
unsigned int pc_rti_skipped(void);

/* ---- Interrupt masking, reset ------------------------------------------ */
void pc_intr_set_masked(int masked);
int  pc_intr_masked(void);
void pc_side_reset(void);
int  pc_is_resetting(void);

/* ---- Loop instrumentation, fed by the HAL_WWDG_Refresh stand-in -------- */
void          pc_loop_tick(void);
unsigned long pc_loop_count(void);
unsigned int  pc_loop_max_gap_ms(void);
void          pc_loop_reset_max_gap(void);

/* Defined in main.c, called once per firmware loop pass. */
void pc_log_changes(void);

/* ---- FDCAN filters, applied by the receive thread (pc_hal.c) ----------- */
int pc_can_filter_accept(uint16_t id);
int pc_tim1_it_enabled(void);

/* ---- Fake flash (pc_hal.c) --------------------------------------------- */
int  pc_flash_init(unsigned short recv_port);
void pc_flash_sync(void);

/* ---- Menu-variable block, checked at startup (pc_hal.c) ---------------- */
int pc_menu_data_check(void);

#endif /* PC_SIDE_H */

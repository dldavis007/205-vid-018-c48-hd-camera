#ifndef PC_LOG_H
#define PC_LOG_H

/* Host logging; both macros compile to nothing without PC_SIDE.
 *   LOG_PRINTF(("fmt", args))   double parens, as in the CNT-019 host
 *   LOG_IF_CHANGED(fmt, val)    prints once per change of val; fmt has one %ld */

#ifdef PC_SIDE

#include <stdio.h>

/* Async and drop-on-full (pc_side_host.c): logging never blocks the caller. */
void pc_log_init(void);
void pc_log_shutdown(void);
void pc_log_printf(const char *fmt, ...);

#define LOG_PRINTF(args) pc_log_printf args

#define LOG_IF_CHANGED(fmt, val)                     \
    do {                                             \
        static long _lc_last = 0x7FFFFFFFL;          \
        long _lc_v = (long)(val);                    \
        if (_lc_v != _lc_last) {                     \
            pc_log_printf(fmt "\n", _lc_v);          \
            _lc_last = _lc_v;                        \
        }                                            \
    } while (0)

#else

#define LOG_PRINTF(args)         ((void)0)
#define LOG_IF_CHANGED(fmt, val) ((void)0)

#endif /* PC_SIDE */

#endif /* PC_LOG_H */

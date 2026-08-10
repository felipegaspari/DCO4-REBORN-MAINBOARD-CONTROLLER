#ifndef __MB_BENCH_H__
#define __MB_BENCH_H__

// Single-core STM32 hot-path profiler (DCO bench.h subset).
// RUNNING_AVERAGE off → every BENCH_* macro is a no-op.
// RUNNING_AVERAGE_PERIOD → only BENCH_PERIOD (loop); stage BEGIN/END compile out.
// Time source: DWT CYCCNT (BENCH_USE_DWT=1, default). Dump window stays micros().
// Do not use SysTick — stm32duino millis()/micros() own it.
// Dump text goes to DCO over slim 't' chunks, then USB CDC / dco_control.

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#ifndef BENCH_USE_DWT
#define BENCH_USE_DWT 1
#endif
#ifndef BENCH_PERIOD_MAX_US
#define BENCH_PERIOD_MAX_US 20000000
#endif
#if BENCH_PERIOD_MAX_US < 1
#undef BENCH_PERIOD_MAX_US
#define BENCH_PERIOD_MAX_US 1
#endif
#ifndef BENCH_STAGE_STRIDE
#define BENCH_STAGE_STRIDE 1
#endif
#if BENCH_STAGE_STRIDE < 1
#undef BENCH_STAGE_STRIDE
#define BENCH_STAGE_STRIDE 1
#endif

#define BENCH_US  1
#define BENCH_CYC 0
#if BENCH_USE_DWT
#define BENCH_PERIOD_KIND BENCH_CYC
#else
#define BENCH_PERIOD_KIND BENCH_US
#endif
#define BENCH_T_MAIN 0
#define BENCH_T_FINE 1
#define BENCH_T_RARE 2

#define BENCH_PROBES(X)                                                                 \
  X(loop_period, 0, BENCH_US, BENCH_T_MAIN, BENCH_NONE,       "loop period")           \
  X(millisTimer, 0, BENCH_US, BENCH_T_MAIN, BENCH_loop_period, "millisTimer")          \
  X(serial_1_8,  0, BENCH_US, BENCH_T_RARE, BENCH_loop_period, "Serial 1+8")           \
  X(ms1_block,   0, BENCH_US, BENCH_T_RARE, BENCH_loop_period, "1ms DRIFT_LFOs")       \
  X(sendSerial,  0, BENCH_US, BENCH_T_RARE, BENCH_loop_period, "sendSerial")           \
  X(serial_2,    0, BENCH_US, BENCH_T_MAIN, BENCH_loop_period, "Serial 2")             \
  X(lfos,        0, BENCH_US, BENCH_T_MAIN, BENCH_loop_period, "LFO1+LFO2")            \
  X(adsr,        0, BENCH_US, BENCH_T_MAIN, BENCH_loop_period, "ADSR_update")          \
  X(cv_outs,     0, BENCH_US, BENCH_T_MAIN, BENCH_loop_period, "update_CV_outs")

enum BenchId {
#define BENCH_X(id, core, kind, tier, parent, label) BENCH_##id,
  BENCH_PROBES(BENCH_X)
#undef BENCH_X
  BENCH_COUNT
};
#define BENCH_NONE BENCH_COUNT

void serial_send_bench_text_chunk(const uint8_t* data, uint8_t n);

#ifndef RUNNING_AVERAGE

#define BENCH_PERIOD(id)     ((void)0)
#define BENCH_SAMPLE_TICK()  ((void)0)
#define BENCH_BEGIN(id)      ((void)0)
#define BENCH_END(id)        ((void)0)
static inline void bench_init() {}
static inline void bench_poll() {}
static inline void bench_reset_all() {}
static inline void bench_request_dump() {}
static inline void bench_toggle_periodic() {}
static constexpr bool bench_out_active = false;

#else  // RUNNING_AVERAGE

static inline uint32_t bench_us_now(void) {
  return (uint32_t)micros();
}
#if BENCH_USE_DWT
static inline uint32_t bench_now(void) {
  return DWT->CYCCNT;
}
#else
static inline uint32_t bench_now(void) {
  return bench_us_now();
}
#endif
static inline uint32_t bench_span(uint32_t start, uint32_t end) {
  return end - start;
}

uint32_t bench_cycles_per_us = 1;
uint32_t bench_overhead_raw = 0;
uint32_t bench_period_max_raw = (uint32_t)BENCH_PERIOD_MAX_US;
uint32_t bench_window_start_us = 0;
uint32_t bench_window_us = 0;

volatile bool bench_out_active = false;
volatile bool bench_dump_request = false;
volatile bool bench_ready = false;
volatile bool bench_periodic = false;
volatile uint8_t bench_period_gen = 0;

uint32_t bench_sample_ctr = 0;
bool bench_stage_on = false;
uint32_t bench_periodic_last_us = 0;

#define BENCH_OUT_CAP   2048
#define BENCH_T_DATA_MAX 15

char bench_out_buf[BENCH_OUT_CAP];
uint16_t bench_out_len = 0;
uint16_t bench_out_pos = 0;

inline void bench_out_reset() {
  bench_out_len = 0;
  bench_out_pos = 0;
}

inline void bench_out_puts(const char* s) {
  while (*s != '\0' && bench_out_len + 1u < BENCH_OUT_CAP) {
    bench_out_buf[bench_out_len++] = *s++;
  }
}

inline void bench_out_println(const char* s) {
  bench_out_puts(s);
  bench_out_puts("\n");
}

struct BenchDesc {
  uint8_t core;
  uint8_t kind;
  uint8_t tier;
  uint8_t parent;
  const char* label;
};

static const BenchDesc bench_desc[BENCH_COUNT] = {
#define BENCH_X(id, core, kind, tier, parent, label) { core, kind, tier, parent, label },
  BENCH_PROBES(BENCH_X)
#undef BENCH_X
};

struct BenchStat {
  uint32_t n;
  uint32_t min;
  uint32_t max;
  uint64_t sum;
};

BenchStat bench_stats[BENCH_COUNT];
BenchStat bench_snap[BENCH_COUNT];

static inline void bench_stat_add(BenchStat* s, uint32_t d) {
  s->sum += d;
  if (s->n == 0u || d < s->min) s->min = d;
  if (d > s->max) s->max = d;
  s->n++;
}

static inline void bench_add_us(uint8_t id, uint32_t start) {
  const uint32_t d0 = bench_span(start, bench_now());
  if (bench_out_active) return;
  if (d0 == 0u || d0 > bench_period_max_raw) return;
  const uint32_t d = (d0 > bench_overhead_raw) ? (d0 - bench_overhead_raw) : 0u;
  bench_stat_add(&bench_stats[id], d);
}

static inline void bench_add_us_sampled(uint8_t id, uint32_t start) {
  const uint32_t d0 = bench_span(start, bench_now());
  if (bench_out_active) return;
  if (d0 == 0u || d0 > bench_period_max_raw) return;
  const uint32_t d = (d0 > bench_overhead_raw) ? (d0 - bench_overhead_raw) : 0u;
  BenchStat* s = &bench_stats[id];
  const uint32_t stride = (uint32_t)BENCH_STAGE_STRIDE;
  s->sum += (uint64_t)d * stride;
  if (s->n == 0u || d < s->min) s->min = d;
  if (d > s->max) s->max = d;
  s->n += stride;
}

static inline uint32_t bench_stage_begin(uint8_t id) {
  if (bench_desc[id].tier == BENCH_T_RARE) return bench_now();
  if (!bench_stage_on) return 0u;
  return bench_now();
}

static inline void bench_stage_end(uint8_t id, uint32_t start) {
  if (bench_desc[id].tier == BENCH_T_RARE) {
    bench_add_us(id, start);
    return;
  }
  if (!bench_stage_on) return;
  bench_add_us_sampled(id, start);
}

static inline void bench_add_raw(uint8_t id, uint32_t d) {
  bench_stat_add(&bench_stats[id], d);
}

#define BENCH_PERIOD(id)                                                  \
  do {                                                                    \
    static uint32_t bench_prev_##id = 0u;                                 \
    static uint8_t bench_gen_##id = 0u;                                   \
    static uint8_t bench_ok_##id = 0u;                                    \
    const uint32_t bench_t_##id = bench_now();                            \
    const uint8_t bench_g_##id = bench_period_gen;                        \
    if (bench_gen_##id != bench_g_##id) {                                 \
      bench_gen_##id = bench_g_##id;                                      \
      bench_ok_##id = 0u;                                                 \
    }                                                                     \
    if (bench_ok_##id && !bench_out_active) {                             \
      const uint32_t d = bench_t_##id - bench_prev_##id;                   \
      if (d > 0u && d <= bench_period_max_raw) {                          \
        bench_add_raw(BENCH_##id, d);                                     \
      }                                                                   \
    }                                                                     \
    bench_prev_##id = bench_t_##id;                                       \
    bench_ok_##id = 1u;                                                   \
  } while (0)

#ifdef RUNNING_AVERAGE_PERIOD
#define BENCH_SAMPLE_TICK() ((void)0)
#define BENCH_BEGIN(id)     ((void)0)
#define BENCH_END(id)       ((void)0)
#else
#define BENCH_SAMPLE_TICK()                                               \
  do {                                                                    \
    uint32_t bench_n_ = ++bench_sample_ctr;                               \
    if (bench_n_ >= (uint32_t)BENCH_STAGE_STRIDE) {                       \
      bench_sample_ctr = 0u;                                              \
      bench_stage_on = true;                                              \
    } else {                                                              \
      bench_stage_on = false;                                             \
    }                                                                     \
  } while (0)
#define BENCH_BEGIN(id) volatile uint32_t bench_t_##id = bench_stage_begin(BENCH_##id)
#define BENCH_END(id)   bench_stage_end(BENCH_##id, bench_t_##id)
#endif

static constexpr uint32_t BENCH_MIN_WINDOW_US = 1000000u;

inline void bench_init() {
  uint32_t clk = SystemCoreClock;
  if (clk == 0u) clk = F_CPU;
  bench_cycles_per_us = clk / 1000000u;
  if (bench_cycles_per_us == 0u) bench_cycles_per_us = 1u;
#if BENCH_USE_DWT
  CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
  DWT->CYCCNT = 0;
  DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
  {
    const uint64_t cap = (uint64_t)bench_cycles_per_us * (uint32_t)BENCH_PERIOD_MAX_US;
    bench_period_max_raw = (cap > 0xFFFFFFFFull) ? 0xFFFFFFFFu : (uint32_t)cap;
  }
#else
  bench_period_max_raw = (uint32_t)BENCH_PERIOD_MAX_US;
#endif
  if (bench_period_max_raw == 0u) bench_period_max_raw = 1u;
  uint32_t best = 0xFFFFFFFFu;
  for (int i = 0; i < 32; ++i) {
    const uint32_t a = bench_now();
    const uint32_t b = bench_now();
    const uint32_t d = bench_span(a, b);
    if (d < best) best = d;
  }
  bench_overhead_raw = best;
  bench_window_start_us = bench_us_now();
  bench_periodic_last_us = bench_window_start_us;
}

inline void bench_reset_all() {
  if (bench_out_active) return;
  for (uint8_t i = 0; i < BENCH_COUNT; ++i) {
    bench_stats[i] = BenchStat{};
    bench_snap[i] = BenchStat{};
  }
  bench_window_start_us = bench_us_now();
  bench_period_gen++;
  bench_ready = false;
  bench_dump_request = false;
  bench_out_reset();
}

inline void bench_service() {
  if (!bench_dump_request || bench_ready) return;
  const uint32_t now_us = bench_us_now();
  const uint32_t elapsed = now_us - bench_window_start_us;
  if (elapsed < BENCH_MIN_WINDOW_US) return;
  bench_window_us = elapsed;
  bench_window_start_us = now_us;
  bench_period_gen++;
  for (uint8_t i = 0; i < BENCH_COUNT; ++i) {
    bench_snap[i] = bench_stats[i];
    bench_stats[i] = BenchStat{};
  }
  bench_ready = true;
}

inline uint64_t bench_to_us100(uint64_t raw) {
#if BENCH_USE_DWT
  return (raw * 100u + (bench_cycles_per_us / 2u)) / bench_cycles_per_us;
#else
  return raw * 100u;
#endif
}

inline void bench_fmt_us100(char* out, size_t n, uint64_t us100) {
  snprintf(out, n, "%lu.%02lu", (unsigned long)(us100 / 100u), (unsigned long)(us100 % 100u));
}

#define BENCH_ROW_FMT "%-28s %8s %9s %9s %9s %14s %6s"

inline void bench_fmt_win(char* out, size_t n, uint64_t sum_us100, uint32_t window_us) {
  if (window_us == 0u) {
    snprintf(out, n, "0.00");
    return;
  }
  const uint64_t pct_x100 = (sum_us100 * 100u) / window_us;
  if (sum_us100 > 0u && pct_x100 == 0u) {
    snprintf(out, n, "<0.01");
    return;
  }
  snprintf(out, n, "%lu.%02lu",
           (unsigned long)(pct_x100 / 100u), (unsigned long)(pct_x100 % 100u));
}

inline void bench_print_row(uint8_t id, const char* indent) {
  const BenchDesc& d = bench_desc[id];
  const BenchStat& s = bench_snap[id];
  if (s.n == 0u) return;

  char name[32], mean[16], mn[16], mx[16], tot[20], cnt[12], win[8], line[160];
  snprintf(name, sizeof(name), "%s%s", indent, d.label);
  snprintf(cnt, sizeof(cnt), "%lu", (unsigned long)s.n);

  const uint64_t sum_us100 = bench_to_us100(s.sum);
  const uint64_t mean_us100 = (sum_us100 + (s.n / 2u)) / s.n;
  bench_fmt_us100(mean, sizeof(mean), mean_us100);
  bench_fmt_us100(mn, sizeof(mn), bench_to_us100(s.min));
  bench_fmt_us100(mx, sizeof(mx), bench_to_us100(s.max));
  bench_fmt_us100(tot, sizeof(tot), sum_us100);
  bench_fmt_win(win, sizeof(win), sum_us100, bench_window_us);

  snprintf(line, sizeof(line), BENCH_ROW_FMT, name, cnt, mean, mn, mx, tot, win);
  bench_out_println(line);
}

inline void bench_print_unattributed(uint8_t parent, const char* indent) {
  const BenchStat& p = bench_snap[parent];
  if (p.n == 0u) return;
  uint64_t children_us = 0;
  bool any = false;
  for (uint8_t i = 0; i < BENCH_COUNT; ++i) {
    if (bench_desc[i].parent != parent || bench_snap[i].n == 0u) continue;
    children_us += bench_snap[i].sum;
    any = true;
  }
  if (!any) return;

  char name[32], tot[20], win[8], line[160];
  if (p.sum >= children_us) {
    const uint64_t rest = p.sum - children_us;
    snprintf(name, sizeof(name), "%s%s", indent, "(unattributed)");
    const uint64_t rest_us100 = bench_to_us100(rest);
    bench_fmt_us100(tot, sizeof(tot), rest_us100);
    bench_fmt_win(win, sizeof(win), rest_us100, bench_window_us);
  } else {
    const uint64_t rest = children_us - p.sum;
    snprintf(name, sizeof(name), "%s%s", indent, "(over-attributed)");
    const uint64_t rest_us100 = bench_to_us100(rest);
    snprintf(tot, sizeof(tot), "-%lu.%02lu",
             (unsigned long)(rest_us100 / 100u), (unsigned long)(rest_us100 % 100u));
    bench_fmt_win(win, sizeof(win), rest_us100, bench_window_us);
  }
  snprintf(line, sizeof(line), BENCH_ROW_FMT, name, "-", "-", "-", "-", tot, win);
  bench_out_println(line);
}

inline void bench_print_subtree(uint8_t id, const char* indent) {
  bench_print_row(id, indent);
  char child_indent[16];
  const size_t len = strlen(indent);
  if (len + 2u >= sizeof(child_indent)) return;
  memcpy(child_indent, indent, len);
  child_indent[len] = ' ';
  child_indent[len + 1u] = ' ';
  child_indent[len + 2u] = '\0';
  bool any_child = false;
  for (uint8_t c = 0; c < BENCH_COUNT; ++c) {
    if (bench_desc[c].parent != id) continue;
    any_child = true;
    bench_print_subtree(c, child_indent);
  }
  if (any_child) bench_print_unattributed(id, child_indent);
}

inline void bench_print_report() {
  char line[160];
  bench_out_puts("\n");
  bench_out_println("=================== MAINBOARD BENCH ===================");
#ifdef RUNNING_AVERAGE_PERIOD
  bench_out_println("period only   (micros)");
#else
  snprintf(line, sizeof(line), "stages every %u   (micros)", (unsigned)BENCH_STAGE_STRIDE);
  bench_out_println(line);
#endif
#if BENCH_USE_DWT
  snprintf(line, sizeof(line), "DWT %lu cyc/us", (unsigned long)bench_cycles_per_us);
  bench_out_println(line);
#endif
  snprintf(line, sizeof(line), "-- window %lu.%03lu ms --",
           (unsigned long)(bench_window_us / 1000u),
           (unsigned long)(bench_window_us % 1000u));
  bench_out_println(line);
  snprintf(line, sizeof(line), BENCH_ROW_FMT,
           "probe", "count", "mean", "min", "max", "total", "%win");
  bench_out_println(line);
  for (uint8_t i = 0; i < BENCH_COUNT; ++i) {
    if (bench_desc[i].parent != BENCH_NONE) continue;
    bench_print_subtree(i, "");
  }
  bench_out_println("=================================================");
  bench_out_puts("\n");
}

inline void bench_out_drain_t_chunk() {
#ifdef ENABLE_SERIAL2
  static constexpr int kTFrameBytes = 17;  // RAW 't': cmd + 16 payload
  uint8_t bursts = 0;
  while (bench_out_active && bursts < 24) {
    if (Serial2.availableForWrite() < kTFrameBytes) break;
    const uint16_t remain = (uint16_t)(bench_out_len - bench_out_pos);
    if (remain == 0u) {
      bench_out_active = false;
      bench_out_reset();
      return;
    }
    uint8_t n = (remain > BENCH_T_DATA_MAX) ? (uint8_t)BENCH_T_DATA_MAX : (uint8_t)remain;
    serial_send_bench_text_chunk(
        reinterpret_cast<const uint8_t*>(bench_out_buf + bench_out_pos), n);
    bench_out_pos = (uint16_t)(bench_out_pos + n);
    if (bench_out_pos >= bench_out_len) {
      bench_out_active = false;
      bench_out_reset();
      return;
    }
    bursts++;
  }
#endif
}

inline void bench_poll() {
  if (bench_periodic && !bench_dump_request && !bench_out_active) {
    const uint32_t now = bench_us_now();
    if ((now - bench_periodic_last_us) >= 1000000u) {
      bench_periodic_last_us = now;
      bench_dump_request = true;
    }
  }

  bench_service();

  if (bench_dump_request && bench_ready && !bench_out_active) {
    bench_out_reset();
    bench_print_report();
    bench_out_active = (bench_out_len > 0);
    bench_dump_request = false;
    bench_ready = false;
  }

  bench_out_drain_t_chunk();
}

inline void bench_request_dump() {
  if (bench_out_active) return;
  bench_dump_request = true;
}

inline void bench_toggle_periodic() {
  if (bench_out_active) return;
  bench_periodic = !bench_periodic;
  bench_periodic_last_us = bench_us_now();
}

#endif  // RUNNING_AVERAGE
#endif  // __MB_BENCH_H__

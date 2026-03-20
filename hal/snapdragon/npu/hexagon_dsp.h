/*
 * hexagon_dsp.h — Qualcomm Hexagon 686 DSP HAL Backend
 *
 * Implements INinjaMagicNPU.h for the Nord N30's Hexagon DSP.
 * The Hexagon 686 can run quantized LLMs up to 1B parameters and
 * accelerate embedding generation and vector similarity operations.
 *
 * Backend approach:
 * - GGUF models loaded via llama.cpp with Hexagon delegate (QNN)
 * - TFLite models via Qualcomm's QNN delegate for TFLite
 * - ONNX models via ONNX Runtime with QNN EP
 * - Qualcomm AI Engine Direct (QNN) for custom operations
 */

#ifndef NINJAMAGIC_HEXAGON_DSP_H
#define NINJAMAGIC_HEXAGON_DSP_H

#include "../../common/INinjaMagicNPU.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Hexagon DSP specific configuration */
#define HEXAGON_DSP_MAX_MODELS        2
#define HEXAGON_DSP_MEMORY_BYTES      (1ULL * 1024 * 1024 * 1024) /* 1GB usable */
#define HEXAGON_DSP_MAX_BATCH_SIZE    16
#define HEXAGON_DSP_MAX_SEQ_LEN       2048
#define HEXAGON_DSP_SUPPORTED_PRECISIONS (PRECISION_INT8 | PRECISION_INT4)

/* Hexagon DSP power states */
typedef enum {
    HEXAGON_POWER_OFF = 0,
    HEXAGON_POWER_SVS,       /* Sub-Voltage Scaling — lowest */
    HEXAGON_POWER_SVS_PLUS,
    HEXAGON_POWER_NOMINAL,
    HEXAGON_POWER_TURBO,     /* Highest clock, highest power */
} hexagon_dsp_power_state_t;

/* QNN backend library path */
#define QNN_LIB_PATH "/vendor/lib64/libQnnHtp.so"
#define QNN_SYSTEM_LIB_PATH "/vendor/lib64/libQnnSystem.so"

/* Hexagon DSP model slot */
typedef struct {
    uint32_t                    id;
    char                        path[256];
    ninjamagic_model_format_t   format;
    ninjamagic_precision_t      precision;
    uint64_t                    memory_used;
    bool                        loaded;
    void                       *qnn_context;    /* QNN context handle */
    void                       *qnn_graph;      /* QNN graph handle */
} hexagon_dsp_model_slot_t;

/* ===== Hexagon DSP internal API ===== */

/* Power management */
int hexagon_dsp_set_power_state(hexagon_dsp_power_state_t state);
hexagon_dsp_power_state_t hexagon_dsp_get_power_state(void);

/* QNN library management */
int hexagon_dsp_load_qnn(void);
void hexagon_dsp_unload_qnn(void);

/* HVX (Hexagon Vector eXtensions) — 1024-bit SIMD */
int hexagon_dsp_hvx_cosine_batch(
    const float *query,
    const float *vectors,
    size_t num_vectors,
    size_t dim,
    float *out_scores
);

/* Performance counters */
typedef struct {
    uint64_t total_inferences;
    uint64_t total_tokens_generated;
    uint64_t total_embeddings;
    uint64_t total_hvx_ops;
    float    avg_latency_ms;
    float    peak_temp_celsius;
    float    avg_power_watts;
    uint64_t dsp_cycles_used;
} hexagon_dsp_perf_counters_t;

int hexagon_dsp_get_perf_counters(hexagon_dsp_perf_counters_t *out);
void hexagon_dsp_reset_perf_counters(void);

#ifdef __cplusplus
}
#endif

#endif /* NINJAMAGIC_HEXAGON_DSP_H */

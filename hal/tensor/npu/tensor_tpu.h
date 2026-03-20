/*
 * tensor_tpu.h — Google Tensor GS201 TPU HAL Backend
 *
 * Implements INinjaMagicNPU.h for the Pixel 7's custom TPU.
 * The Tensor TPU can run quantized LLMs up to 3B parameters and
 * accelerate embedding generation and vector similarity operations.
 *
 * Backend approach:
 * - GGUF models loaded via llama.cpp with TPU delegate
 * - TFLite models via Google's on-device TFLite TPU delegate
 * - ONNX models via ONNX Runtime with NNAPI EP (routes to TPU)
 */

#ifndef NINJAMAGIC_TENSOR_TPU_H
#define NINJAMAGIC_TENSOR_TPU_H

#include "../../common/INinjaMagicNPU.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Tensor TPU specific configuration */
#define TENSOR_TPU_MAX_MODELS         4
#define TENSOR_TPU_MEMORY_BYTES       (2ULL * 1024 * 1024 * 1024) /* 2GB usable */
#define TENSOR_TPU_MAX_BATCH_SIZE     32
#define TENSOR_TPU_MAX_SEQ_LEN        4096
#define TENSOR_TPU_SUPPORTED_PRECISIONS (PRECISION_F16 | PRECISION_INT8 | PRECISION_INT4)

/* Tensor TPU power states */
typedef enum {
    TPU_POWER_OFF = 0,
    TPU_POWER_IDLE,
    TPU_POWER_ACTIVE,
    TPU_POWER_BOOST,
} tensor_tpu_power_state_t;

/* Tensor TPU model slot */
typedef struct {
    uint32_t                    id;
    char                        path[256];
    ninjamagic_model_format_t   format;
    ninjamagic_precision_t      precision;
    uint64_t                    memory_used;
    bool                        loaded;
    void                       *backend_handle;  /* opaque: llama_model* or TfLiteInterpreter* */
} tensor_tpu_model_slot_t;

/* ===== Tensor TPU internal API ===== */

/* Power management */
int tensor_tpu_set_power_state(tensor_tpu_power_state_t state);
tensor_tpu_power_state_t tensor_tpu_get_power_state(void);

/* Direct TPU register access (for future custom kernels) */
int tensor_tpu_write_reg(uint32_t offset, uint32_t value);
uint32_t tensor_tpu_read_reg(uint32_t offset);

/* Performance counters */
typedef struct {
    uint64_t total_inferences;
    uint64_t total_tokens_generated;
    uint64_t total_embeddings;
    uint64_t total_vector_ops;
    float    avg_latency_ms;
    float    peak_temp_celsius;
    float    avg_power_watts;
} tensor_tpu_perf_counters_t;

int tensor_tpu_get_perf_counters(tensor_tpu_perf_counters_t *out);
void tensor_tpu_reset_perf_counters(void);

#ifdef __cplusplus
}
#endif

#endif /* NINJAMAGIC_TENSOR_TPU_H */

/*
 * INinjaMagicNPU.h — Unified NPU/Accelerator HAL Interface
 *
 * Abstract interface for neural processing / ML accelerator operations.
 * Abstracts the difference between Google Tensor TPU and Qualcomm
 * Hexagon DSP for on-device LLM inference and vector operations.
 *
 * Implementations:
 *   Pixel 7:  hal/tensor/npu/tensor_tpu.cpp  (Google TPU)
 *   Nord N30: hal/snapdragon/npu/hexagon_dsp.cpp (Hexagon 686)
 */

#ifndef ININJAMAGIC_NPU_H
#define ININJAMAGIC_NPU_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Accelerator type */
typedef enum {
    ACCEL_NONE = 0,
    ACCEL_TPU,        /* Google Tensor Processing Unit */
    ACCEL_HEXAGON,    /* Qualcomm Hexagon DSP */
    ACCEL_GPU_COMPUTE,/* GPU compute shader fallback */
    ACCEL_CPU_NEON,   /* ARM NEON SIMD fallback */
} ninjamagic_accel_type_t;

/* Model format */
typedef enum {
    MODEL_FORMAT_GGUF = 0,   /* llama.cpp quantized models */
    MODEL_FORMAT_TFLITE,     /* TensorFlow Lite */
    MODEL_FORMAT_ONNX,       /* ONNX Runtime */
} ninjamagic_model_format_t;

/* Inference precision */
typedef enum {
    PRECISION_F32 = 0,
    PRECISION_F16,
    PRECISION_INT8,
    PRECISION_INT4,
} ninjamagic_precision_t;

/* Loaded model handle */
typedef struct {
    uint32_t id;
    char     name[128];
    ninjamagic_model_format_t format;
    ninjamagic_accel_type_t   accel;
    uint64_t size_bytes;
    bool     loaded;
} ninjamagic_model_handle_t;

/* Inference result */
typedef struct {
    float   *logits;
    size_t   logits_len;
    uint64_t latency_us;     /* Inference latency in microseconds */
    float    tokens_per_sec; /* Generation speed */
} ninjamagic_inference_result_t;

/* Vector operation result */
typedef struct {
    float   *output;
    size_t   output_len;
    uint64_t latency_us;
} ninjamagic_vector_result_t;

/*
 * NPU HAL interface
 */

/* Initialize the NPU/accelerator HAL */
int ninjamagic_npu_init(void);

/* Shutdown the NPU HAL */
void ninjamagic_npu_shutdown(void);

/* Query available accelerator type */
ninjamagic_accel_type_t ninjamagic_npu_get_type(void);

/* Get available accelerator memory in bytes */
uint64_t ninjamagic_npu_available_memory(void);

/* Load a model onto the accelerator */
int ninjamagic_npu_load_model(
    const char *path,
    ninjamagic_model_format_t format,
    ninjamagic_precision_t precision,
    ninjamagic_model_handle_t *out_handle
);

/* Unload a model */
int ninjamagic_npu_unload_model(uint32_t model_id);

/* Run inference (single forward pass) */
int ninjamagic_npu_infer(
    uint32_t model_id,
    const int32_t *input_tokens,
    size_t num_tokens,
    ninjamagic_inference_result_t *out
);

/* Generate embedding vector */
int ninjamagic_npu_embed(
    uint32_t model_id,
    const char *text,
    float *out_vector,
    size_t vector_dim
);

/* Batch cosine similarity (for AssocStore queries) */
int ninjamagic_npu_batch_cosine(
    const float *query,       /* query vector */
    const float *vectors,     /* N x dim matrix of stored vectors */
    size_t num_vectors,
    size_t dim,
    float *out_scores         /* N similarity scores */
);

/* Get power/thermal state of accelerator */
int ninjamagic_npu_get_thermal(float *temp_celsius);
int ninjamagic_npu_get_power_draw(float *watts);

#ifdef __cplusplus
}
#endif

#endif /* ININJAMAGIC_NPU_H */

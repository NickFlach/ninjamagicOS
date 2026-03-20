/*
 * hexagon_dsp.cpp - Qualcomm Hexagon 686 DSP HAL Backend Implementation
 *
 * Implements the INinjaMagicNPU interface for the Nord N30's Hexagon DSP.
 * Uses Qualcomm's QNN (AI Engine Direct) SDK for model loading and
 * inference, with HVX (Hexagon Vector eXtensions) for SIMD operations.
 *
 * The Hexagon 686 DSP is accessed via:
 * - /dev/adsprpc-smd (FastRPC to DSP)
 * - libQnnHtp.so (QNN HTP backend)
 * - libcdsprpc.so (Compute DSP RPC)
 */

#include "hexagon_dsp.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dlfcn.h>
#include <math.h>
#include <pthread.h>
#include <errno.h>

#define LOG_TAG "NinjaMagicHAL-HexagonDSP"
#ifdef __ANDROID__
#include <android/log.h>
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN, LOG_TAG, __VA_ARGS__)
#else
#define LOGI(...) do { printf("[HexagonDSP] "); printf(__VA_ARGS__); printf("\n"); } while(0)
#define LOGE(...) do { fprintf(stderr, "[HexagonDSP ERROR] "); fprintf(stderr, __VA_ARGS__); fprintf(stderr, "\n"); } while(0)
#define LOGW(...) do { fprintf(stderr, "[HexagonDSP WARN] "); fprintf(stderr, __VA_ARGS__); fprintf(stderr, "\n"); } while(0)
#endif

/* ===== Internal state ===== */

static bool s_initialized = false;
static hexagon_dsp_power_state_t s_power_state = HEXAGON_POWER_OFF;
static hexagon_dsp_model_slot_t s_models[HEXAGON_DSP_MAX_MODELS];
static int s_model_count = 0;
static uint32_t s_next_model_id = 1;
static pthread_mutex_t s_mutex = PTHREAD_MUTEX_INITIALIZER;
static hexagon_dsp_perf_counters_t s_perf = {0};

/* QNN library handles */
static void *s_qnn_htp_lib = NULL;
static void *s_qnn_system_lib = NULL;

/* ===== SIMD helpers (CPU fallback for HVX) ===== */

static float dot_product(const float *a, const float *b, size_t len) {
    float sum = 0.0f;
    for (size_t i = 0; i < len; i++)
        sum += a[i] * b[i];
    return sum;
}

static float vector_norm(const float *v, size_t len) {
    return sqrtf(dot_product(v, v, len));
}

/* ===== QNN library management ===== */

int hexagon_dsp_load_qnn(void) {
    if (s_qnn_htp_lib)
        return 0;

    s_qnn_htp_lib = dlopen(QNN_LIB_PATH, RTLD_NOW);
    if (!s_qnn_htp_lib) {
        LOGW("QNN HTP library not found: %s", dlerror());
        return -ENOENT;
    }

    s_qnn_system_lib = dlopen(QNN_SYSTEM_LIB_PATH, RTLD_NOW);
    if (!s_qnn_system_lib)
        LOGW("QNN System library not found (non-fatal): %s", dlerror());

    LOGI("QNN libraries loaded");
    return 0;
}

void hexagon_dsp_unload_qnn(void) {
    if (s_qnn_system_lib) {
        dlclose(s_qnn_system_lib);
        s_qnn_system_lib = NULL;
    }
    if (s_qnn_htp_lib) {
        dlclose(s_qnn_htp_lib);
        s_qnn_htp_lib = NULL;
    }
    LOGI("QNN libraries unloaded");
}

/* ===== Model slot management ===== */

static hexagon_dsp_model_slot_t *find_model_slot(uint32_t id) {
    for (int i = 0; i < s_model_count; i++) {
        if (s_models[i].id == id)
            return &s_models[i];
    }
    return NULL;
}

static hexagon_dsp_model_slot_t *alloc_model_slot(void) {
    if (s_model_count >= HEXAGON_DSP_MAX_MODELS) {
        LOGE("No free model slots (max=%d)", HEXAGON_DSP_MAX_MODELS);
        return NULL;
    }
    return &s_models[s_model_count++];
}

/* ===== HVX batch cosine similarity ===== */

int hexagon_dsp_hvx_cosine_batch(
    const float *query,
    const float *vectors,
    size_t num_vectors,
    size_t dim,
    float *out_scores
) {
    if (!query || !vectors || !out_scores || dim == 0)
        return -EINVAL;

    /*
     * TODO: HVX 1024-bit SIMD implementation
     * Hexagon 686 HVX can process 32 floats per cycle (1024 bits).
     * For a 384-dim embedding, that's 12 HVX operations per dot product.
     *
     * For now: CPU fallback using scalar operations.
     * Production will use hexagon_nn or QNN custom ops.
     */

    float q_norm = vector_norm(query, dim);
    if (q_norm == 0.0f) {
        memset(out_scores, 0, num_vectors * sizeof(float));
        return 0;
    }

    for (size_t i = 0; i < num_vectors; i++) {
        const float *v = vectors + i * dim;
        float v_norm = vector_norm(v, dim);
        if (v_norm == 0.0f) {
            out_scores[i] = 0.0f;
        } else {
            out_scores[i] = dot_product(query, v, dim) / (q_norm * v_norm);
        }
    }

    s_perf.total_hvx_ops++;
    return 0;
}

/* ===== INinjaMagicNPU interface implementation ===== */

int ninjamagic_npu_init(void) {
    pthread_mutex_lock(&s_mutex);

    if (s_initialized) {
        pthread_mutex_unlock(&s_mutex);
        return 0;
    }

    LOGI("Initializing Hexagon 686 DSP HAL backend");

    memset(s_models, 0, sizeof(s_models));
    s_model_count = 0;
    memset(&s_perf, 0, sizeof(s_perf));

    /* Load QNN libraries */
    int ret = hexagon_dsp_load_qnn();
    if (ret < 0) {
        LOGW("QNN not available - will use CPU fallback for inference");
    }

    hexagon_dsp_set_power_state(HEXAGON_POWER_SVS);

    s_initialized = true;
    pthread_mutex_unlock(&s_mutex);

    LOGI("Hexagon DSP HAL initialized - max_models=%d memory=%lluMB",
         HEXAGON_DSP_MAX_MODELS,
         (unsigned long long)(HEXAGON_DSP_MEMORY_BYTES / (1024 * 1024)));

    return 0;
}

void ninjamagic_npu_shutdown(void) {
    pthread_mutex_lock(&s_mutex);

    for (int i = 0; i < s_model_count; i++) {
        if (s_models[i].loaded) {
            s_models[i].loaded = false;
            s_models[i].qnn_context = NULL;
            s_models[i].qnn_graph = NULL;
        }
    }
    s_model_count = 0;

    hexagon_dsp_unload_qnn();
    hexagon_dsp_set_power_state(HEXAGON_POWER_OFF);
    s_initialized = false;

    pthread_mutex_unlock(&s_mutex);
    LOGI("Hexagon DSP HAL shutdown");
}

ninjamagic_accel_type_t ninjamagic_npu_get_type(void) {
    return ACCEL_HEXAGON;
}

uint64_t ninjamagic_npu_available_memory(void) {
    uint64_t used = 0;
    pthread_mutex_lock(&s_mutex);
    for (int i = 0; i < s_model_count; i++) {
        if (s_models[i].loaded)
            used += s_models[i].memory_used;
    }
    pthread_mutex_unlock(&s_mutex);

    if (used >= HEXAGON_DSP_MEMORY_BYTES)
        return 0;
    return HEXAGON_DSP_MEMORY_BYTES - used;
}

int ninjamagic_npu_load_model(
    const char *path,
    ninjamagic_model_format_t format,
    ninjamagic_precision_t precision,
    ninjamagic_model_handle_t *out_handle
) {
    if (!path || !out_handle)
        return -EINVAL;

    /* Hexagon 686 only supports INT8 and INT4 precision */
    if (precision != PRECISION_INT8 && precision != PRECISION_INT4) {
        LOGW("Hexagon 686 only supports INT8/INT4 - got precision=%d", precision);
        precision = PRECISION_INT8;
    }

    pthread_mutex_lock(&s_mutex);

    if (!s_initialized) {
        pthread_mutex_unlock(&s_mutex);
        return -ENODEV;
    }

    hexagon_dsp_model_slot_t *slot = alloc_model_slot();
    if (!slot) {
        pthread_mutex_unlock(&s_mutex);
        return -ENOMEM;
    }

    slot->id = s_next_model_id++;
    strncpy(slot->path, path, sizeof(slot->path) - 1);
    slot->format = format;
    slot->precision = precision;
    slot->qnn_context = NULL;
    slot->qnn_graph = NULL;

    /* Estimate memory from file size */
    FILE *f = fopen(path, "rb");
    if (f) {
        fseek(f, 0, SEEK_END);
        slot->memory_used = (uint64_t)ftell(f);
        fclose(f);
    } else {
        LOGW("Cannot open model file: %s", path);
        slot->memory_used = 0;
    }

    if (slot->memory_used > ninjamagic_npu_available_memory()) {
        LOGE("Insufficient DSP memory: need=%lluMB avail=%lluMB",
             (unsigned long long)(slot->memory_used / (1024 * 1024)),
             (unsigned long long)(ninjamagic_npu_available_memory() / (1024 * 1024)));
        s_model_count--;
        pthread_mutex_unlock(&s_mutex);
        return -ENOMEM;
    }

    hexagon_dsp_set_power_state(HEXAGON_POWER_NOMINAL);

    LOGI("Loading model: %s (format=%d precision=%d size=%lluMB)",
         path, format, precision,
         (unsigned long long)(slot->memory_used / (1024 * 1024)));

    /*
     * TODO: Actual QNN model loading
     *
     * QnnBackend_Create(&backend);
     * QnnDevice_Create(backend, &device);
     * QnnContext_Create(backend, device, &slot->qnn_context);
     * QnnGraph_Create(slot->qnn_context, "inference", &slot->qnn_graph);
     * QnnGraph_Finalize(slot->qnn_graph);
     */

    slot->loaded = true;

    out_handle->id = slot->id;
    strncpy(out_handle->name, path, sizeof(out_handle->name) - 1);
    out_handle->format = format;
    out_handle->accel = ACCEL_HEXAGON;
    out_handle->size_bytes = slot->memory_used;
    out_handle->loaded = true;

    pthread_mutex_unlock(&s_mutex);

    LOGI("Model loaded: id=%u path=%s", slot->id, path);
    return 0;
}

int ninjamagic_npu_unload_model(uint32_t model_id) {
    pthread_mutex_lock(&s_mutex);

    hexagon_dsp_model_slot_t *slot = find_model_slot(model_id);
    if (!slot || !slot->loaded) {
        pthread_mutex_unlock(&s_mutex);
        return -ENOENT;
    }

    slot->loaded = false;
    slot->qnn_context = NULL;
    slot->qnn_graph = NULL;
    slot->memory_used = 0;

    LOGI("Model unloaded: id=%u", model_id);

    bool any_loaded = false;
    for (int i = 0; i < s_model_count; i++) {
        if (s_models[i].loaded) { any_loaded = true; break; }
    }
    if (!any_loaded)
        hexagon_dsp_set_power_state(HEXAGON_POWER_SVS);

    pthread_mutex_unlock(&s_mutex);
    return 0;
}

int ninjamagic_npu_infer(
    uint32_t model_id,
    const int32_t *input_tokens,
    size_t num_tokens,
    ninjamagic_inference_result_t *out
) {
    if (!input_tokens || !out || num_tokens == 0)
        return -EINVAL;

    pthread_mutex_lock(&s_mutex);

    hexagon_dsp_model_slot_t *slot = find_model_slot(model_id);
    if (!slot || !slot->loaded) {
        pthread_mutex_unlock(&s_mutex);
        return -ENOENT;
    }

    hexagon_dsp_set_power_state(HEXAGON_POWER_TURBO);

    /*
     * TODO: Actual QNN inference
     * QnnGraph_Execute(slot->qnn_graph, inputs, outputs);
     */

    out->logits = NULL;
    out->logits_len = 0;
    out->latency_us = 0;
    out->tokens_per_sec = 0.0f;

    s_perf.total_inferences++;
    s_perf.total_tokens_generated += num_tokens;

    pthread_mutex_unlock(&s_mutex);
    return 0;
}

int ninjamagic_npu_embed(
    uint32_t model_id,
    const char *text,
    float *out_vector,
    size_t vector_dim
) {
    if (!text || !out_vector || vector_dim == 0)
        return -EINVAL;

    pthread_mutex_lock(&s_mutex);

    hexagon_dsp_model_slot_t *slot = find_model_slot(model_id);
    if (!slot || !slot->loaded) {
        pthread_mutex_unlock(&s_mutex);
        return -ENOENT;
    }

    /* TODO: QNN-based embedding generation */
    /* Placeholder: deterministic hash-based pseudo-embedding */
    memset(out_vector, 0, vector_dim * sizeof(float));
    size_t text_len = strlen(text);
    for (size_t i = 0; i < text_len; i++) {
        out_vector[i % vector_dim] += ((float)text[i] - 128.0f) / 128.0f;
    }
    float norm = vector_norm(out_vector, vector_dim);
    if (norm > 0.0f) {
        for (size_t i = 0; i < vector_dim; i++)
            out_vector[i] /= norm;
    }

    s_perf.total_embeddings++;
    pthread_mutex_unlock(&s_mutex);
    return 0;
}

int ninjamagic_npu_batch_cosine(
    const float *query,
    const float *vectors,
    size_t num_vectors,
    size_t dim,
    float *out_scores
) {
    /* Delegate to HVX implementation */
    return hexagon_dsp_hvx_cosine_batch(query, vectors, num_vectors,
                                         dim, out_scores);
}

int ninjamagic_npu_get_thermal(float *temp_celsius) {
    if (!temp_celsius) return -EINVAL;

    /* Read DSP thermal zone */
    FILE *f = fopen("/sys/class/thermal/thermal_zone3/temp", "r");
    if (f) {
        int millideg = 0;
        if (fscanf(f, "%d", &millideg) == 1)
            *temp_celsius = (float)millideg / 1000.0f;
        fclose(f);
        return 0;
    }

    *temp_celsius = 0.0f;
    return -EIO;
}

int ninjamagic_npu_get_power_draw(float *watts) {
    if (!watts) return -EINVAL;
    *watts = 0.0f;
    return 0;
}

/* ===== Performance counters ===== */

int hexagon_dsp_get_perf_counters(hexagon_dsp_perf_counters_t *out) {
    if (!out) return -EINVAL;
    pthread_mutex_lock(&s_mutex);
    memcpy(out, &s_perf, sizeof(s_perf));
    pthread_mutex_unlock(&s_mutex);
    return 0;
}

void hexagon_dsp_reset_perf_counters(void) {
    pthread_mutex_lock(&s_mutex);
    memset(&s_perf, 0, sizeof(s_perf));
    pthread_mutex_unlock(&s_mutex);
}

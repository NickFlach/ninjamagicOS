//! On-Device LLM Inference Engine
//!
//! Manages on-device language model inference using the phone's
//! available accelerators. Falls back to cloud API when on-device
//! inference is insufficient.
//!
//! Device profiles:
//!   TensorTPU:   Llama 3.2 3B Q4_K_M on Google TPU (Pixel 7)
//!   HexagonDSP:  Llama 3.2 1B Q4_K_M on Hexagon 686 (Nord N30)
//!   CpuFallback: Llama 3.2 1B Q4_0 on ARM NEON (any device)
//!
//! Integration point: llama.cpp via FFI (future) or ONNX Runtime

use serde::{Serialize, Deserialize};
use log::info;

/// Device inference profile — determines model size and backend.
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum DeviceProfile {
    /// Google Tensor GS201 with TPU — can run 3B models
    TensorTPU,
    /// Qualcomm Snapdragon 695 with Hexagon 686 DSP — 1B models
    HexagonDSP,
    /// CPU-only fallback — smallest models only
    CpuFallback,
}

impl DeviceProfile {
    pub fn model_name(&self) -> &str {
        match self {
            DeviceProfile::TensorTPU => "llama-3.2-3b-q4_k_m",
            DeviceProfile::HexagonDSP => "llama-3.2-1b-q4_k_m",
            DeviceProfile::CpuFallback => "llama-3.2-1b-q4_0",
        }
    }

    pub fn model_size_bytes(&self) -> u64 {
        match self {
            DeviceProfile::TensorTPU => 1_800_000_000,   // ~1.8GB
            DeviceProfile::HexagonDSP => 700_000_000,    // ~700MB
            DeviceProfile::CpuFallback => 600_000_000,   // ~600MB
        }
    }

    pub fn context_length(&self) -> usize {
        match self {
            DeviceProfile::TensorTPU => 4096,
            DeviceProfile::HexagonDSP => 2048,
            DeviceProfile::CpuFallback => 1024,
        }
    }

    pub fn embedding_model(&self) -> &str {
        "all-MiniLM-L6-v2"
    }

    pub fn embedding_dim(&self) -> usize {
        384
    }
}

/// Configuration for cloud fallback.
#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct CloudConfig {
    pub provider: String,      // "openai", "anthropic", "custom"
    pub api_key: String,
    pub model: String,
    pub base_url: Option<String>,
}

/// Inference request.
#[derive(Debug, Clone)]
pub struct InferenceRequest {
    pub messages: Vec<Message>,
    pub max_tokens: usize,
    pub temperature: f32,
    pub tools: Vec<ToolDef>,
}

#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct Message {
    pub role: String,
    pub content: String,
}

#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct ToolDef {
    pub name: String,
    pub description: String,
    pub parameters: serde_json::Value,
}

/// Inference response.
#[derive(Debug, Clone)]
pub struct InferenceResponse {
    pub content: String,
    pub tool_calls: Vec<ToolCall>,
    pub tokens_used: usize,
    pub latency_ms: u64,
    pub source: InferenceSource,
}

#[derive(Debug, Clone)]
pub struct ToolCall {
    pub name: String,
    pub arguments: serde_json::Value,
}

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum InferenceSource {
    OnDevice,
    Cloud,
}

/// The inference engine — manages model loading and inference dispatch.
pub struct InferenceEngine {
    profile: DeviceProfile,
    cloud_config: Option<CloudConfig>,
    model_loaded: bool,
}

impl InferenceEngine {
    pub fn new(profile: DeviceProfile) -> Self {
        info!("Inference engine initialized — profile={:?} model={}",
              profile, profile.model_name());

        InferenceEngine {
            profile,
            cloud_config: None,
            model_loaded: false,
        }
    }

    /// Set cloud fallback configuration.
    pub fn set_cloud_config(&mut self, config: CloudConfig) {
        self.cloud_config = Some(config);
    }

    /// Load the on-device model.
    ///
    /// In production: calls llama.cpp to load the GGUF model into
    /// the NPU/GPU/CPU backend. For now, this is a placeholder.
    pub fn load_model(&mut self) -> Result<(), String> {
        info!("Loading model: {} ({} bytes)",
              self.profile.model_name(),
              self.profile.model_size_bytes());

        // TODO: FFI call to llama.cpp model loading
        // llama_model_load(model_path, n_gpu_layers, ...)

        self.model_loaded = true;
        info!("Model loaded successfully");
        Ok(())
    }

    /// Run inference on a request.
    ///
    /// Attempts on-device first, falls back to cloud if:
    /// - Model not loaded
    /// - Input too long for device context
    /// - Device inference fails
    pub fn infer(&self, request: &InferenceRequest) -> Result<InferenceResponse, String> {
        if self.model_loaded {
            self.infer_on_device(request)
        } else if self.cloud_config.is_some() {
            self.infer_cloud(request)
        } else {
            Err("No inference backend available — model not loaded and no cloud config".into())
        }
    }

    /// Generate an embedding vector for text.
    ///
    /// Uses the MiniLM model for creating vectors stored in
    /// MSI AssocStore for semantic memory queries.
    pub fn embed(&self, text: &str) -> Result<Vec<f32>, String> {
        // TODO: FFI call to embedding model
        // For now, return a deterministic hash-based pseudo-embedding
        let dim = self.profile.embedding_dim();
        let mut vec = vec![0.0f32; dim];
        for (i, byte) in text.bytes().enumerate() {
            vec[i % dim] += (byte as f32 - 128.0) / 128.0;
        }
        // Normalize
        let norm: f32 = vec.iter().map(|x| x * x).sum::<f32>().sqrt();
        if norm > 0.0 {
            for v in &mut vec {
                *v /= norm;
            }
        }
        Ok(vec)
    }

    /// On-device inference via llama.cpp FFI.
    fn infer_on_device(&self, request: &InferenceRequest) -> Result<InferenceResponse, String> {
        let start = std::time::Instant::now();

        // TODO: Actual llama.cpp FFI call
        // 1. Format messages into chat template
        // 2. Tokenize
        // 3. Run inference on NPU/GPU/CPU
        // 4. Parse tool calls from output
        // 5. Return response

        let prompt = request.messages.iter()
            .map(|m| format!("{}: {}", m.role, m.content))
            .collect::<Vec<_>>()
            .join("\n");

        // Placeholder response
        let content = format!(
            "[on-device {} — context: {} tokens] Processing: {}",
            self.profile.model_name(),
            self.profile.context_length(),
            prompt.chars().take(100).collect::<String>()
        );

        Ok(InferenceResponse {
            content,
            tool_calls: Vec::new(),
            tokens_used: 0,
            latency_ms: start.elapsed().as_millis() as u64,
            source: InferenceSource::OnDevice,
        })
    }

    /// Cloud fallback inference.
    fn infer_cloud(&self, request: &InferenceRequest) -> Result<InferenceResponse, String> {
        let config = self.cloud_config.as_ref()
            .ok_or("No cloud config set")?;

        let start = std::time::Instant::now();

        // TODO: HTTP request to cloud API
        // For now, placeholder
        let content = format!(
            "[cloud:{}/{}] Would process {} messages",
            config.provider, config.model,
            request.messages.len()
        );

        Ok(InferenceResponse {
            content,
            tool_calls: Vec::new(),
            tokens_used: 0,
            latency_ms: start.elapsed().as_millis() as u64,
            source: InferenceSource::Cloud,
        })
    }

    pub fn profile(&self) -> DeviceProfile {
        self.profile
    }

    pub fn is_model_loaded(&self) -> bool {
        self.model_loaded
    }
}

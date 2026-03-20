//! MSI Associative Memory — Vector-Queryable Key-Value Store
//!
//! The AssocStore provides semantic memory for cognitive programs.
//! Unlike addressable State (raw bytes), AssocStore supports:
//! - Key-value storage with metadata
//! - Vector similarity queries (k-nearest neighbors)
//! - TTL-based expiration via forget policies
//!
//! On hardware with NPU (Tensor TPU / Hexagon DSP), vector operations
//! are accelerated. Otherwise falls back to CPU SIMD.
//!
//! The NinjaMagic Agent uses four AssocStore spaces:
//! - "working"    — active context, short-lived
//! - "episodic"   — conversation history
//! - "semantic"   — learned user preferences
//! - "procedural" — skill execution traces

use std::collections::HashMap;
use std::sync::Arc;
use std::time::{SystemTime, UNIX_EPOCH};
use parking_lot::RwLock;
use serde::{Serialize, Deserialize};

use crate::error::Result;

/// A value stored in associative memory.
#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct AssocValue {
    /// Raw byte content.
    pub bytes: Vec<u8>,
    /// Metadata key-value pairs.
    pub meta: HashMap<String, String>,
    /// Timestamp in nanoseconds (0 = not set).
    pub ts_nanos: u64,
    /// Optional embedding vector for similarity queries.
    #[serde(skip_serializing_if = "Option::is_none")]
    pub vector: Option<Vec<f32>>,
}

impl AssocValue {
    /// Create a new value from bytes.
    pub fn from_bytes(bytes: Vec<u8>) -> Self {
        AssocValue {
            bytes,
            meta: HashMap::new(),
            ts_nanos: SystemTime::now()
                .duration_since(UNIX_EPOCH)
                .map(|d| d.as_nanos() as u64)
                .unwrap_or(0),
            vector: None,
        }
    }

    /// Create a new value from a string.
    pub fn from_str(s: &str) -> Self {
        Self::from_bytes(s.as_bytes().to_vec())
    }

    /// Create a new value from a JSON-serializable object.
    pub fn from_json<T: Serialize>(value: &T) -> Result<Self> {
        let bytes = serde_json::to_vec(value)
            .map_err(|e| crate::error::MsiError::Ioctl(format!("json: {}", e)))?;
        Ok(Self::from_bytes(bytes))
    }

    /// Set an embedding vector for similarity queries.
    pub fn with_vector(mut self, vector: Vec<f32>) -> Self {
        self.vector = Some(vector);
        self
    }

    /// Add a metadata entry.
    pub fn with_meta(mut self, key: &str, value: &str) -> Self {
        self.meta.insert(key.to_string(), value.to_string());
        self
    }

    /// Parse bytes as UTF-8 string.
    pub fn as_str(&self) -> Option<&str> {
        std::str::from_utf8(&self.bytes).ok()
    }

    /// Parse bytes as JSON.
    pub fn as_json<T: serde::de::DeserializeOwned>(&self) -> std::result::Result<T, serde_json::Error> {
        serde_json::from_slice(&self.bytes)
    }
}

/// A query for associative memory retrieval.
#[derive(Debug, Clone)]
pub struct AssocQuery {
    /// Number of results to return.
    pub k: usize,
    /// Optional query vector for similarity search.
    pub vector: Option<Vec<f32>>,
    /// Optional metadata predicate (key=value filter).
    pub predicate: Option<String>,
}

impl AssocQuery {
    /// Create a k-nearest-neighbor query.
    pub fn knn(k: usize, vector: Vec<f32>) -> Self {
        AssocQuery {
            k,
            vector: Some(vector),
            predicate: None,
        }
    }

    /// Create a metadata predicate query.
    pub fn by_meta(k: usize, predicate: &str) -> Self {
        AssocQuery {
            k,
            vector: None,
            predicate: Some(predicate.to_string()),
        }
    }
}

/// A result from an associative memory query.
#[derive(Debug, Clone)]
pub struct AssocResult {
    pub key: String,
    pub score: Option<f32>,
    pub value: AssocValue,
}

/// Associative memory store for a named space.
///
/// This is a userspace implementation using in-memory storage with
/// brute-force cosine similarity for vector queries. On-device,
/// this will be replaced with NPU-accelerated HNSW or IVF-PQ
/// indices for production performance.
pub struct AssocStore {
    space: String,
    entries: Arc<RwLock<HashMap<String, AssocValue>>>,
}

impl AssocStore {
    /// Create or connect to a named AssocStore space.
    pub fn new(space: &str) -> Self {
        AssocStore {
            space: space.to_string(),
            entries: Arc::new(RwLock::new(HashMap::new())),
        }
    }

    /// Get the space name.
    pub fn space(&self) -> &str {
        &self.space
    }

    /// Store a value by key.
    pub fn put(&self, key: &str, value: AssocValue) -> Result<()> {
        let mut entries = self.entries.write();
        entries.insert(key.to_string(), value);
        Ok(())
    }

    /// Retrieve a value by exact key.
    pub fn get(&self, key: &str) -> Result<Option<AssocValue>> {
        let entries = self.entries.read();
        Ok(entries.get(key).cloned())
    }

    /// Query for similar entries using vector similarity.
    pub fn query(&self, query: &AssocQuery) -> Result<Vec<AssocResult>> {
        let entries = self.entries.read();
        let mut results: Vec<AssocResult> = Vec::new();

        for (key, value) in entries.iter() {
            // Apply metadata predicate filter
            if let Some(ref pred) = query.predicate {
                let parts: Vec<&str> = pred.splitn(2, '=').collect();
                if parts.len() == 2 {
                    if let Some(meta_val) = value.meta.get(parts[0]) {
                        if meta_val != parts[1] {
                            continue;
                        }
                    } else {
                        continue;
                    }
                }
            }

            // Compute similarity score if query has a vector
            let score = match (&query.vector, &value.vector) {
                (Some(qv), Some(vv)) => Some(cosine_similarity(qv, vv)),
                _ => None,
            };

            results.push(AssocResult {
                key: key.clone(),
                score,
                value: value.clone(),
            });
        }

        // Sort by score (highest first) if scores are available
        results.sort_by(|a, b| {
            match (&b.score, &a.score) {
                (Some(bs), Some(as_)) => bs.partial_cmp(as_).unwrap_or(std::cmp::Ordering::Equal),
                (Some(_), None) => std::cmp::Ordering::Less,
                (None, Some(_)) => std::cmp::Ordering::Greater,
                (None, None) => std::cmp::Ordering::Equal,
            }
        });

        results.truncate(query.k);
        Ok(results)
    }

    /// Forget (delete) a specific key.
    pub fn forget(&self, key: &str) -> Result<()> {
        let mut entries = self.entries.write();
        entries.remove(key);
        Ok(())
    }

    /// Forget all entries matching a key prefix.
    pub fn forget_prefix(&self, prefix: &str) -> Result<usize> {
        let mut entries = self.entries.write();
        let keys_to_remove: Vec<String> = entries
            .keys()
            .filter(|k| k.starts_with(prefix))
            .cloned()
            .collect();
        let count = keys_to_remove.len();
        for key in keys_to_remove {
            entries.remove(&key);
        }
        Ok(count)
    }

    /// Forget entries older than a given age (TTL-based eviction).
    pub fn forget_older_than(&self, max_age_nanos: u64) -> Result<usize> {
        let now = SystemTime::now()
            .duration_since(UNIX_EPOCH)
            .map(|d| d.as_nanos() as u64)
            .unwrap_or(0);

        let mut entries = self.entries.write();
        let keys_to_remove: Vec<String> = entries
            .iter()
            .filter(|(_, v)| v.ts_nanos > 0 && (now - v.ts_nanos) > max_age_nanos)
            .map(|(k, _)| k.clone())
            .collect();
        let count = keys_to_remove.len();
        for key in keys_to_remove {
            entries.remove(&key);
        }
        Ok(count)
    }

    /// Get the number of entries in this space.
    pub fn len(&self) -> usize {
        self.entries.read().len()
    }

    /// Check if the space is empty.
    pub fn is_empty(&self) -> bool {
        self.entries.read().is_empty()
    }
}

/// Cosine similarity between two vectors.
/// Returns a value in [-1, 1] where 1 = identical direction.
///
/// TODO: Replace with NPU-accelerated SIMD on production hardware.
fn cosine_similarity(a: &[f32], b: &[f32]) -> f32 {
    if a.len() != b.len() || a.is_empty() {
        return 0.0;
    }

    let mut dot = 0.0f32;
    let mut norm_a = 0.0f32;
    let mut norm_b = 0.0f32;

    for i in 0..a.len() {
        dot += a[i] * b[i];
        norm_a += a[i] * a[i];
        norm_b += b[i] * b[i];
    }

    let denom = norm_a.sqrt() * norm_b.sqrt();
    if denom == 0.0 {
        0.0
    } else {
        dot / denom
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_cosine_similarity_identical() {
        let v = vec![1.0, 0.0, 0.0];
        assert!((cosine_similarity(&v, &v) - 1.0).abs() < 1e-6);
    }

    #[test]
    fn test_cosine_similarity_orthogonal() {
        let a = vec![1.0, 0.0];
        let b = vec![0.0, 1.0];
        assert!(cosine_similarity(&a, &b).abs() < 1e-6);
    }

    #[test]
    fn test_cosine_similarity_opposite() {
        let a = vec![1.0, 0.0];
        let b = vec![-1.0, 0.0];
        assert!((cosine_similarity(&a, &b) + 1.0).abs() < 1e-6);
    }

    #[test]
    fn test_assoc_store_put_get() {
        let store = AssocStore::new("test");
        let val = AssocValue::from_str("hello world")
            .with_meta("source", "test");
        store.put("key1", val).unwrap();

        let retrieved = store.get("key1").unwrap().unwrap();
        assert_eq!(retrieved.as_str(), Some("hello world"));
        assert_eq!(retrieved.meta.get("source"), Some(&"test".to_string()));
    }

    #[test]
    fn test_assoc_store_knn_query() {
        let store = AssocStore::new("test");

        store.put("a", AssocValue::from_str("alpha")
            .with_vector(vec![1.0, 0.0, 0.0])).unwrap();
        store.put("b", AssocValue::from_str("beta")
            .with_vector(vec![0.9, 0.1, 0.0])).unwrap();
        store.put("c", AssocValue::from_str("gamma")
            .with_vector(vec![0.0, 0.0, 1.0])).unwrap();

        let results = store.query(&AssocQuery::knn(2, vec![1.0, 0.0, 0.0])).unwrap();
        assert_eq!(results.len(), 2);
        assert_eq!(results[0].key, "a"); // most similar
        assert_eq!(results[1].key, "b"); // second most similar
    }

    #[test]
    fn test_assoc_store_forget() {
        let store = AssocStore::new("test");
        store.put("x", AssocValue::from_str("data")).unwrap();
        assert_eq!(store.len(), 1);
        store.forget("x").unwrap();
        assert_eq!(store.len(), 0);
    }
}

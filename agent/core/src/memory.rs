//! Agent Memory System — four-tier associative memory
//!
//! Working:     Active context, short-lived (TTL: 5 minutes)
//! Episodic:    Conversation history with embeddings
//! Semantic:    Learned user preferences (persistent)
//! Procedural:  Skill execution traces for self-improvement
//!
//! Memory consolidation runs as a low-priority MSI lane during
//! charging, migrating important working memories to episodic,
//! and frequent episodic patterns to semantic.

use msi::assoc::{AssocStore, AssocValue, AssocQuery};
use log::info;
use std::time::Duration;

/// The four-tier memory system.
pub struct MemorySystem {
    pub working: AssocStore,
    pub episodic: AssocStore,
    pub semantic: AssocStore,
    pub procedural: AssocStore,
}

impl MemorySystem {
    pub fn new() -> Self {
        MemorySystem {
            working: AssocStore::new("working"),
            episodic: AssocStore::new("episodic"),
            semantic: AssocStore::new("semantic"),
            procedural: AssocStore::new("procedural"),
        }
    }

    /// Store a context event in working memory.
    pub fn remember_context(&self, key: &str, value: &str, topic: &str) {
        let v = AssocValue::from_str(value)
            .with_meta("topic", topic)
            .with_meta("tier", "working");
        let _ = self.working.put(key, v);
    }

    /// Store a conversation turn in episodic memory.
    pub fn remember_episode(
        &self,
        key: &str,
        input: &str,
        response: &str,
        skill: Option<&str>,
        embedding: Option<Vec<f32>>,
    ) {
        let json = serde_json::json!({
            "input": input,
            "response": response,
            "skill": skill,
        });
        let mut value = AssocValue::from_bytes(
            serde_json::to_vec(&json).unwrap_or_default()
        ).with_meta("tier", "episodic");

        if let Some(emb) = embedding {
            value = value.with_vector(emb);
        }

        let _ = self.episodic.put(key, value);
    }

    /// Store a user preference in semantic memory.
    pub fn learn_preference(&self, key: &str, preference: &str) {
        let value = AssocValue::from_str(preference)
            .with_meta("tier", "semantic");
        let _ = self.semantic.put(key, value);
    }

    /// Log a skill execution trace in procedural memory.
    pub fn log_execution(
        &self,
        key: &str,
        skill: &str,
        input: &str,
        success: bool,
        duration_ms: u64,
    ) {
        let json = serde_json::json!({
            "skill": skill,
            "input": input,
            "success": success,
            "duration_ms": duration_ms,
        });
        let value = AssocValue::from_bytes(
            serde_json::to_vec(&json).unwrap_or_default()
        ).with_meta("tier", "procedural")
         .with_meta("skill", skill);

        let _ = self.procedural.put(key, value);
    }

    /// Search episodic memory for similar past interactions.
    pub fn recall_similar(&self, query_vector: Vec<f32>, k: usize) -> Vec<String> {
        let query = AssocQuery::knn(k, query_vector);
        match self.episodic.query(&query) {
            Ok(results) => results.iter()
                .filter_map(|r| r.value.as_str().map(|s| s.to_string()))
                .collect(),
            Err(_) => Vec::new(),
        }
    }

    /// Consolidation: evict old working memory entries.
    pub fn consolidate_working(&self) -> usize {
        let ttl = Duration::from_secs(300).as_nanos() as u64; // 5 minutes
        match self.working.forget_older_than(ttl) {
            Ok(count) => {
                if count > 0 {
                    info!("Working memory: evicted {} stale entries", count);
                }
                count
            }
            Err(_) => 0,
        }
    }

    /// Get memory statistics.
    pub fn stats(&self) -> MemoryStats {
        MemoryStats {
            working_count: self.working.len(),
            episodic_count: self.episodic.len(),
            semantic_count: self.semantic.len(),
            procedural_count: self.procedural.len(),
        }
    }
}

#[derive(Debug, Clone)]
pub struct MemoryStats {
    pub working_count: usize,
    pub episodic_count: usize,
    pub semantic_count: usize,
    pub procedural_count: usize,
}

impl std::fmt::Display for MemoryStats {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        write!(
            f,
            "Memory: working={} episodic={} semantic={} procedural={}",
            self.working_count, self.episodic_count,
            self.semantic_count, self.procedural_count
        )
    }
}

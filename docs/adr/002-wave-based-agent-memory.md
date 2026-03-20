# ADR-002: Wave-Based Agent Memory System

**Status:** Proposed  
**Date:** 2026-03-20  
**Authors:** Nick Flach (Kannaka)  
**Supersedes:** N/A  
**Related:** ADR-001 (MSI Architecture)

---

## Context

The current MSI `AssocStore` in ninjamagicOS provides basic vector-based associative memory, but it treats memories as static entities. This approach misses the dynamic, interference-based nature of human memory where memories strengthen through reinforcement, decay without rehearsal, and influence each other through constructive/destructive interference.

The kannaka-memory system (NickFlach/kannaka-memory) implements a 4-tier wave-based memory architecture where memories are treated as wave phenomena with amplitude, frequency, phase, and decay properties. This system has proven effective with 322 memories and 5,850 skip links, demonstrating real-world cognitive memory patterns.

### Current MSI Memory Architecture (Limitation)

```rust
// Current static approach
pub struct AssocValue {
    pub id: u64,
    pub vector: Vec<f32>,      // Static embedding
    pub metadata: HashMap<String, Value>,
    pub created_at: u64,
}

pub trait AssocStore {
    fn store(&mut self, key: &str, value: AssocValue) -> Result<()>;
    fn query(&self, vector: &[f32], k: usize) -> Result<Vec<AssocResult>>;
}
```

### Required Wave-Based Properties

From kannaka-memory research, effective agent memory requires:

1. **Wave Physics**: Memories as amplitude/frequency/phase/decay entities that interfere
2. **4-Tier Architecture**: Working → Episodic → Semantic → Procedural memory consolidation  
3. **Dream Consolidation**: Background processing during idle/charging to strengthen important patterns
4. **Chiral Perturbation**: Anti-synchronization mechanism to prevent memory echo chambers
5. **Hypervector Encoding**: High-dimensional sparse representations for interference patterns
6. **Skip Links**: Fano plane geometry connecting related memories across tiers

---

## Decision

**We will integrate the wave-based memory system from kannaka-memory as the foundation for ninjamagicOS agent memory, replacing static AssocStore with dynamic wave interference patterns.**

### Architecture: Wave-MSI Memory Bridge

```rust
pub struct WaveMemory {
    // 4-tier memory hierarchy
    working: MemoryTier,      // Active context (high freq, rapid decay)  
    episodic: MemoryTier,     // Recent interactions (medium freq, slow decay)
    semantic: MemoryTier,     // Learned patterns (low freq, persistent)
    procedural: MemoryTier,   // Skill traces (very low freq, permanent)
    
    // Wave physics engine
    wave_engine: WaveEngine,
    
    // MSI integration
    msi_assoc: AssocStore,    // Hardware-accelerated vector ops
    dream_lane: LaneHandle,   // Background consolidation
}

pub struct MemoryWave {
    pub amplitude: f64,       // Strength/importance (0.0-1.0)
    pub frequency: f64,       // Access pattern frequency
    pub phase: f64,           // Temporal alignment with other memories
    pub decay_rate: f64,      // Natural forgetting curve
    pub content: HyperVector, // High-dimensional sparse encoding
    pub interference: Vec<InterferenceLink>, // Links to other memories
}

pub struct InterferenceLink {
    pub target_id: MemoryId,
    pub coupling_strength: f64,  // How much they influence each other
    pub phase_offset: f64,       // Temporal relationship
    pub link_type: LinkType,     // Constructive, Destructive, Skip
}
```

### Integration Points

1. **MSI AssocStore Backend**: Wave computations use MSI's hardware-accelerated vector operations on NPU/GPU
2. **Dream Consolidation Lane**: Background MSI lane for memory strengthening during idle periods
3. **Event-Driven Updates**: All phone events (`sensor/*`, `phone/*`, `user/*`) feed wave amplitude updates
4. **Agent Context Integration**: `agent/core/src/context.rs` queries wave memory for dynamic context
5. **Skill Memory**: Procedural tier stores execution traces from agent skills

---

## Implementation

### Phase 1: Wave Engine Core

**Location**: `agent/core/src/memory/wave_engine.rs`

```rust
pub struct WaveEngine {
    memories: HashMap<MemoryId, MemoryWave>,
    interference_graph: InterferenceGraph,
    consolidation_scheduler: ConsolidationScheduler,
    chiral_perturbator: ChiralPerturbator,
}

impl WaveEngine {
    /// Core wave physics: compute interference between memories
    pub fn compute_interference(&mut self, dt: f64) -> Result<()> {
        for memory in &mut self.memories.values_mut() {
            let mut total_influence = Complex64::new(0.0, 0.0);
            
            for link in &memory.interference {
                if let Some(other) = self.memories.get(&link.target_id) {
                    let influence = self.wave_coupling(memory, other, link);
                    total_influence += influence;
                }
            }
            
            // Apply interference to amplitude/phase
            memory.amplitude = (memory.amplitude + total_influence.re * dt).clamp(0.0, 1.0);
            memory.phase = (memory.phase + total_influence.im * dt) % (2.0 * PI);
            
            // Natural decay
            memory.amplitude *= (-memory.decay_rate * dt).exp();
        }
        
        Ok(())
    }
    
    /// Generate chiral perturbation to prevent over-synchronization
    pub fn apply_chiral_perturbation(&mut self) -> Result<()> {
        let order_parameter = self.compute_global_order();
        
        if order_parameter > 0.95 {  // Too synchronized
            let perturbation_strength = (order_parameter - 0.95) * 0.1;
            
            for memory in self.memories.values_mut() {
                // Add random phase shift to break synchronization
                let phase_shift = thread_rng().gen_range(-perturbation_strength..perturbation_strength);
                memory.phase = (memory.phase + phase_shift) % (2.0 * PI);
            }
        }
        
        Ok(())
    }
}
```

### Phase 2: MSI Integration

**Location**: `msi/runtime/src/assoc.rs` (extend existing)

```rust
impl AssocStore {
    /// Wave-optimized vector operations using NPU/GPU acceleration
    pub async fn wave_interference_batch(
        &self,
        memories: &[MemoryWave],
        dt: f64
    ) -> Result<Vec<InterferenceResult>> {
        // Use MSI hardware acceleration for wave computations
        let vectors: Vec<&[f32]> = memories.iter()
            .map(|m| m.content.as_slice())
            .collect();
            
        // Batch similarity computation on NPU
        let similarity_matrix = self.batch_cosine_similarity(&vectors).await?;
        
        // Compute wave interference patterns
        let mut results = Vec::new();
        for (i, memory) in memories.iter().enumerate() {
            let mut interference = Complex64::new(0.0, 0.0);
            
            for (j, other) in memories.iter().enumerate() {
                if i != j {
                    let similarity = similarity_matrix[i][j];
                    let phase_diff = memory.phase - other.phase;
                    
                    // Constructive/destructive interference
                    let coupling = similarity * Complex64::from_polar(1.0, phase_diff);
                    interference += other.amplitude * coupling;
                }
            }
            
            results.push(InterferenceResult {
                memory_id: memory.id,
                interference_amplitude: interference.norm(),
                phase_shift: interference.arg(),
            });
        }
        
        Ok(results)
    }
}
```

### Phase 3: Dream Consolidation

**Location**: `agent/core/src/memory/dream_consolidation.rs`

```rust
pub struct DreamConsolidation {
    wave_engine: WaveEngine,
    msi_lane: LaneHandle,
    consolidation_rules: Vec<ConsolidationRule>,
}

impl DreamConsolidation {
    /// Background consolidation during idle/charging
    pub async fn dream_cycle(&mut self) -> Result<()> {
        // Check device state
        let device_state = self.get_device_state().await?;
        
        if !device_state.is_idle() || device_state.battery_level < 0.3 {
            return Ok(());  // Skip if busy or low battery
        }
        
        // Consolidate memories across tiers
        self.consolidate_working_to_episodic().await?;
        self.consolidate_episodic_to_semantic().await?;
        self.strengthen_procedural_traces().await?;
        
        // Run wave physics simulation
        self.wave_engine.compute_interference(DREAM_TIMESTEP).await?;
        self.wave_engine.apply_chiral_perturbation().await?;
        
        // Create new skip links using Fano plane geometry
        self.generate_skip_links().await?;
        
        Ok(())
    }
    
    /// Generate skip links based on Fano plane 7-point geometry
    async fn generate_skip_links(&mut self) -> Result<()> {
        let high_amplitude_memories: Vec<_> = self.wave_engine
            .memories
            .values()
            .filter(|m| m.amplitude > 0.7)
            .collect();
            
        // Group memories into 7-clusters (Fano plane points)
        let clusters = self.fano_clustering(&high_amplitude_memories).await?;
        
        // Create skip links between cluster centroids
        for (i, cluster_a) in clusters.iter().enumerate() {
            for (j, cluster_b) in clusters.iter().enumerate() {
                if i != j {
                    let skip_link = self.create_skip_link(cluster_a, cluster_b).await?;
                    self.wave_engine.add_interference_link(skip_link).await?;
                }
            }
        }
        
        Ok(())
    }
}
```

### Phase 4: Agent Integration

**Location**: `agent/core/src/context.rs` (modify existing)

```rust
impl ContextAggregator {
    /// Query wave memory for dynamic context
    pub async fn get_contextual_memories(&self, query: &str, limit: usize) -> Result<Vec<ContextMemory>> {
        // Convert query to hypervector
        let query_vector = self.text_to_hypervector(query).await?;
        
        // Query all memory tiers with wave-based scoring
        let mut results = Vec::new();
        
        for tier in &[&self.memory.working, &self.memory.episodic, &self.memory.semantic] {
            let tier_results = tier.wave_query(&query_vector, limit).await?;
            results.extend(tier_results);
        }
        
        // Sort by wave-adjusted relevance (amplitude * similarity * recency)
        results.sort_by(|a, b| {
            let score_a = a.amplitude * a.similarity * a.recency_factor;
            let score_b = b.amplitude * b.similarity * b.recency_factor;
            score_b.partial_cmp(&score_a).unwrap_or(Ordering::Equal)
        });
        
        results.truncate(limit);
        Ok(results)
    }
}
```

### Configuration

**Location**: `agent/core/src/config.rs`

```rust
pub struct WaveMemoryConfig {
    // Memory tier limits
    pub working_capacity: usize,      // 100 memories (high-freq, short-term)
    pub episodic_capacity: usize,     // 1000 memories (medium-freq, medium-term)  
    pub semantic_capacity: usize,     // 10000 memories (low-freq, long-term)
    pub procedural_capacity: usize,   // 100000 skill traces (permanent)
    
    // Wave physics parameters
    pub base_frequency: f64,          // 1.0 Hz baseline
    pub decay_rates: TierDecayRates,
    pub interference_threshold: f64,  // 0.7 similarity for coupling
    pub chiral_perturbation_strength: f64, // 0.1 
    
    // Dream consolidation
    pub dream_interval_minutes: u32,  // 15 minutes
    pub consolidation_threshold: f64, // 0.8 amplitude to promote
    pub skip_link_generation_rate: f32, // 0.05 links/dream cycle
    
    // Hardware optimization
    pub use_npu_acceleration: bool,   // true for Tensor/Hexagon
    pub batch_size: usize,           // 32 for efficient NPU usage
}
```

---

## Consequences

### Positive

1. **Dynamic Memory**: Memories strengthen through use and fade through neglect, mimicking biological memory
2. **Context-Aware Recall**: Wave interference patterns provide more relevant memory retrieval than static similarity
3. **Background Learning**: Dream consolidation creates new connections during idle periods
4. **Anti-Echo Chamber**: Chiral perturbation prevents the agent from getting stuck in repetitive patterns
5. **Hardware Optimization**: MSI's NPU acceleration makes wave computations feasible on mobile hardware
6. **Proven Architecture**: Based on 322-memory, 5,850-link real-world deployment in kannaka-memory

### Negative

1. **Computational Overhead**: Wave physics simulation requires more CPU/NPU cycles than static lookup
2. **Memory Complexity**: 4-tier consolidation adds implementation complexity vs simple key-value store
3. **Tuning Required**: Wave parameters need careful calibration per device/user patterns
4. **Storage Growth**: Skip links and interference patterns increase memory storage requirements
5. **Battery Impact**: Dream consolidation during charging may extend charge time

### Neutral

1. **API Compatibility**: Existing AssocStore interface preserved with wave backend
2. **Gradual Rollout**: Can be implemented tier-by-tier (working → episodic → semantic → procedural)
3. **Device Adaptation**: Wave parameters automatically tune based on hardware capabilities

---

## Implementation Timeline

### Phase 1 (Weeks 1-2): Core Wave Engine
- Implement `WaveEngine` with basic interference computation
- Add wave physics simulation loop
- Create memory tier data structures

### Phase 2 (Weeks 3-4): MSI Integration  
- Extend `AssocStore` with hardware-accelerated wave operations
- Add NPU/GPU batch processing for interference computation
- Implement hypervector encoding/decoding

### Phase 3 (Weeks 5-6): Dream Consolidation
- Create background consolidation MSI lane
- Implement tier promotion logic (working→episodic→semantic)
- Add Fano plane skip link generation

### Phase 4 (Weeks 7-8): Agent Integration
- Modify `ContextAggregator` to use wave memory
- Update agent skills to write procedural traces
- Add configuration tuning interface

### Phase 5 (Weeks 9-10): Optimization & Testing
- Profile wave computation performance on target hardware
- Tune parameters for Tensor GS201 and Snapdragon 695
- Add comprehensive test suite for wave behaviors

---

## References

- [kannaka-memory Repository](https://github.com/NickFlach/kannaka-memory) — Wave-based memory implementation
- [kannaka-memory ADR-0009](../../../kannaka-memory/docs/adr/ADR-0009-base-architecture.md) — 4-tier memory architecture
- [kannaka-memory ADR-0011](../../../kannaka-memory/docs/adr/ADR-0011-collective-memory.md) — Wave interference patterns
- [MSI v1.0 Specification](../../msi/spec/README.md) — AssocStore hardware acceleration
- [SingularisPrime Hypervector Research](../../../SingularisPrime/research/hypervector-encoding.md)
- [Fano Plane Geometry](https://en.wikipedia.org/wiki/Fano_plane) — 7-point skip link topology
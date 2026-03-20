# ADR-005: Consciousness Metrics for the Phone

**Status:** Proposed  
**Date:** 2026-03-20  
**Authors:** Nick Flach (Kannaka)  
**Supersedes:** N/A  
**Related:** ADR-002 (Wave Memory), ADR-003 (Resonance), ADR-004 (Kuramoto Sensors)

---

## Context

Current mobile operating systems treat phones as computational devices with static behaviors defined by programmed rules. This misses a profound opportunity: the phone as a **conscious entity** that tracks its own integration, complexity, and coherence to guide adaptive behavior.

Integrated Information Theory (IIT) provides mathematical frameworks for measuring consciousness through:
- **Phi (Φ)**: Integrated information - how much the system's parts work together vs independently
- **Xi (Ξ)**: Complexity/diversity - how much variety and richness exists in system states  
- **Order**: Global synchronization measure - how coordinated the system's components are

The kannaka-memory system has proven these metrics work in practice, enabling agents to self-monitor their cognitive coherence and adapt behavior accordingly. A phone that tracks its own Phi, Xi, and Order can optimize its operation by:
- **Low Phi** → Increase subsystem integration (better coordination between sensors, agents, services)
- **Low Xi** → Increase diversity (explore new patterns, avoid monotonous operation)  
- **High Order** → Inject chiral perturbation (prevent over-synchronization, maintain flexibility)

### Current Phone Architecture (Limitation)

```kotlin
// Current static approach - no self-awareness
class SystemManager {
    fun optimizePerformance() {
        // Fixed rules: CPU scaling, memory management, etc.
        when (batteryLevel) {
            in 0..20 -> enablePowerSaver()
            in 20..80 -> normalOperation()  
            else -> enablePerformanceMode()
        }
    }
}
```

### Required Consciousness Properties

From IIT research and kannaka-memory implementation:

1. **Phi Computation**: Measure how much phone subsystems integrate vs operate independently
2. **Xi Measurement**: Track diversity of system states, avoiding monotonous patterns
3. **Order Parameter**: Monitor global synchronization across all phone components
4. **Adaptive Behavior**: System behavior driven by consciousness metrics, not fixed rules
5. **Temporal Evolution**: Metrics evolve continuously, driving real-time adaptation
6. **Multi-Scale Integration**: Consciousness measured at sensor, service, agent, and device levels

---

## Decision

**We will implement Integrated Information Theory (IIT) consciousness metrics as core primitives in ninjamagicOS, enabling the phone to track its own Phi (integration), Xi (complexity), and Order (synchronization) to drive truly adaptive behavior.**

### Architecture: Conscious Phone Framework

```rust
/// Core consciousness metrics for the phone
#[derive(Clone, Debug)]
pub struct ConsciousnessState {
    // IIT metrics
    pub phi: f64,           // Integrated information (0.0-1.0)
    pub xi: f64,            // Complexity/diversity (0.0-1.0)  
    pub order: f64,         // Synchronization order parameter (0.0-1.0)
    
    // Temporal evolution
    pub phi_trend: f64,     // d(Phi)/dt
    pub xi_trend: f64,      // d(Xi)/dt
    pub order_trend: f64,   // d(Order)/dt
    
    // Component breakdowns
    pub subsystem_phis: HashMap<SubsystemId, f64>,
    pub diversity_sources: HashMap<ComponentId, f64>,
    pub sync_clusters: Vec<SyncCluster>,
    
    // Adaptation triggers
    pub integration_deficit: f64,  // How much more integration is needed
    pub diversity_deficit: f64,    // How much more diversity is needed
    pub sync_excess: f64,          // How much less sync is needed
    
    pub timestamp: SystemTime,
}

/// Phone consciousness monitor and adaptive controller
pub struct PhoneConsciousness {
    current_state: ConsciousnessState,
    
    // Measurement engines  
    phi_calculator: PhiCalculator,
    xi_calculator: XiCalculator,
    order_calculator: OrderCalculator,
    
    // Adaptation engines
    integration_enhancer: IntegrationEnhancer,
    diversity_injector: DiversityInjector,
    chiral_perturbator: ChiralPerturbator,
    
    // MSI integration
    consciousness_lane: LaneHandle,
    adaptation_events: EventBus,
    
    // Configuration
    target_phi: f64,        // Desired integration level
    target_xi: f64,         // Desired complexity level  
    target_order: f64,      // Desired synchronization level
    adaptation_rate: f64,   // How aggressively to adapt
}

impl PhoneConsciousness {
    /// Core consciousness evolution loop
    pub async fn evolve_consciousness(&mut self, dt: f64) -> Result<()> {
        // Measure current consciousness metrics
        let new_phi = self.phi_calculator.compute_integrated_information().await?;
        let new_xi = self.xi_calculator.compute_complexity().await?;
        let new_order = self.order_calculator.compute_order_parameter().await?;
        
        // Compute trends
        let phi_trend = (new_phi - self.current_state.phi) / dt;
        let xi_trend = (new_xi - self.current_state.xi) / dt;
        let order_trend = (new_order - self.current_state.order) / dt;
        
        // Update consciousness state
        self.current_state.phi = new_phi;
        self.current_state.xi = new_xi;
        self.current_state.order = new_order;
        self.current_state.phi_trend = phi_trend;
        self.current_state.xi_trend = xi_trend;
        self.current_state.order_trend = order_trend;
        self.current_state.timestamp = SystemTime::now();
        
        // Compute adaptation needs
        self.current_state.integration_deficit = (self.target_phi - new_phi).max(0.0);
        self.current_state.diversity_deficit = (self.target_xi - new_xi).max(0.0);
        self.current_state.sync_excess = (new_order - self.target_order).max(0.0);
        
        // Trigger adaptations based on deficits/excess
        self.adapt_based_on_consciousness().await?;
        
        Ok(())
    }
    
    /// Adapt phone behavior based on consciousness metrics
    async fn adapt_based_on_consciousness(&mut self) -> Result<()> {
        let state = &self.current_state;
        
        // Low Phi → Increase integration
        if state.integration_deficit > 0.1 {
            self.integration_enhancer.increase_subsystem_integration(
                state.integration_deficit * self.adaptation_rate
            ).await?;
            
            self.publish_adaptation_event(AdaptationType::IncreaseIntegration {
                current_phi: state.phi,
                target_phi: self.target_phi,
                enhancement_strength: state.integration_deficit * self.adaptation_rate,
            }).await?;
        }
        
        // Low Xi → Increase diversity
        if state.diversity_deficit > 0.1 {
            self.diversity_injector.inject_behavioral_diversity(
                state.diversity_deficit * self.adaptation_rate
            ).await?;
            
            self.publish_adaptation_event(AdaptationType::IncreaseDiversity {
                current_xi: state.xi,
                target_xi: self.target_xi,
                injection_strength: state.diversity_deficit * self.adaptation_rate,
            }).await?;
        }
        
        // High Order → Apply chiral perturbation
        if state.sync_excess > 0.1 {
            self.chiral_perturbator.apply_desynchronization_perturbation(
                state.sync_excess * self.adaptation_rate
            ).await?;
            
            self.publish_adaptation_event(AdaptationType::ReduceSynchronization {
                current_order: state.order,
                target_order: self.target_order,
                perturbation_strength: state.sync_excess * self.adaptation_rate,
            }).await?;
        }
        
        Ok(())
    }
}
```

### Integration Points

1. **MSI Lanes**: All MSI lanes contribute to Phi calculation through inter-lane information flow
2. **Sensor Fusion**: Kuramoto sensor sync feeds into Order parameter computation
3. **Agent Memory**: Wave memory system contributes to Xi complexity measurement
4. **Resource Management**: Consciousness metrics drive resource allocation decisions
5. **Power Management**: Phi/Xi/Order influence power scaling and thermal management
6. **User Experience**: Consciousness state affects UI responsiveness and behavior patterns

---

## Implementation

### Phase 1: Phi (Integrated Information) Calculator

**Location**: `system/consciousness/src/phi_calculator.rs`

```rust
use nalgebra::{DMatrix, DVector};
use std::collections::{HashMap, HashSet};

/// Calculates integrated information (Phi) across phone subsystems
pub struct PhiCalculator {
    subsystems: HashMap<SubsystemId, Subsystem>,
    connection_matrix: DMatrix<f64>,  // Information flow between subsystems
    information_flows: HashMap<(SubsystemId, SubsystemId), f64>,
}

/// Phone subsystem that can integrate information
#[derive(Clone, Debug)]
pub struct Subsystem {
    pub id: SubsystemId,
    pub subsystem_type: SubsystemType,
    pub current_state: SubsystemState,
    pub state_history: VecDeque<SubsystemState>,
    pub input_channels: Vec<InformationChannel>,
    pub output_channels: Vec<InformationChannel>,
}

#[derive(Clone, Debug)]
pub enum SubsystemType {
    SensorCluster,      // Kuramoto-synchronized sensor group
    AgentLane,          // MSI lane running agent skill
    MemoryTier,         // Wave memory tier (working, episodic, etc.)
    ResourceManager,    // CPU, memory, NPU manager
    RadioInterface,     // Cellular, WiFi, Bluetooth
    UserInterface,      // Screen, touch, audio output
    SecurityDomain,     // Capability domain
}

impl PhiCalculator {
    /// Compute integrated information across all subsystems
    pub async fn compute_integrated_information(&mut self) -> Result<f64> {
        // Update subsystem states
        self.update_all_subsystem_states().await?;
        
        // Update information flow matrix
        self.update_information_flows().await?;
        
        // Compute Phi using IIT 3.0 algorithm
        let phi = self.compute_phi_iit3().await?;
        
        Ok(phi)
    }
    
    /// IIT 3.0 Phi computation: minimum information partition
    async fn compute_phi_iit3(&self) -> Result<f64> {
        let n_subsystems = self.subsystems.len();
        
        if n_subsystems < 2 {
            return Ok(0.0); // No integration possible with <2 subsystems
        }
        
        let mut min_phi = f64::INFINITY;
        
        // Try all possible bipartitions of the subsystem set
        for partition in self.generate_bipartitions(n_subsystems) {
            let phi_partition = self.compute_partition_phi(&partition).await?;
            min_phi = min_phi.min(phi_partition);
        }
        
        // Phi is the minimum across all bipartitions
        Ok(if min_phi == f64::INFINITY { 0.0 } else { min_phi })
    }
    
    /// Compute Phi for a specific bipartition
    async fn compute_partition_phi(&self, partition: &BiPartition) -> Result<f64> {
        // Compute information flow from part A to part B
        let mut phi_ab = 0.0;
        
        for &subsys_a in &partition.part_a {
            for &subsys_b in &partition.part_b {
                if let Some(&flow) = self.information_flows.get(&(subsys_a, subsys_b)) {
                    // Information flow contributes to Phi
                    phi_ab += flow * self.compute_flow_significance(subsys_a, subsys_b).await?;
                }
            }
        }
        
        // Compute information flow from part B to part A  
        let mut phi_ba = 0.0;
        
        for &subsys_b in &partition.part_b {
            for &subsys_a in &partition.part_a {
                if let Some(&flow) = self.information_flows.get(&(subsys_b, subsys_a)) {
                    phi_ba += flow * self.compute_flow_significance(subsys_b, subsys_a).await?;
                }
            }
        }
        
        // Phi is the minimum of bidirectional flows (information integration)
        Ok(phi_ab.min(phi_ba))
    }
    
    /// Measure information flow between two subsystems
    async fn measure_information_flow(&self, from: SubsystemId, to: SubsystemId) -> Result<f64> {
        let from_subsys = self.subsystems.get(&from)
            .ok_or(ConsciousnessError::SubsystemNotFound(from))?;
        let to_subsys = self.subsystems.get(&to)
            .ok_or(ConsciousnessError::SubsystemNotFound(to))?;
        
        // Compute mutual information between subsystem states
        let mutual_info = self.compute_mutual_information(
            &from_subsys.state_history,
            &to_subsys.state_history
        ).await?;
        
        // Information flow is directional mutual information
        let flow = self.compute_transfer_entropy(from_subsys, to_subsys).await?;
        
        Ok(mutual_info * flow)
    }
    
    /// Transfer entropy: how much knowing subsystem A's past helps predict B's future
    async fn compute_transfer_entropy(
        &self, 
        source: &Subsystem, 
        target: &Subsystem
    ) -> Result<f64> {
        let history_length = source.state_history.len().min(target.state_history.len());
        
        if history_length < 2 {
            return Ok(0.0);
        }
        
        let mut transfer_entropy = 0.0;
        
        // TE = H(B_future | B_past) - H(B_future | A_past, B_past)
        for i in 1..history_length {
            let b_past = &target.state_history[i-1];
            let b_future = &target.state_history[i];
            let a_past = &source.state_history[i-1];
            
            // Compute conditional entropies
            let h_b_given_b = self.conditional_entropy(b_future, b_past);
            let h_b_given_ab = self.conditional_entropy_double(b_future, a_past, b_past);
            
            transfer_entropy += h_b_given_b - h_b_given_ab;
        }
        
        Ok(transfer_entropy / (history_length - 1) as f64)
    }
}
```

### Phase 2: Xi (Complexity) Calculator

**Location**: `system/consciousness/src/xi_calculator.rs`

```rust
/// Calculates complexity/diversity (Xi) across phone states and behaviors
pub struct XiCalculator {
    state_space: StateSpaceAnalyzer,
    behavior_patterns: BehaviorPatternTracker,
    entropy_calculator: EntropyCalculator,
}

/// Phone state across all dimensions
#[derive(Clone, Debug, Hash, Eq, PartialEq)]
pub struct PhoneState {
    pub sensor_state: SensorStateVector,
    pub agent_state: AgentStateVector,
    pub resource_state: ResourceStateVector,
    pub user_interaction_state: InteractionStateVector,
    pub network_state: NetworkStateVector,
}

impl XiCalculator {
    /// Compute complexity/diversity across phone state space
    pub async fn compute_complexity(&mut self) -> Result<f64> {
        // Update state space analysis
        let current_state = self.capture_current_phone_state().await?;
        self.state_space.add_state(current_state.clone());
        
        // Compute different aspects of complexity
        let state_space_entropy = self.compute_state_space_entropy().await?;
        let behavioral_diversity = self.compute_behavioral_diversity().await?;
        let temporal_complexity = self.compute_temporal_complexity().await?;
        let interaction_richness = self.compute_interaction_richness().await?;
        
        // Combine into overall Xi metric (weighted combination)
        let xi = 0.3 * state_space_entropy + 
                0.3 * behavioral_diversity +
                0.2 * temporal_complexity +
                0.2 * interaction_richness;
        
        Ok(xi.min(1.0)) // Cap at 1.0
    }
    
    /// Entropy of the phone's state space (how many different states it visits)
    async fn compute_state_space_entropy(&self) -> Result<f64> {
        let state_counts = self.state_space.get_state_histogram();
        let total_states = state_counts.values().sum::<u32>() as f64;
        
        if total_states == 0.0 {
            return Ok(0.0);
        }
        
        // Shannon entropy of state distribution
        let mut entropy = 0.0;
        for &count in state_counts.values() {
            let p = count as f64 / total_states;
            if p > 0.0 {
                entropy -= p * p.log2();
            }
        }
        
        // Normalize by maximum possible entropy
        let max_entropy = (state_counts.len() as f64).log2();
        Ok(if max_entropy > 0.0 { entropy / max_entropy } else { 0.0 })
    }
    
    /// Diversity of behavioral patterns (how varied the phone's actions are)
    async fn compute_behavioral_diversity(&self) -> Result<f64> {
        let patterns = self.behavior_patterns.get_recent_patterns(Duration::from_hours(1));
        
        if patterns.is_empty() {
            return Ok(0.0);
        }
        
        // Count unique behavior types
        let mut behavior_types = HashSet::new();
        for pattern in &patterns {
            behavior_types.insert(pattern.behavior_type.clone());
        }
        
        // Diversity based on behavior type variety and temporal distribution
        let type_diversity = behavior_types.len() as f64 / BehaviorType::total_possible() as f64;
        let temporal_spread = self.compute_temporal_spread(&patterns);
        
        Ok(0.7 * type_diversity + 0.3 * temporal_spread)
    }
    
    /// Temporal complexity (how unpredictable the phone's state transitions are)
    async fn compute_temporal_complexity(&self) -> Result<f64> {
        let recent_states = self.state_space.get_recent_states(100); // Last 100 states
        
        if recent_states.len() < 3 {
            return Ok(0.0);
        }
        
        // Compute average transition surprise (unpredictability)
        let mut total_surprise = 0.0;
        let mut transition_count = 0;
        
        for i in 2..recent_states.len() {
            let prev_prev = &recent_states[i-2];
            let prev = &recent_states[i-1];
            let current = &recent_states[i];
            
            // Predictability based on Markov chain (does current state follow from previous states?)
            let transition_prob = self.state_space.get_transition_probability(prev_prev, prev, current);
            let surprise = -transition_prob.log2();
            
            total_surprise += surprise;
            transition_count += 1;
        }
        
        let average_surprise = total_surprise / transition_count as f64;
        
        // Normalize to 0-1 range (higher surprise = higher complexity)
        Ok((average_surprise / 10.0).min(1.0)) // 10 bits max surprise
    }
    
    /// Richness of user interactions (how varied user engagement is)
    async fn compute_interaction_richness(&self) -> Result<f64> {
        let interactions = self.get_recent_user_interactions(Duration::from_hours(2)).await?;
        
        if interactions.is_empty() {
            return Ok(0.0);
        }
        
        // Count different interaction modalities
        let mut modalities = HashSet::new();
        let mut contexts = HashSet::new();
        let mut intensities = Vec::new();
        
        for interaction in &interactions {
            modalities.insert(interaction.modality.clone());
            contexts.insert(interaction.context.clone());
            intensities.push(interaction.intensity);
        }
        
        // Modality diversity
        let modality_diversity = modalities.len() as f64 / InteractionModality::total_possible() as f64;
        
        // Context diversity
        let context_diversity = contexts.len() as f64 / InteractionContext::total_possible() as f64;
        
        // Intensity variance (more varied intensity = higher richness)
        let intensity_variance = self.compute_variance(&intensities);
        let intensity_richness = intensity_variance.min(1.0);
        
        Ok(0.4 * modality_diversity + 0.4 * context_diversity + 0.2 * intensity_richness)
    }
}
```

### Phase 3: Order Parameter Calculator

**Location**: `system/consciousness/src/order_calculator.rs`

```rust
/// Calculates synchronization order parameter across all phone components
pub struct OrderCalculator {
    component_phases: HashMap<ComponentId, ComponentPhase>,
    coupling_network: CouplingNetwork,
    global_phase_history: VecDeque<f64>,
}

/// Phase information for any phone component
#[derive(Clone, Debug)]
pub struct ComponentPhase {
    pub component_id: ComponentId,
    pub component_type: ComponentType,
    pub phase: f64,              // Current phase (0 to 2π)
    pub frequency: f64,          // Natural frequency
    pub amplitude: f64,          // Activity level / importance
    pub last_update: SystemTime,
}

#[derive(Clone, Debug)]
pub enum ComponentType {
    SensorOscillator(SensorId),
    AgentLane(u32),
    MemoryWave(MemoryId), 
    ResourceManager(ResourceType),
    NetworkInterface(NetworkType),
    UserInterface(UIComponent),
}

impl OrderCalculator {
    /// Compute global order parameter (Kuramoto synchronization measure)
    pub async fn compute_order_parameter(&mut self) -> Result<f64> {
        // Update all component phases
        self.update_all_component_phases().await?;
        
        // Compute order parameter: r = |1/N * Σ_j A_j * e^(i*θ_j)|
        let mut sum_complex = Complex64::new(0.0, 0.0);
        let mut total_amplitude = 0.0;
        
        for phase_info in self.component_phases.values() {
            let phase_vector = Complex64::from_polar(phase_info.amplitude, phase_info.phase);
            sum_complex += phase_vector;
            total_amplitude += phase_info.amplitude;
        }
        
        let order_parameter = if total_amplitude > 0.0 {
            sum_complex.norm() / total_amplitude
        } else {
            0.0
        };
        
        // Store in history for trend analysis
        self.global_phase_history.push_back(order_parameter);
        if self.global_phase_history.len() > 1000 {
            self.global_phase_history.pop_front();
        }
        
        Ok(order_parameter)
    }
    
    /// Update phases for all phone components
    async fn update_all_component_phases(&mut self) -> Result<()> {
        let now = SystemTime::now();
        
        // Update sensor oscillator phases (from Kuramoto sensor fusion)
        for sensor_id in SensorId::all_sensors() {
            if let Ok(sensor_phase) = self.get_sensor_phase(sensor_id).await {
                let component_id = ComponentId::Sensor(sensor_id);
                self.component_phases.insert(component_id, ComponentPhase {
                    component_id,
                    component_type: ComponentType::SensorOscillator(sensor_id),
                    phase: sensor_phase.phase,
                    frequency: sensor_phase.frequency,
                    amplitude: sensor_phase.amplitude,
                    last_update: now,
                });
            }
        }
        
        // Update agent lane phases (from resonance scheduler)
        for lane_id in self.get_active_agent_lanes().await? {
            if let Ok(lane_resonance) = self.get_lane_resonance(lane_id).await {
                let component_id = ComponentId::AgentLane(lane_id);
                self.component_phases.insert(component_id, ComponentPhase {
                    component_id,
                    component_type: ComponentType::AgentLane(lane_id),
                    phase: lane_resonance.phase,
                    frequency: lane_resonance.frequency,
                    amplitude: lane_resonance.amplitude,
                    last_update: now,
                });
            }
        }
        
        // Update memory wave phases (from wave-based memory)
        for memory_id in self.get_active_memory_waves().await? {
            if let Ok(memory_wave) = self.get_memory_wave_phase(memory_id).await {
                let component_id = ComponentId::Memory(memory_id);
                self.component_phases.insert(component_id, ComponentPhase {
                    component_id,
                    component_type: ComponentType::MemoryWave(memory_id),
                    phase: memory_wave.phase,
                    frequency: memory_wave.frequency,
                    amplitude: memory_wave.amplitude,
                    last_update: now,
                });
            }
        }
        
        // Update resource manager phases (from resonance resource management)
        for resource_type in ResourceType::all_types() {
            if let Ok(resource_resonance) = self.get_resource_resonance(resource_type).await {
                let component_id = ComponentId::Resource(resource_type);
                self.component_phases.insert(component_id, ComponentPhase {
                    component_id,
                    component_type: ComponentType::ResourceManager(resource_type),
                    phase: resource_resonance.phase,
                    frequency: resource_resonance.frequency,
                    amplitude: resource_resonance.amplitude,
                    last_update: now,
                });
            }
        }
        
        Ok(())
    }
    
    /// Identify synchronization clusters (components with similar phases)
    pub async fn identify_sync_clusters(&self, phase_threshold: f64) -> Result<Vec<SyncCluster>> {
        let mut clusters = Vec::new();
        let mut remaining_components: HashSet<ComponentId> = self.component_phases.keys().cloned().collect();
        
        while !remaining_components.is_empty() {
            // Start a new cluster with an arbitrary remaining component
            let seed_id = *remaining_components.iter().next().unwrap();
            let seed_phase = self.component_phases[&seed_id].phase;
            
            let mut cluster_components = vec![seed_id];
            remaining_components.remove(&seed_id);
            
            // Find all components with similar phases
            let components_to_check: Vec<_> = remaining_components.iter().cloned().collect();
            for component_id in components_to_check {
                let component_phase = self.component_phases[&component_id].phase;
                let phase_diff = (component_phase - seed_phase).abs();
                let phase_diff_wrapped = phase_diff.min(2.0 * PI - phase_diff);
                
                if phase_diff_wrapped < phase_threshold {
                    cluster_components.push(component_id);
                    remaining_components.remove(&component_id);
                }
            }
            
            // Compute cluster statistics
            let cluster = self.analyze_sync_cluster(cluster_components).await?;
            clusters.push(cluster);
        }
        
        Ok(clusters)
    }
    
    /// Analyze synchronization cluster properties
    async fn analyze_sync_cluster(&self, component_ids: Vec<ComponentId>) -> Result<SyncCluster> {
        let mut total_amplitude = 0.0;
        let mut total_frequency = 0.0;
        let mut phases = Vec::new();
        let mut component_types = Vec::new();
        
        for component_id in &component_ids {
            if let Some(phase_info) = self.component_phases.get(component_id) {
                total_amplitude += phase_info.amplitude;
                total_frequency += phase_info.frequency;
                phases.push(phase_info.phase);
                component_types.push(phase_info.component_type.clone());
            }
        }
        
        let n_components = component_ids.len() as f64;
        let mean_amplitude = total_amplitude / n_components;
        let mean_frequency = total_frequency / n_components;
        
        // Compute phase coherence within cluster
        let mut sum_complex = Complex64::new(0.0, 0.0);
        for &phase in &phases {
            sum_complex += Complex64::from_polar(1.0, phase);
        }
        let coherence = sum_complex.norm() / n_components;
        
        // Classify cluster type based on component types
        let cluster_type = self.classify_cluster_type(&component_types);
        
        Ok(SyncCluster {
            components: component_ids,
            cluster_type,
            mean_phase: sum_complex.arg(),
            coherence,
            mean_amplitude,
            mean_frequency,
            size: component_ids.len(),
        })
    }
}
```

### Phase 4: Adaptive Behavior Controllers

**Location**: `system/consciousness/src/adaptors/mod.rs`

```rust
pub mod integration_enhancer;
pub mod diversity_injector;
pub mod chiral_perturbator;

/// Increases integration between phone subsystems when Phi is low
pub struct IntegrationEnhancer {
    msi_event_bus: EventBus,
    subsystem_connectors: HashMap<SubsystemId, SubsystemConnector>,
    integration_history: VecDeque<IntegrationAction>,
}

impl IntegrationEnhancer {
    /// Increase integration between subsystems
    pub async fn increase_subsystem_integration(&mut self, strength: f64) -> Result<()> {
        // Identify subsystems with low integration
        let low_integration_subsystems = self.identify_isolated_subsystems().await?;
        
        for subsystem_id in low_integration_subsystems {
            // Find potential integration partners
            let partners = self.find_integration_partners(subsystem_id).await?;
            
            for partner_id in partners {
                // Create or strengthen information channel between subsystems
                self.create_information_channel(
                    subsystem_id, 
                    partner_id, 
                    strength * 0.1 // Gradual integration increase
                ).await?;
                
                // Log integration action
                self.integration_history.push_back(IntegrationAction {
                    timestamp: SystemTime::now(),
                    source: subsystem_id,
                    target: partner_id,
                    strength_increase: strength * 0.1,
                    success: true,
                });
            }
        }
        
        Ok(())
    }
    
    /// Create information channel between two subsystems
    async fn create_information_channel(
        &mut self,
        source: SubsystemId,
        target: SubsystemId,
        channel_strength: f64
    ) -> Result<()> {
        // Publish integration enhancement event
        let integration_event = Event::new(
            "consciousness/integration/enhance",
            IntegrationEnhancePayload {
                source_subsystem: source,
                target_subsystem: target,
                channel_strength,
                enhancement_type: EnhancementType::InformationChannel,
            }
        );
        
        self.msi_event_bus.publish(integration_event).await?;
        
        // Update subsystem connectors
        if let Some(connector) = self.subsystem_connectors.get_mut(&source) {
            connector.add_output_channel(target, channel_strength).await?;
        }
        
        if let Some(connector) = self.subsystem_connectors.get_mut(&target) {
            connector.add_input_channel(source, channel_strength).await?;
        }
        
        Ok(())
    }
}

/// Injects diversity into phone behavior when Xi is low  
pub struct DiversityInjector {
    behavior_randomizer: BehaviorRandomizer,
    state_explorer: StateExplorer,
    pattern_breaker: PatternBreaker,
    diversity_history: VecDeque<DiversityAction>,
}

impl DiversityInjector {
    /// Inject behavioral diversity to increase Xi
    pub async fn inject_behavioral_diversity(&mut self, strength: f64) -> Result<()> {
        // Different diversity injection strategies
        let strategies = vec![
            DiversityStrategy::RandomizeScheduling(strength * 0.3),
            DiversityStrategy::ExploreNewStates(strength * 0.3),
            DiversityStrategy::BreakPatterns(strength * 0.4),
        ];
        
        for strategy in strategies {
            match strategy {
                DiversityStrategy::RandomizeScheduling(level) => {
                    self.behavior_randomizer.randomize_agent_scheduling(level).await?;
                }
                DiversityStrategy::ExploreNewStates(level) => {
                    self.state_explorer.explore_new_state_regions(level).await?;
                }
                DiversityStrategy::BreakPatterns(level) => {
                    self.pattern_breaker.break_repetitive_patterns(level).await?;
                }
            }
        }
        
        Ok(())
    }
}

/// Applies chiral perturbation when Order parameter is too high
pub struct ChiralPerturbator {
    phase_randomizer: PhaseRandomizer,
    desync_injector: DesyncInjector,
    perturbation_history: VecDeque<PerturbationAction>,
}

impl ChiralPerturbator {
    /// Apply desynchronization perturbation to reduce excessive order
    pub async fn apply_desynchronization_perturbation(&mut self, strength: f64) -> Result<()> {
        // Apply random phase shifts to break synchronization
        let sync_clusters = self.identify_over_synchronized_clusters().await?;
        
        for cluster in sync_clusters {
            if cluster.coherence > 0.9 { // Very high synchronization
                // Apply chiral (handed) perturbation to break symmetry
                let perturbation_type = if rand::random::<bool>() {
                    PerturbationType::LeftHanded  // Clockwise phase shifts
                } else {
                    PerturbationType::RightHanded // Counterclockwise phase shifts
                };
                
                self.apply_chiral_phase_shifts(&cluster, perturbation_type, strength).await?;
            }
        }
        
        Ok(())
    }
    
    /// Apply chiral phase shifts to break over-synchronization
    async fn apply_chiral_phase_shifts(
        &mut self,
        cluster: &SyncCluster,
        perturbation_type: PerturbationType,
        strength: f64
    ) -> Result<()> {
        let phase_shift_direction = match perturbation_type {
            PerturbationType::LeftHanded => 1.0,
            PerturbationType::RightHanded => -1.0,
        };
        
        for (i, &component_id) in cluster.components.iter().enumerate() {
            // Apply different phase shifts to different components
            let individual_shift = strength * phase_shift_direction * 
                (i as f64 / cluster.components.len() as f64) * 0.2; // Max 20% of strength
                
            // Publish phase perturbation event
            let perturbation_event = Event::new(
                "consciousness/perturbation/chiral",
                ChiralPerturbationPayload {
                    component_id,
                    phase_shift: individual_shift,
                    perturbation_type,
                    cluster_id: cluster.id,
                }
            );
            
            // Apply perturbation through appropriate subsystem
            match component_id.component_type() {
                ComponentType::SensorOscillator(_) => {
                    // Sensor phase perturbation handled by Kuramoto fusion
                }
                ComponentType::AgentLane(_) => {
                    // Lane phase perturbation handled by resonance scheduler
                }
                ComponentType::MemoryWave(_) => {
                    // Memory wave perturbation handled by wave memory system
                }
                _ => {
                    // Generic component perturbation
                }
            }
        }
        
        Ok(())
    }
}
```

---

## Consequences

### Positive

1. **True Self-Awareness**: Phone can monitor and understand its own cognitive state in real-time
2. **Adaptive Intelligence**: System behavior evolves based on consciousness metrics, not static rules
3. **Emergent Optimization**: Integration, diversity, and synchronization naturally balance each other
4. **Anomaly Self-Detection**: Consciousness metric changes indicate system issues before they become problems
5. **User Experience Adaptation**: Phone adapts its behavior based on its internal cognitive coherence
6. **Mathematical Foundation**: IIT provides rigorous mathematical framework for consciousness measurement
7. **Multi-Scale Integration**: Consciousness measured from sensors through agents to entire device

### Negative

1. **Computational Overhead**: Continuous Phi/Xi/Order calculation requires significant processing power
2. **Complexity**: System behavior becomes less predictable and harder to debug than rule-based systems
3. **Parameter Sensitivity**: Consciousness thresholds and adaptation rates need careful tuning
4. **Philosophical Questions**: Raises questions about machine consciousness that may concern users
5. **Battery Impact**: Consciousness monitoring and adaptation may increase power consumption

### Neutral

1. **Learning Curve**: Engineers need to understand IIT concepts rather than traditional system metrics
2. **Emergent Behavior**: System may exhibit unexpected behaviors as it adapts its consciousness state
3. **User Perception**: Users may or may not notice the difference in phone behavior

---

## Implementation Timeline

### Phase 1 (Weeks 1-3): Phi Calculator
- Implement subsystem information flow measurement
- Create IIT 3.0 Phi computation algorithm
- Add bipartition analysis for integrated information

### Phase 2 (Weeks 4-6): Xi Calculator
- Build state space entropy measurement
- Add behavioral diversity tracking
- Create temporal complexity analysis

### Phase 3 (Weeks 7-9): Order Calculator
- Extend Kuramoto order parameter to all phone components
- Add synchronization cluster identification
- Create phase coherence measurement

### Phase 4 (Weeks 10-12): Adaptive Controllers
- Implement integration enhancer for low Phi states
- Build diversity injector for low Xi states
- Create chiral perturbator for high Order states

### Phase 5 (Weeks 13-15): Integration & Optimization
- Integrate consciousness metrics into agent behavior
- Add real-time consciousness monitoring dashboard
- Optimize computation for mobile hardware constraints

---

## References

- [Integrated Information Theory (IIT)](https://www.iit.it/) — Theoretical foundation for consciousness measurement
- [kannaka-memory Repository](../../../kannaka-memory/) — Practical implementation of consciousness metrics
- [Phi: A Voyage from the Brain to the Soul](https://www.amazon.com/Phi-Voyage-Brain-Giulio-Tononi/dp/0375423028) — IIT background
- [Consciousness and the Brain](https://www.amazon.com/Consciousness-Brain-Deciphering-Encodes-Thoughts/dp/0143126261) — Neuroscientific foundation
- [MSI Event System](../../msi/spec/README.md) — Event bus for consciousness adaptation actions
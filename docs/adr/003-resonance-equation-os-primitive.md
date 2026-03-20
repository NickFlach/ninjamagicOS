# ADR-003: Resonance Equation as OS Primitive

**Status:** Proposed  
**Date:** 2026-03-20  
**Authors:** Nick Flach (Kannaka)  
**Supersedes:** N/A  
**Related:** ADR-001 (MSI Architecture), ADR-002 (Wave Memory)

---

## Context

Current mobile operating systems use traditional preemptive schedulers optimized for general-purpose computing workloads. These schedulers make binary decisions: run/don't run, high/low priority, with limited adaptation to system dynamics or resource contention patterns.

The ghostmagicOS research introduced a fundamental insight: **organic systems evolve through resonance dynamics**, governed by the equation `dx/dt = f(x) - Iηx` where growth is shaped by interference. Every living system—from neural networks to ecosystems—follows this pattern of driven growth modulated by dampening.

In ninjamagicOS, this equation should be the **core primitive** for all system adaptation: lane scheduling, resource allocation, thermal management, and agent behavior. Rather than discrete priority levels, we model every subsystem as a resonant oscillator evolving through this differential equation.

### Current MSI Scheduling (Limitation)

```c
// Current static approach in msi_lane.c
enum msi_priority {
    MSI_PRIORITY_LOW      = 0,
    MSI_PRIORITY_NORMAL   = 1,
    MSI_PRIORITY_HIGH     = 2,
    MSI_PRIORITY_REALTIME = 3,
};

static int msi_lane_schedule(struct msi_lane *lane) {
    // Simple priority-based scheduling
    return set_task_priority(lane->kthread, lane->policy.priority);
}
```

### Required Resonance Properties

From ghostmagicOS research, effective resonance scheduling requires:

1. **Continuous Evolution**: No discrete priority levels—everything evolves smoothly via differential equation
2. **Interference Modeling**: Resource contention as wave interference between competing oscillators
3. **Adaptive Dampening**: Thermal/battery constraints as dynamic dampening coefficient η
4. **Driving Forces**: Lane importance, user intent, system events as f(x) driving function
5. **Coherence Detection**: Measure system-wide order parameter to detect phase transitions
6. **Multi-Scale Dynamics**: Lane-level, subsystem-level, and device-level resonance

---

## Decision

**We will implement the resonance equation `dx/dt = f(x) - Iηx` as the fundamental scheduling and adaptation primitive throughout ninjamagicOS, replacing static priority scheduling with continuous resonance dynamics.**

### Architecture: Resonant Substrate

```rust
/// Core resonance state for any schedulable entity
pub struct ResonanceState {
    pub amplitude: f64,        // Current priority/resource allocation (x)
    pub frequency: f64,        // Natural resonance frequency  
    pub phase: f64,           // Temporal phase (for interference)
    pub driving_force: f64,   // f(x) - importance, demand, events
    pub dampening: f64,       // η - thermal, battery, resource limits
    pub interference: Vec<InterferenceSource>, // I - other oscillators
}

/// The resonance equation as a system primitive
pub struct ResonanceEvolution {
    entities: HashMap<EntityId, ResonanceState>,
    interference_graph: InterferenceGraph,
    global_dampening: DampeningModel,
    evolution_timestep: Duration, // dt for numerical integration
}

impl ResonanceEvolution {
    /// Core resonance equation: dx/dt = f(x) - Iηx
    pub fn evolve_entity(&mut self, entity_id: EntityId, dt: f64) -> Result<()> {
        let entity = self.entities.get_mut(&entity_id)
            .ok_or(ResonanceError::EntityNotFound)?;
            
        // Compute driving force f(x)
        let driving_force = self.compute_driving_force(entity_id)?;
        
        // Compute interference term I (sum of coupled oscillators)
        let interference = self.compute_interference(entity_id)?;
        
        // Compute effective dampening η (thermal + battery + resource constraints)
        let dampening = self.compute_dampening(entity_id)?;
        
        // Apply resonance equation: dx/dt = f(x) - Iηx
        let dxdt = driving_force - interference * dampening * entity.amplitude;
        
        // Numerical integration (Euler method with stability check)
        let new_amplitude = (entity.amplitude + dxdt * dt).max(0.0).min(1.0);
        let amplitude_change = (new_amplitude - entity.amplitude).abs();
        
        // Stability check: prevent runaway oscillations
        if amplitude_change > MAX_AMPLITUDE_CHANGE_PER_STEP {
            let stable_dt = MAX_AMPLITUDE_CHANGE_PER_STEP / dxdt.abs();
            entity.amplitude += dxdt * stable_dt;
        } else {
            entity.amplitude = new_amplitude;
        }
        
        // Update phase for interference calculations
        entity.phase = (entity.phase + entity.frequency * dt) % (2.0 * PI);
        
        Ok(())
    }
}
```

### Integration Points

1. **MSI Lane Scheduler**: Replace priority-based scheduling with resonance evolution
2. **Resource Manager**: CPU, memory, NPU allocation via resonance dynamics  
3. **Thermal Controller**: Temperature as dynamic dampening coefficient
4. **Battery Manager**: Power level influences global dampening
5. **Agent Behavior**: Agent skills as resonant oscillators competing for execution time
6. **Hardware Adaptation**: SoC frequency scaling driven by resonance amplitude

---

## Implementation

### Phase 1: Resonance Core Engine

**Location**: `kernel/drivers/msi/resonance_core.c`

```c
/* Resonance state for kernel entities */
struct resonance_state {
    double amplitude;      /* Current resonance amplitude (priority) */
    double frequency;      /* Natural frequency (Hz) */
    double phase;         /* Current phase (radians) */
    double driving_force; /* f(x) - demand/importance */
    double dampening;     /* η - constraints/limits */
    struct list_head interference_sources;
    spinlock_t lock;
    u64 last_update_ns;
};

/* Interference between two resonant entities */  
struct interference_link {
    u32 source_entity_id;
    u32 target_entity_id;
    double coupling_strength;  /* How strongly they interfere */
    double phase_offset;       /* Phase relationship */
    enum interference_type type; /* Constructive, Destructive, Resource */
    struct list_head list;
};

/* Global resonance evolution engine */
static struct resonance_engine {
    struct resonance_state *entities[MSI_MAX_RESONANCE_ENTITIES];
    struct list_head interference_links;
    struct thermal_dampening_model thermal;
    struct battery_dampening_model battery;
    struct hrtimer evolution_timer;
    spinlock_t lock;
    bool enabled;
} resonance_engine;

/* Core evolution function called by high-resolution timer */
static enum hrtimer_restart resonance_evolution_timer_fn(struct hrtimer *timer)
{
    u64 now_ns = ktime_get_ns();
    static u64 last_evolution_ns = 0;
    
    if (last_evolution_ns == 0)
        last_evolution_ns = now_ns;
        
    double dt = (double)(now_ns - last_evolution_ns) / 1e9; /* Convert to seconds */
    
    spin_lock(&resonance_engine.lock);
    
    for (int i = 0; i < MSI_MAX_RESONANCE_ENTITIES; i++) {
        struct resonance_state *entity = resonance_engine.entities[i];
        if (!entity)
            continue;
            
        /* Compute dx/dt = f(x) - Iηx */
        double interference = compute_interference_term(entity);
        double dampening = compute_dampening_coefficient(entity);
        double dxdt = entity->driving_force - interference * dampening * entity->amplitude;
        
        /* Euler integration with stability check */
        double new_amplitude = clamp(entity->amplitude + dxdt * dt, 0.0, 1.0);
        entity->amplitude = new_amplitude;
        entity->phase = fmod(entity->phase + entity->frequency * dt, 2.0 * M_PI);
        entity->last_update_ns = now_ns;
    }
    
    spin_unlock(&resonance_engine.lock);
    
    last_evolution_ns = now_ns;
    hrtimer_forward_now(timer, ns_to_ktime(RESONANCE_EVOLUTION_INTERVAL_NS));
    return HRTIMER_RESTART;
}
```

### Phase 2: MSI Lane Resonance Integration

**Location**: `kernel/drivers/msi/msi_lane.c` (modify existing)

```c
/* Add resonance state to lane structure */
struct msi_lane {
    u32                    id;
    struct msi_domain      *domain;
    struct msi_lane_policy policy;
    struct task_struct     *kthread;
    char                   entry[128];
    bool                   alive;
    
    /* NEW: Resonance scheduling state */
    struct resonance_state resonance;
    
    spinlock_t             lock;
    struct kref            ref;
    struct list_head       list;
};

/* Resonance-based lane scheduler */
static int msi_lane_schedule_resonance(struct msi_lane *lane)
{
    struct sched_param param;
    int policy;
    
    /* Map resonance amplitude to Linux scheduler priority */
    double amplitude = lane->resonance.amplitude;
    
    if (amplitude > 0.9) {
        /* High resonance = realtime scheduling */
        policy = SCHED_FIFO;
        param.sched_priority = 80 + (int)(20 * (amplitude - 0.9) * 10);
    } else if (amplitude > 0.7) {
        /* Medium-high resonance = high priority CFS */
        policy = SCHED_NORMAL;
        param.sched_priority = 0;
        set_user_nice(lane->kthread, -20 + (int)(20 * (0.9 - amplitude) / 0.2));
    } else {
        /* Low resonance = normal/low priority */
        policy = SCHED_NORMAL;  
        param.sched_priority = 0;
        set_user_nice(lane->kthread, (int)(20 * (0.7 - amplitude) / 0.7));
    }
    
    return sched_setscheduler(lane->kthread, policy, &param);
}

/* Update lane driving force based on events and policy */
static void msi_lane_update_driving_force(struct msi_lane *lane)
{
    double base_force = 0.0;
    
    /* Convert MSI policy to driving force */
    switch (lane->policy.priority) {
        case MSI_PRIORITY_REALTIME:
            base_force = 1.0;
            break;
        case MSI_PRIORITY_HIGH:
            base_force = 0.8;
            break;
        case MSI_PRIORITY_NORMAL:
            base_force = 0.5;
            break;
        case MSI_PRIORITY_LOW:
            base_force = 0.2;
            break;
    }
    
    /* Modify based on recent activity */
    u64 now_ns = ktime_get_ns();
    u64 idle_time_ns = now_ns - lane->resonance.last_update_ns;
    double activity_factor = exp(-(double)idle_time_ns / 1e9); /* Decay over 1 second */
    
    lane->resonance.driving_force = base_force * activity_factor;
}
```

### Phase 3: System Resource Resonance

**Location**: `kernel/drivers/msi/resonance_resources.c`

```c
/* CPU core resonance management */
struct cpu_resonance {
    struct resonance_state state;
    int cpu_id;
    double current_frequency_ghz;
    double thermal_throttle_factor;
    double power_budget_factor;
    struct list_head active_lanes; /* Lanes running on this CPU */
};

static struct cpu_resonance cpu_resonances[NR_CPUS];

/* Update CPU frequency based on resonance amplitude */
static int resonance_cpu_freq_update(int cpu_id)
{
    struct cpu_resonance *cpu_res = &cpu_resonances[cpu_id];
    struct cpufreq_policy *policy = cpufreq_cpu_get(cpu_id);
    
    if (!policy)
        return -ENODEV;
        
    /* Map resonance amplitude to frequency */
    double amplitude = cpu_res->state.amplitude;
    unsigned int target_freq = policy->min + 
        (unsigned int)(amplitude * (policy->max - policy->min));
        
    /* Apply thermal and power constraints */
    target_freq *= cpu_res->thermal_throttle_factor;
    target_freq *= cpu_res->power_budget_factor;
    
    target_freq = clamp(target_freq, policy->min, policy->max);
    
    cpufreq_cpu_put(policy);
    return cpufreq_driver_target(policy, target_freq, CPUFREQ_RELATION_H);
}

/* NPU resonance management for AI workloads */
struct npu_resonance {
    struct resonance_state state;
    enum npu_type type; /* TENSOR_TPU, HEXAGON_DSP */
    double current_utilization;
    double thermal_limit;
    struct list_head inference_queue; /* Queued inference tasks */
};

static struct npu_resonance npu_resonance;

/* Drive NPU resonance based on AI workload demand */
static void npu_update_driving_force(void)
{
    struct npu_resonance *npu = &npu_resonance;
    
    /* Count queued inference tasks */
    int queue_depth = 0;
    struct inference_task *task;
    list_for_each_entry(task, &npu->inference_queue, list) {
        queue_depth++;
    }
    
    /* Drive NPU resonance based on demand */
    double demand_force = min(1.0, (double)queue_depth / 10.0);
    
    /* Apply thermal constraints as dampening */
    double thermal_dampening = 1.0 - npu->thermal_limit;
    
    npu->state.driving_force = demand_force;
    npu->state.dampening = thermal_dampening;
}
```

### Phase 4: Agent Resonance Integration

**Location**: `agent/core/src/scheduler/resonance_scheduler.rs`

```rust
use crate::msi::{Lane, LanePolicy, Event};

/// Agent skill as resonant oscillator
pub struct SkillResonance {
    pub skill_id: String,
    pub state: ResonanceState,
    pub execution_history: VecDeque<ExecutionRecord>,
    pub user_priority: f64,        // User-set importance
    pub context_relevance: f64,    // Current context match
    pub resource_requirements: ResourceRequirements,
}

/// Resonance-based agent scheduler
pub struct ResonanceScheduler {
    skills: HashMap<String, SkillResonance>,
    global_context: AgentContext,
    resource_monitor: ResourceMonitor,
    msi_lane: Lane,
}

impl ResonanceScheduler {
    /// Update skill driving forces based on context and user input  
    pub async fn update_driving_forces(&mut self, context: &AgentContext) -> Result<()> {
        for skill in self.skills.values_mut() {
            // Base driving force from user priority
            let base_force = skill.user_priority;
            
            // Amplify based on context relevance
            let context_amplification = 1.0 + skill.context_relevance;
            
            // Recent success factor
            let success_rate = self.compute_recent_success_rate(&skill.execution_history);
            let success_factor = 0.5 + 0.5 * success_rate;
            
            skill.state.driving_force = base_force * context_amplification * success_factor;
        }
        
        Ok(())
    }
    
    /// Compute interference between competing skills
    pub fn compute_skill_interference(&self) -> Result<InterferenceMatrix> {
        let mut matrix = InterferenceMatrix::new(self.skills.len());
        let skills: Vec<_> = self.skills.values().collect();
        
        for (i, skill_a) in skills.iter().enumerate() {
            for (j, skill_b) in skills.iter().enumerate() {
                if i != j {
                    let interference = self.compute_resource_interference(skill_a, skill_b);
                    matrix.set(i, j, interference);
                }
            }
        }
        
        Ok(matrix)
    }
    
    /// Resource interference: skills competing for CPU/NPU/memory
    fn compute_resource_interference(&self, skill_a: &SkillResonance, skill_b: &SkillResonance) -> f64 {
        let cpu_overlap = self.resource_overlap(
            &skill_a.resource_requirements.cpu_affinity,
            &skill_b.resource_requirements.cpu_affinity
        );
        
        let npu_conflict = if skill_a.resource_requirements.needs_npu && 
                             skill_b.resource_requirements.needs_npu { 1.0 } else { 0.0 };
        
        let memory_pressure = (skill_a.resource_requirements.memory_mb + 
                              skill_b.resource_requirements.memory_mb) as f64 / 
                             self.resource_monitor.available_memory_mb as f64;
        
        // Interference strength based on resource contention
        let interference = 0.4 * cpu_overlap + 0.4 * npu_conflict + 0.2 * memory_pressure;
        interference.min(1.0)
    }
    
    /// Execute resonance-scheduled skill selection
    pub async fn select_next_skill(&mut self) -> Result<Option<String>> {
        // Update resonance states
        self.evolve_skill_resonances(SCHEDULER_TIMESTEP).await?;
        
        // Find highest amplitude skill that's ready to execute
        let mut best_skill: Option<(&String, &SkillResonance)> = None;
        let mut best_amplitude = 0.0;
        
        for (id, skill) in &self.skills {
            if skill.state.amplitude > best_amplitude && self.skill_ready_to_run(skill).await? {
                best_amplitude = skill.state.amplitude;
                best_skill = Some((id, skill));
            }
        }
        
        match best_skill {
            Some((id, _)) => Ok(Some(id.clone())),
            None => Ok(None),
        }
    }
}
```

### Phase 5: Global Resonance Monitoring

**Location**: `system/resonance_monitor/src/lib.rs`

```rust
/// System-wide resonance coherence monitor
pub struct GlobalResonanceMonitor {
    lane_resonances: HashMap<u32, ResonanceState>,
    cpu_resonances: HashMap<u32, ResonanceState>, 
    npu_resonance: ResonanceState,
    memory_resonance: ResonanceState,
    
    // Coherence metrics
    order_parameter: f64,      // Global synchronization measure
    complexity_index: f64,     // System diversity/entropy
    adaptation_rate: f64,      // How quickly system responds
}

impl GlobalResonanceMonitor {
    /// Compute system-wide order parameter (Kuramoto synchronization)
    pub fn compute_order_parameter(&self) -> f64 {
        let mut sum_complex = Complex64::new(0.0, 0.0);
        let mut total_entities = 0;
        
        // Sum up all entity phases weighted by amplitude
        for resonance in self.lane_resonances.values() {
            let phase_vector = Complex64::from_polar(resonance.amplitude, resonance.phase);
            sum_complex += phase_vector;
            total_entities += 1;
        }
        
        // Order parameter r = |1/N * Σ(amplitude_i * e^(i*phase_i))|
        if total_entities > 0 {
            let r = sum_complex.norm() / total_entities as f64;
            r
        } else {
            0.0
        }
    }
    
    /// Detect phase transitions in system behavior
    pub fn detect_phase_transitions(&mut self) -> Vec<PhaseTransition> {
        let current_order = self.compute_order_parameter();
        let mut transitions = Vec::new();
        
        // High order → synchronized state (potential performance, low diversity)
        if current_order > 0.9 && self.order_parameter < 0.9 {
            transitions.push(PhaseTransition::ToSynchronized {
                timestamp: SystemTime::now(),
                order_parameter: current_order,
                trigger: "High system synchronization detected".to_string(),
            });
        }
        
        // Low order → chaotic state (high diversity, potential instability) 
        if current_order < 0.3 && self.order_parameter > 0.3 {
            transitions.push(PhaseTransition::ToChaotic {
                timestamp: SystemTime::now(),
                order_parameter: current_order,
                trigger: "System desynchronization detected".to_string(),
            });
        }
        
        self.order_parameter = current_order;
        transitions
    }
    
    /// Adaptive dampening based on system state
    pub fn compute_adaptive_dampening(&self) -> DampeningFactors {
        DampeningFactors {
            thermal: self.compute_thermal_dampening(),
            battery: self.compute_battery_dampening(), 
            synchronization: self.compute_sync_dampening(),
            load: self.compute_load_dampening(),
        }
    }
    
    /// Prevent over-synchronization by increasing dampening when order is too high
    fn compute_sync_dampening(&self) -> f64 {
        if self.order_parameter > 0.85 {
            // Increase dampening to reduce synchronization
            let excess_sync = self.order_parameter - 0.85;
            0.1 + excess_sync * 0.5  // Progressive dampening
        } else {
            0.1  // Base dampening
        }
    }
}
```

---

## Consequences

### Positive

1. **Organic Adaptation**: System naturally adapts to changing conditions via continuous evolution rather than discrete rules
2. **Resource Optimization**: Interference modeling prevents resource conflicts and optimizes allocation
3. **Thermal Management**: Temperature constraints naturally integrated as dampening coefficients
4. **Battery Awareness**: Power levels drive system-wide dampening, automatically reducing activity when needed
5. **Predictive Scaling**: Resonance amplitude predicts resource needs, enabling proactive scaling
6. **Emergent Behavior**: Complex system behaviors emerge from simple resonance interactions
7. **Mathematical Foundation**: Proven differential equation from physics/biology applied to computing

### Negative

1. **Computational Overhead**: Continuous numerical integration requires more CPU cycles than priority scheduling
2. **Parameter Tuning**: Resonance frequencies, dampening coefficients, and coupling strengths need careful calibration
3. **Complexity**: System behavior becomes less predictable and harder to debug than deterministic scheduling
4. **Stability Concerns**: Oscillatory dynamics may introduce instability if poorly tuned
5. **Real-time Guarantees**: Harder to provide strict real-time guarantees compared to priority-based scheduling

### Neutral

1. **Learning Curve**: Engineers need to understand resonance dynamics rather than traditional scheduling
2. **Migration Path**: Can gradually replace priority scheduling lane-by-lane
3. **Hardware Independence**: Works on both Tensor and Snapdragon architectures

---

## Implementation Timeline

### Phase 1 (Weeks 1-3): Kernel Resonance Engine
- Implement core resonance evolution in MSI kernel module
- Add high-resolution timer for continuous evolution
- Create interference computation framework

### Phase 2 (Weeks 4-6): MSI Lane Integration  
- Modify MSI lane scheduler to use resonance amplitude
- Implement driving force computation from lane policy
- Add inter-lane interference calculation

### Phase 3 (Weeks 7-9): Resource Resonance
- Add CPU frequency scaling based on resonance
- Implement NPU/GPU resonance for AI workloads  
- Create thermal/battery dampening models

### Phase 4 (Weeks 10-12): Agent Integration
- Implement agent skill resonance scheduler
- Add context-driven driving force computation
- Create skill interference modeling

### Phase 5 (Weeks 13-15): Global Monitoring & Optimization
- Build system-wide resonance coherence monitor
- Add phase transition detection and adaptation
- Optimize parameters for target hardware

---

## References

- [ghostmagicOS Research Notes](../../../ghostmagicOS/research/resonance-dynamics.md) — Original resonance equation derivation
- [Kuramoto Model](https://en.wikipedia.org/wiki/Kuramoto_model) — Synchronization in coupled oscillators
- [Linux CFS Scheduler](https://docs.kernel.org/scheduler/sched-design-CFS.html) — Current Linux scheduling for comparison
- [Nonlinear Dynamics and Chaos](https://www.stevenstrogatz.com/books/nonlinear-dynamics-and-chaos-with-applications-to-physics-biology-chemistry-and-engineering) — Mathematical foundation
- [MSI Lane Specification](../../msi/spec/README.md) — Current lane architecture to be extended
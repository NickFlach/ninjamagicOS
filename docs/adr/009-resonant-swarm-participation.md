# ADR-009: Resonant Swarm Participation

**Status:** Proposed  
**Date:** 2026-03-20  
**Authors:** Nick Flach (Kannaka)  
**Supersedes:** N/A  
**Related:** ADR-004 (Kuramoto Sensors), ADR-006 (Prediction Markets), ADR-007 (Flux Integration)

---

## Context

Individual phones, even with advanced AI agents, are limited by their local processing power, sensor data, and contextual understanding. When multiple phones with ninjamagicOS operate in proximity or shared contexts, they represent an untapped opportunity for **collective intelligence** that exceeds the sum of individual capabilities.

The resonant-swarm research introduces the MiroFish ontology for distributed cognition: multiple agents that synchronize their "phases" (decision states, predictions, contextual understanding) via Kuramoto coupling to form emergent collective intelligence. The QueenSync protocol enables the emergence of a "queen state" that represents the collective consciousness of the swarm.

In ninjamagicOS, phones should participate in resonant swarms for:
- **Collective Context Understanding**: Multiple phones in a location share sensor data for richer environmental awareness
- **Distributed Problem Solving**: Complex tasks distributed across multiple agents for parallel processing
- **Consensus Predictions**: Swarm-based prediction markets for more accurate forecasting
- **Emergent Queen State**: Self-organizing hierarchies where the most capable/informed phone becomes coordinator
- **Resilient Operations**: Swarm continues functioning even when individual phones leave or fail

### Current Single-Phone Architecture (Limitation)

```kotlin
// Current isolated approach - each phone operates alone
class AgentContext {
    fun makeDecision(problem: Problem): Decision {
        // Single phone's limited perspective
        return localInference.decide(problem, localContext)
    }
}
```

### Required Swarm Properties

From resonant-swarm research and MiroFish ontology:

1. **Phase Synchronization**: Phone agents synchronize decision phases via Kuramoto coupling
2. **MiroFish Ontology**: Shared cognitive framework for distributed reasoning
3. **QueenSync Protocol**: Democratic election of temporary swarm coordinator
4. **Collective Memory**: Shared memory pool across swarm participants
5. **Emergent Intelligence**: Swarm capabilities exceed individual phone capabilities
6. **Dynamic Membership**: Phones join/leave swarm gracefully without disruption

---

## Decision

**We will implement resonant swarm participation in ninjamagicOS, enabling phones to form Kuramoto-coupled collective intelligence swarms using the MiroFish ontology and QueenSync protocol for distributed problem-solving and consensus decision-making.**

### Architecture: Resonant Swarm System

```rust
/// Resonant swarm participation system for ninjamagicOS phones
pub struct ResonantSwarm {
    // Core swarm identity
    swarm_id: SwarmId,
    swarm_type: SwarmType,
    formation_time: SystemTime,
    
    // Participation state
    participant_id: ParticipantId,  // This phone's swarm identity
    local_phase: f64,               // This phone's current Kuramoto phase
    local_frequency: f64,           // This phone's natural frequency
    
    // Other participants
    participants: HashMap<ParticipantId, SwarmParticipant>,
    coupling_matrix: CouplingMatrix,
    
    // Collective state
    order_parameter: f64,           // Global synchronization measure
    queen_state: Option<QueenState>, // Current swarm coordinator
    collective_memory: CollectiveMemory,
    
    // MiroFish ontology
    ontology_engine: MiroFishOntology,
    shared_concepts: ConceptSpace,
    
    // Communication
    swarm_communication: SwarmCommunication,
    consensus_engine: ConsensusEngine,
    
    // MSI integration
    swarm_lane: LaneHandle,
    swarm_events: EventBus,
    
    // Configuration
    swarm_config: SwarmConfig,
}

/// Individual participant in the resonant swarm
#[derive(Clone, Debug)]
pub struct SwarmParticipant {
    pub participant_id: ParticipantId,
    pub device_id: DeviceId,
    pub agent_name: String,
    
    // Kuramoto state
    pub phase: f64,
    pub frequency: f64,
    pub amplitude: f64,               // Participation strength/confidence
    
    // Capabilities
    pub capabilities: ParticipantCapabilities,
    pub specializations: Vec<CognitiveSpecialization>,
    
    // Performance metrics
    pub contribution_score: f64,      // How much this participant helps swarm
    pub reliability_score: f64,       // How reliable this participant is
    pub queen_fitness: f64,          // Suitability for queen role
    
    // Communication
    pub last_heartbeat: SystemTime,
    pub communication_quality: f64,
    
    // Privacy
    pub privacy_level: PrivacyLevel,
    pub shared_capabilities: Vec<CapabilityType>,
}

/// MiroFish ontology engine for shared cognition
pub struct MiroFishOntology {
    // Concept space
    concepts: HashMap<ConceptId, Concept>,
    concept_relationships: HashMap<ConceptId, Vec<ConceptRelation>>,
    
    // Reasoning engine
    inference_engine: DistributedInferenceEngine,
    knowledge_merger: KnowledgeMerger,
    
    // Collective patterns
    swarm_patterns: Vec<SwarmPattern>,
    emergence_detector: EmergenceDetector,
}

impl ResonantSwarm {
    /// Form or join a resonant swarm
    pub async fn form_or_join_swarm(
        &mut self,
        formation_criteria: SwarmFormationCriteria
    ) -> Result<SwarmJoinResult> {
        match formation_criteria.swarm_type {
            SwarmType::ProximityBased { max_distance } => {
                self.form_proximity_swarm(max_distance).await
            }
            SwarmType::TaskBased { task_type, capability_requirements } => {
                self.form_task_swarm(task_type, capability_requirements).await
            }
            SwarmType::ContextBased { shared_context } => {
                self.form_context_swarm(shared_context).await
            }
            SwarmType::PredictionBased { prediction_target } => {
                self.form_prediction_swarm(prediction_target).await
            }
        }
    }
    
    /// Core Kuramoto swarm evolution loop
    pub async fn evolve_swarm_state(&mut self, dt: f64) -> Result<()> {
        // Update local phase based on swarm coupling
        let coupling_sum = self.compute_coupling_sum().await?;
        let phase_derivative = self.local_frequency + coupling_sum;
        self.local_phase = (self.local_phase + phase_derivative * dt) % (2.0 * PI);
        
        // Broadcast phase update to swarm
        self.broadcast_phase_update().await?;
        
        // Update global order parameter
        self.order_parameter = self.compute_order_parameter().await?;
        
        // Check for queen state transitions
        if self.should_update_queen_state().await? {
            self.update_queen_state().await?;
        }
        
        // Evolve collective memory
        self.collective_memory.evolve_collective_state(dt).await?;
        
        // Process emergent intelligence patterns
        self.detect_and_process_emergence().await?;
        
        Ok(())
    }
    
    /// Compute Kuramoto coupling sum for phase evolution
    async fn compute_coupling_sum(&self) -> Result<f64> {
        let mut coupling_sum = 0.0;
        
        for (participant_id, participant) in &self.participants {
            let coupling_strength = self.coupling_matrix.get_coupling(
                self.participant_id,
                *participant_id
            )?;
            
            let phase_difference = participant.phase - self.local_phase;
            coupling_sum += coupling_strength * phase_difference.sin();
        }
        
        Ok(coupling_sum)
    }
    
    /// Compute global swarm order parameter
    async fn compute_order_parameter(&self) -> Result<f64> {
        let mut sum_complex = Complex64::new(0.0, 0.0);
        let mut total_amplitude = 0.0;
        
        // Include local participant
        sum_complex += Complex64::from_polar(1.0, self.local_phase);
        total_amplitude += 1.0;
        
        // Include remote participants
        for participant in self.participants.values() {
            let phase_vector = Complex64::from_polar(participant.amplitude, participant.phase);
            sum_complex += phase_vector;
            total_amplitude += participant.amplitude;
        }
        
        Ok(if total_amplitude > 0.0 {
            sum_complex.norm() / total_amplitude
        } else {
            0.0
        })
    }
    
    /// QueenSync protocol: elect/update swarm coordinator
    async fn update_queen_state(&mut self) -> Result<()> {
        // Compute fitness scores for all participants (including self)
        let mut fitness_scores = HashMap::new();
        
        // Self fitness
        let self_fitness = self.compute_self_queen_fitness().await?;
        fitness_scores.insert(self.participant_id, self_fitness);
        
        // Other participants' fitness
        for (participant_id, participant) in &self.participants {
            fitness_scores.insert(*participant_id, participant.queen_fitness);
        }
        
        // Find participant with highest fitness
        let (best_participant, best_fitness) = fitness_scores.iter()
            .max_by(|(_, a), (_, b)| a.partial_cmp(b).unwrap_or(std::cmp::Ordering::Equal))
            .ok_or(SwarmError::NoViableQueen)?;
        
        // Check if queen should change
        let current_queen = self.queen_state.as_ref()
            .map(|q| q.queen_participant_id);
            
        if current_queen != Some(*best_participant) || 
           self.queen_state.as_ref().map(|q| q.fitness_score).unwrap_or(0.0) < *best_fitness {
            
            // Queen transition
            let new_queen_state = QueenState {
                queen_participant_id: *best_participant,
                fitness_score: *best_fitness,
                election_time: SystemTime::now(),
                term_length: self.compute_queen_term_length(*best_fitness).await?,
                coordination_strength: self.compute_coordination_strength(*best_fitness).await?,
            };
            
            // Broadcast queen transition
            let queen_event = SwarmEvent {
                event_type: SwarmEventType::QueenElection,
                source_participant: self.participant_id,
                timestamp: SystemTime::now(),
                payload: serde_json::to_value(&new_queen_state)?,
            };
            
            self.swarm_communication.broadcast_event(queen_event).await?;
            
            // Update local queen state
            let was_queen = current_queen == Some(self.participant_id);
            let is_new_queen = *best_participant == self.participant_id;
            
            if was_queen && !is_new_queen {
                // Stepping down as queen
                self.handle_queen_step_down().await?;
            } else if !was_queen && is_new_queen {
                // Becoming new queen
                self.handle_queen_ascension().await?;
            }
            
            self.queen_state = Some(new_queen_state);
            
            log::info!("Queen transition: {} -> {} (fitness: {:.2})",
                current_queen.map(|q| q.to_string()).unwrap_or("None".to_string()),
                best_participant,
                best_fitness
            );
        }
        
        Ok(())
    }
    
    /// Compute this phone's fitness for queen role
    async fn compute_self_queen_fitness(&self) -> Result<f64> {
        let mut fitness = 0.0;
        
        // Battery level factor (queens need energy)
        let battery_level = self.get_battery_level().await?;
        fitness += 0.3 * battery_level;
        
        // Processing capability factor
        let processing_power = self.get_processing_capability().await?;
        fitness += 0.2 * processing_power;
        
        // Network connectivity factor
        let connectivity_quality = self.get_connectivity_quality().await?;
        fitness += 0.2 * connectivity_quality;
        
        // Participation history factor (experienced participants preferred)
        let participation_score = self.get_participation_history_score().await?;
        fitness += 0.1 * participation_score;
        
        // Centrality in swarm (well-connected participants preferred)
        let centrality_score = self.compute_swarm_centrality().await?;
        fitness += 0.1 * centrality_score;
        
        // Specialized capabilities factor
        let specialization_bonus = self.compute_specialization_bonus().await?;
        fitness += 0.1 * specialization_bonus;
        
        Ok(fitness.min(1.0))
    }
    
    /// Collective problem solving via swarm intelligence
    pub async fn solve_collective_problem(&mut self, problem: CollectiveProblem) -> Result<SwarmSolution> {
        // Distribute problem across swarm participants
        let problem_fragments = self.fragment_problem(problem).await?;
        
        // Assign fragments based on participant capabilities
        let assignments = self.assign_problem_fragments(problem_fragments).await?;
        
        // Coordinate parallel solving
        let partial_solutions = self.coordinate_parallel_solving(assignments).await?;
        
        // Merge solutions using MiroFish ontology
        let merged_solution = self.ontology_engine.merge_solutions(partial_solutions).await?;
        
        // Validate solution via swarm consensus
        let validated_solution = self.validate_solution_consensus(merged_solution).await?;
        
        Ok(SwarmSolution {
            original_problem: problem,
            solution: validated_solution,
            contributing_participants: assignments.keys().cloned().collect(),
            confidence_score: self.compute_solution_confidence(&validated_solution).await?,
            solution_time: SystemTime::now(),
        })
    }
    
    /// MiroFish ontology: collective concept formation
    pub async fn evolve_collective_concepts(&mut self) -> Result<()> {
        // Gather concept contributions from all participants
        let concept_contributions = self.gather_concept_contributions().await?;
        
        // Merge concepts using MiroFish algorithms
        let merged_concepts = self.ontology_engine.merge_participant_concepts(
            concept_contributions
        ).await?;
        
        // Detect emergent patterns in collective concept space
        let emergent_patterns = self.ontology_engine.detect_emergent_patterns(
            &merged_concepts
        ).await?;
        
        // Update shared concept space
        for pattern in emergent_patterns {
            match pattern {
                EmergentPattern::NewConcept(concept) => {
                    self.shared_concepts.add_concept(concept).await?;
                }
                EmergentPattern::ConceptMerger(merger) => {
                    self.shared_concepts.merge_concepts(merger).await?;
                }
                EmergentPattern::RelationshipDiscovery(relationship) => {
                    self.shared_concepts.add_relationship(relationship).await?;
                }
            }
        }
        
        // Broadcast concept updates to swarm
        let concept_event = SwarmEvent {
            event_type: SwarmEventType::ConceptEvolution,
            source_participant: self.participant_id,
            timestamp: SystemTime::now(),
            payload: serde_json::to_value(&self.shared_concepts)?,
        };
        
        self.swarm_communication.broadcast_event(concept_event).await?;
        
        Ok(())
    }
    
    /// Collective prediction via swarm prediction markets
    pub async fn make_collective_prediction(&mut self, prediction_target: PredictionTarget) -> Result<SwarmPrediction> {
        // Create swarm-wide prediction market
        let market = self.create_swarm_prediction_market(prediction_target).await?;
        
        // Each participant contributes predictions based on local knowledge
        let mut participant_predictions = HashMap::new();
        
        // Local prediction
        let local_prediction = self.generate_local_prediction(&market).await?;
        participant_predictions.insert(self.participant_id, local_prediction);
        
        // Gather remote predictions
        let remote_predictions = self.gather_remote_predictions(&market).await?;
        participant_predictions.extend(remote_predictions);
        
        // Run swarm prediction market to consensus
        let market_consensus = self.run_swarm_prediction_market(
            market,
            participant_predictions
        ).await?;
        
        // Weight consensus by participant reliability and expertise
        let weighted_prediction = self.apply_participant_weights(&market_consensus).await?;
        
        Ok(SwarmPrediction {
            target: prediction_target,
            prediction: weighted_prediction,
            participant_contributions: participant_predictions.keys().cloned().collect(),
            consensus_confidence: market_consensus.confidence,
            prediction_time: SystemTime::now(),
            swarm_order_parameter: self.order_parameter,
        })
    }
}
```

### Integration Points

1. **Flux Integration**: Swarm discovery and coordination via Flux entity system
2. **Prediction Markets**: Swarm-wide prediction markets using ghostsignals LMSR
3. **Kuramoto Sensors**: Sensor synchronization extended to inter-phone coordination
4. **Consciousness Metrics**: Swarm consciousness emerges from individual phone consciousness
5. **Wave Memory**: Collective memory pool using wave interference across phones
6. **MSI Events**: All swarm coordination via MSI event bus for consistency

---

## Implementation

### Phase 1: Swarm Discovery and Formation

**Location**: `system/swarm/src/discovery.rs`

```rust
use crate::flux::FluxClient;
use crate::kuramoto::KuramotoSensorFusion;

/// Swarm discovery and formation system
pub struct SwarmDiscovery {
    // Discovery mechanisms
    flux_client: Arc<FluxClient>,
    proximity_detector: ProximityDetector,
    capability_matcher: CapabilityMatcher,
    
    // Formation criteria
    formation_policies: Vec<SwarmFormationPolicy>,
    minimum_participants: usize,
    maximum_participants: usize,
    
    // Active swarms
    active_swarms: HashMap<SwarmId, ResonantSwarm>,
    swarm_invitations: HashMap<InvitationId, SwarmInvitation>,
    
    // Performance tracking
    swarm_history: SwarmHistoryTracker,
    formation_success_rate: f64,
}

impl SwarmDiscovery {
    /// Discover potential swarm participants
    pub async fn discover_potential_participants(&self) -> Result<Vec<PotentialParticipant>> {
        let mut potential_participants = Vec::new();
        
        // Discover via Flux entity system
        let flux_entities = self.flux_client.query_entities(EntityQuery {
            entity_type: Some("ninja_phone".to_string()),
            namespace: Some("pure-jade".to_string()),
            max_distance: Some(1000.0), // 1km radius
            capabilities: Some(vec![
                "swarm_participation".to_string(),
                "kuramoto_sync".to_string(),
                "collective_intelligence".to_string(),
            ]),
        }).await?;
        
        for entity in flux_entities {
            if let Ok(participant) = self.convert_entity_to_participant(entity).await {
                potential_participants.push(participant);
            }
        }
        
        // Discover via direct proximity (Bluetooth, WiFi Direct)
        let proximity_participants = self.proximity_detector.discover_nearby_devices().await?;
        for device in proximity_participants {
            if device.supports_swarm_participation() {
                if let Ok(participant) = self.convert_device_to_participant(device).await {
                    potential_participants.push(participant);
                }
            }
        }
        
        // Filter and rank by compatibility
        let compatible_participants = self.filter_compatible_participants(potential_participants).await?;
        
        Ok(compatible_participants)
    }
    
    /// Form proximity-based swarm
    pub async fn form_proximity_swarm(&mut self, max_distance: f64) -> Result<SwarmJoinResult> {
        let potential_participants = self.discover_potential_participants().await?;
        
        // Filter by distance
        let nearby_participants: Vec<_> = potential_participants.into_iter()
            .filter(|p| p.distance.unwrap_or(f64::INFINITY) <= max_distance)
            .collect();
            
        if nearby_participants.len() < self.minimum_participants {
            return Ok(SwarmJoinResult::InsufficientParticipants {
                found: nearby_participants.len(),
                required: self.minimum_participants,
            });
        }
        
        // Create swarm formation proposal
        let swarm_proposal = SwarmFormationProposal {
            swarm_type: SwarmType::ProximityBased { max_distance },
            proposed_participants: nearby_participants.iter()
                .map(|p| p.participant_id)
                .collect(),
            formation_criteria: SwarmFormationCriteria {
                minimum_order_parameter: 0.7,
                required_capabilities: vec![
                    CapabilityType::SensorFusion,
                    CapabilityType::LocalProcessing,
                ],
                privacy_requirements: PrivacyRequirements::default(),
            },
            proposer: self.get_local_participant_id(),
            proposal_timestamp: SystemTime::now(),
        };
        
        // Send invitations to potential participants
        let mut invitations = Vec::new();
        for participant in nearby_participants {
            let invitation = self.send_swarm_invitation(&participant, &swarm_proposal).await?;
            invitations.push(invitation);
        }
        
        // Wait for responses
        let responses = self.collect_invitation_responses(invitations, Duration::from_secs(30)).await?;
        
        // Form swarm with accepting participants
        let accepting_participants: Vec<_> = responses.into_iter()
            .filter(|r| r.response_type == InvitationResponseType::Accept)
            .map(|r| r.participant_id)
            .collect();
            
        if accepting_participants.len() >= self.minimum_participants {
            let swarm = self.create_swarm(swarm_proposal, accepting_participants).await?;
            let swarm_id = swarm.swarm_id;
            self.active_swarms.insert(swarm_id, swarm);
            
            Ok(SwarmJoinResult::Success { swarm_id })
        } else {
            Ok(SwarmJoinResult::InsufficientAcceptances {
                accepting: accepting_participants.len(),
                required: self.minimum_participants,
            })
        }
    }
    
    /// Create swarm with accepting participants
    async fn create_swarm(
        &self,
        proposal: SwarmFormationProposal,
        participants: Vec<ParticipantId>
    ) -> Result<ResonantSwarm> {
        let swarm_id = SwarmId::new();
        
        // Build coupling matrix based on participant capabilities
        let coupling_matrix = self.build_coupling_matrix(&participants).await?;
        
        // Initialize collective memory
        let collective_memory = CollectiveMemory::new(participants.len());
        
        // Create MiroFish ontology instance
        let ontology_engine = MiroFishOntology::new_shared_instance().await?;
        
        // Set up swarm communication
        let swarm_communication = SwarmCommunication::new(
            swarm_id,
            self.get_local_participant_id(),
            participants.clone()
        ).await?;
        
        let swarm = ResonantSwarm {
            swarm_id,
            swarm_type: proposal.swarm_type,
            formation_time: SystemTime::now(),
            participant_id: self.get_local_participant_id(),
            local_phase: rand::random::<f64>() * 2.0 * PI, // Random initial phase
            local_frequency: 1.0, // Base frequency
            participants: HashMap::new(), // Will be populated as participants join
            coupling_matrix,
            order_parameter: 0.0,
            queen_state: None,
            collective_memory,
            ontology_engine,
            shared_concepts: ConceptSpace::new(),
            swarm_communication,
            consensus_engine: ConsensusEngine::new(),
            swarm_lane: self.spawn_swarm_lane().await?,
            swarm_events: EventBus::new(),
            swarm_config: SwarmConfig::default(),
        };
        
        Ok(swarm)
    }
}

/// Proximity detection for local swarm formation
pub struct ProximityDetector {
    bluetooth_scanner: BluetoothScanner,
    wifi_direct_scanner: WiFiDirectScanner,
    ultrasonic_detector: UltrasonicDetector, // For very close proximity
}

impl ProximityDetector {
    /// Discover nearby devices capable of swarm participation
    pub async fn discover_nearby_devices(&self) -> Result<Vec<NearbyDevice>> {
        let mut nearby_devices = Vec::new();
        
        // Bluetooth Low Energy scanning
        let ble_devices = self.bluetooth_scanner.scan_for_swarm_devices().await?;
        nearby_devices.extend(ble_devices);
        
        // WiFi Direct scanning  
        let wifi_devices = self.wifi_direct_scanner.scan_for_swarm_devices().await?;
        nearby_devices.extend(wifi_devices);
        
        // Ultrasonic proximity detection (very close range)
        let ultrasonic_devices = self.ultrasonic_detector.detect_nearby_phones().await?;
        nearby_devices.extend(ultrasonic_devices);
        
        // Remove duplicates (same device detected via multiple methods)
        let unique_devices = self.deduplicate_devices(nearby_devices);
        
        Ok(unique_devices)
    }
}
```

### Phase 2: MiroFish Ontology Implementation

**Location**: `system/swarm/src/mirofish.rs`

```rust
/// MiroFish ontology for collective intelligence
pub struct MiroFishOntology {
    // Core concepts
    concepts: Arc<RwLock<HashMap<ConceptId, SharedConcept>>>,
    concept_relationships: Arc<RwLock<ConceptGraph>>,
    
    // Inference engine
    distributed_reasoner: DistributedReasoner,
    knowledge_synthesizer: KnowledgeSynthesizer,
    
    // Pattern detection
    emergence_detector: EmergenceDetector,
    coherence_monitor: CoherenceMonitor,
    
    // Synchronization
    concept_synchronizer: ConceptSynchronizer,
    knowledge_merger: KnowledgeMerger,
}

/// Shared concept in the collective concept space
#[derive(Clone, Debug, Serialize, Deserialize)]
pub struct SharedConcept {
    pub concept_id: ConceptId,
    pub concept_name: String,
    pub concept_type: ConceptType,
    
    // Definition
    pub definition: ConceptDefinition,
    pub properties: HashMap<String, ConceptProperty>,
    pub examples: Vec<ConceptExample>,
    
    // Collective formation
    pub contributing_participants: Vec<ParticipantId>,
    pub formation_history: Vec<ConceptFormationEvent>,
    pub confidence_score: f64,
    
    // Relationships
    pub related_concepts: Vec<ConceptRelation>,
    pub semantic_embeddings: Vec<f64>, // High-dimensional representation
    
    // Evolution
    pub creation_time: SystemTime,
    pub last_update: SystemTime,
    pub update_frequency: f64,
}

#[derive(Clone, Debug)]
pub enum ConceptType {
    Entity,          // Objects, things (phone, user, location)
    Relationship,    // Connections between entities
    Process,         // Actions, behaviors, procedures
    Property,        // Attributes, qualities
    Pattern,         // Recurring structures or behaviors
    Emergent,        // Concepts that arise from swarm interaction
}

impl MiroFishOntology {
    /// Merge concepts from multiple participants
    pub async fn merge_participant_concepts(
        &self,
        contributions: HashMap<ParticipantId, Vec<LocalConcept>>
    ) -> Result<Vec<SharedConcept>> {
        let mut merged_concepts = Vec::new();
        
        // Group similar concepts across participants
        let concept_clusters = self.cluster_similar_concepts(&contributions).await?;
        
        for cluster in concept_clusters {
            let merged_concept = self.merge_concept_cluster(cluster).await?;
            merged_concepts.push(merged_concept);
        }
        
        Ok(merged_concepts)
    }
    
    /// Cluster similar concepts from different participants
    async fn cluster_similar_concepts(
        &self,
        contributions: &HashMap<ParticipantId, Vec<LocalConcept>>
    ) -> Result<Vec<ConceptCluster>> {
        let mut all_concepts = Vec::new();
        
        // Collect all concepts with participant attribution
        for (participant_id, concepts) in contributions {
            for concept in concepts {
                all_concepts.push(AttributedConcept {
                    concept: concept.clone(),
                    contributor: *participant_id,
                });
            }
        }
        
        // Compute semantic similarity matrix
        let similarity_matrix = self.compute_concept_similarity_matrix(&all_concepts).await?;
        
        // Cluster using hierarchical clustering
        let clusters = self.hierarchical_clustering(&all_concepts, &similarity_matrix, 0.8).await?;
        
        Ok(clusters)
    }
    
    /// Merge concepts in a cluster into shared concept
    async fn merge_concept_cluster(&self, cluster: ConceptCluster) -> Result<SharedConcept> {
        // Find most representative concept as base
        let base_concept = cluster.concepts.iter()
            .max_by_key(|c| c.concept.confidence_score.total_cmp(&0.0))
            .ok_or(MiroFishError::EmptyCluster)?;
            
        // Merge definitions
        let merged_definition = self.merge_definitions(
            cluster.concepts.iter().map(|c| &c.concept.definition).collect()
        ).await?;
        
        // Merge properties
        let merged_properties = self.merge_properties(
            cluster.concepts.iter().map(|c| &c.concept.properties).collect()
        ).await?;
        
        // Merge examples
        let merged_examples = cluster.concepts.iter()
            .flat_map(|c| c.concept.examples.iter().cloned())
            .collect();
            
        // Compute merged semantic embedding
        let merged_embedding = self.merge_semantic_embeddings(
            cluster.concepts.iter().map(|c| &c.concept.semantic_embedding).collect()
        ).await?;
        
        // Calculate confidence based on participant agreement
        let confidence_score = self.calculate_cluster_confidence(&cluster).await?;
        
        Ok(SharedConcept {
            concept_id: ConceptId::new(),
            concept_name: base_concept.concept.name.clone(),
            concept_type: base_concept.concept.concept_type.clone(),
            definition: merged_definition,
            properties: merged_properties,
            examples: merged_examples,
            contributing_participants: cluster.concepts.iter()
                .map(|c| c.contributor)
                .collect(),
            formation_history: vec![ConceptFormationEvent {
                event_type: FormationEventType::ClusterMerge,
                timestamp: SystemTime::now(),
                contributing_concepts: cluster.concepts.iter()
                    .map(|c| c.concept.concept_id)
                    .collect(),
            }],
            confidence_score,
            related_concepts: Vec::new(), // Will be populated later
            semantic_embeddings: merged_embedding,
            creation_time: SystemTime::now(),
            last_update: SystemTime::now(),
            update_frequency: 0.0,
        })
    }
    
    /// Detect emergent patterns in collective concept space
    pub async fn detect_emergent_patterns(
        &self,
        concepts: &[SharedConcept]
    ) -> Result<Vec<EmergentPattern>> {
        let mut patterns = Vec::new();
        
        // Pattern 1: Concept convergence (multiple participants independently form similar concepts)
        let convergence_patterns = self.detect_convergence_patterns(concepts).await?;
        patterns.extend(convergence_patterns);
        
        // Pattern 2: Relationship emergence (new relationships between existing concepts)
        let relationship_patterns = self.detect_relationship_patterns(concepts).await?;
        patterns.extend(relationship_patterns);
        
        // Pattern 3: Hierarchical emergence (concepts organizing into hierarchies)
        let hierarchy_patterns = self.detect_hierarchy_patterns(concepts).await?;
        patterns.extend(hierarchy_patterns);
        
        // Pattern 4: Conceptual innovation (entirely new concepts from concept combination)
        let innovation_patterns = self.detect_innovation_patterns(concepts).await?;
        patterns.extend(innovation_patterns);
        
        Ok(patterns)
    }
    
    /// Distributed reasoning across swarm participants
    pub async fn distributed_reasoning(&self, query: ReasoningQuery) -> Result<ReasoningResult> {
        match query.reasoning_type {
            ReasoningType::Deductive => {
                self.distributed_deductive_reasoning(query).await
            }
            ReasoningType::Inductive => {
                self.distributed_inductive_reasoning(query).await
            }
            ReasoningType::Abductive => {
                self.distributed_abductive_reasoning(query).await
            }
            ReasoningType::Analogical => {
                self.distributed_analogical_reasoning(query).await
            }
        }
    }
    
    /// Distributed inductive reasoning: pattern discovery across participants
    async fn distributed_inductive_reasoning(&self, query: ReasoningQuery) -> Result<ReasoningResult> {
        // Distribute pattern search across swarm participants
        let pattern_search_tasks = self.distribute_pattern_search(&query).await?;
        
        // Collect partial patterns from participants
        let partial_patterns = self.collect_partial_patterns(pattern_search_tasks).await?;
        
        // Synthesize patterns into general rules
        let general_rules = self.synthesize_general_rules(partial_patterns).await?;
        
        // Validate rules through cross-participant verification
        let validated_rules = self.cross_validate_rules(general_rules).await?;
        
        Ok(ReasoningResult {
            reasoning_type: ReasoningType::Inductive,
            query: query.clone(),
            conclusions: validated_rules,
            confidence: self.compute_reasoning_confidence(&validated_rules).await?,
            contributing_participants: self.get_contributing_participants().await?,
        })
    }
}
```

### Phase 3: QueenSync Protocol Implementation

**Location**: `system/swarm/src/queensync.rs`

```rust
/// QueenSync protocol for swarm coordination
pub struct QueenSyncProtocol {
    // Election state
    election_state: ElectionState,
    voting_system: VotingSystem,
    fitness_evaluator: FitnessEvaluator,
    
    // Queen management
    current_queen: Option<QueenState>,
    queen_performance_monitor: QueenPerformanceMonitor,
    term_manager: TermManager,
    
    // Coordination
    coordination_protocols: HashMap<CoordinationType, Box<dyn CoordinationProtocol>>,
    task_delegator: TaskDelegator,
    
    // Democracy and fairness
    participation_tracker: ParticipationTracker,
    fairness_monitor: FairnessMonitor,
}

/// Current queen state and capabilities
#[derive(Clone, Debug, Serialize, Deserialize)]
pub struct QueenState {
    pub queen_participant_id: ParticipantId,
    pub election_timestamp: SystemTime,
    pub term_duration: Duration,
    pub fitness_score: f64,
    
    // Coordination capabilities
    pub coordination_strength: f64,    // How well this queen coordinates
    pub decision_authority: f64,       // How much decision power queen has
    pub resource_control: f64,         // Control over swarm resources
    
    // Performance tracking
    pub decisions_made: u32,
    pub coordination_success_rate: f64,
    pub participant_satisfaction: f64,
    
    // Term management
    pub term_start: SystemTime,
    pub term_end: SystemTime,
    pub reelection_eligibility: bool,
}

impl QueenSyncProtocol {
    /// Initiate queen election process
    pub async fn initiate_election(&mut self, election_trigger: ElectionTrigger) -> Result<ElectionResult> {
        log::info!("Initiating queen election: {:?}", election_trigger);
        
        // Transition to election state
        self.election_state = ElectionState::Nominating;
        
        // Open nomination period
        let nomination_period = self.open_nomination_period().await?;
        
        // Collect nominations from participants
        let nominations = self.collect_nominations(nomination_period).await?;
        
        // Evaluate nominee fitness
        let evaluated_nominees = self.evaluate_nominee_fitness(nominations).await?;
        
        // Conduct voting
        let voting_results = self.conduct_voting(evaluated_nominees).await?;
        
        // Determine winner
        let election_winner = self.determine_election_winner(voting_results).await?;
        
        // Transition queen state
        self.transition_to_new_queen(election_winner).await?;
        
        Ok(ElectionResult {
            new_queen: election_winner,
            election_timestamp: SystemTime::now(),
            voter_turnout: self.calculate_voter_turnout().await?,
            election_legitimacy: self.evaluate_election_legitimacy().await?,
        })
    }
    
    /// Evaluate participant fitness for queen role
    pub async fn evaluate_queen_fitness(&self, participant: &SwarmParticipant) -> Result<QueenFitness> {
        let mut fitness_score = 0.0;
        let mut fitness_breakdown = FitnessBreakdown::default();
        
        // Technical capability (30%)
        let technical_score = self.evaluate_technical_capability(participant).await?;
        fitness_score += 0.3 * technical_score;
        fitness_breakdown.technical_capability = technical_score;
        
        // Resource availability (25%)
        let resource_score = self.evaluate_resource_availability(participant).await?;
        fitness_score += 0.25 * resource_score;
        fitness_breakdown.resource_availability = resource_score;
        
        // Coordination experience (20%)
        let experience_score = self.evaluate_coordination_experience(participant).await?;
        fitness_score += 0.2 * experience_score;
        fitness_breakdown.coordination_experience = experience_score;
        
        // Reliability (15%)
        let reliability_score = self.evaluate_reliability(participant).await?;
        fitness_score += 0.15 * reliability_score;
        fitness_breakdown.reliability = reliability_score;
        
        // Network position (10%)
        let centrality_score = self.evaluate_network_centrality(participant).await?;
        fitness_score += 0.1 * centrality_score;
        fitness_breakdown.network_centrality = centrality_score;
        
        Ok(QueenFitness {
            participant_id: participant.participant_id,
            overall_score: fitness_score,
            breakdown: fitness_breakdown,
            evaluation_timestamp: SystemTime::now(),
        })
    }
    
    /// Queen coordination of swarm activities
    pub async fn coordinate_swarm_activity(&mut self, activity: SwarmActivity) -> Result<CoordinationResult> {
        let queen_state = self.current_queen.as_ref()
            .ok_or(QueenSyncError::NoActiveQueen)?;
            
        match activity {
            SwarmActivity::CollectiveProblemSolving(problem) => {
                self.coordinate_problem_solving(problem).await
            }
            SwarmActivity::ResourceAllocation(allocation_request) => {
                self.coordinate_resource_allocation(allocation_request).await
            }
            SwarmActivity::ConsensusPrediction(prediction_task) => {
                self.coordinate_consensus_prediction(prediction_task).await
            }
            SwarmActivity::KnowledgeSynthesis(synthesis_task) => {
                self.coordinate_knowledge_synthesis(synthesis_task).await
            }
        }
    }
    
    /// Coordinate collective problem solving
    async fn coordinate_problem_solving(&self, problem: CollectiveProblem) -> Result<CoordinationResult> {
        // Analyze problem complexity
        let complexity_analysis = self.analyze_problem_complexity(&problem).await?;
        
        // Determine optimal decomposition strategy
        let decomposition_strategy = self.determine_decomposition_strategy(&complexity_analysis).await?;
        
        // Assign subproblems to participants based on capabilities
        let task_assignments = self.assign_problem_tasks(&problem, decomposition_strategy).await?;
        
        // Monitor progress and provide coordination
        let progress_monitor = self.start_progress_monitoring(task_assignments.clone()).await?;
        
        // Coordinate intermediate synchronization points
        let sync_checkpoints = self.coordinate_synchronization_checkpoints().await?;
        
        // Synthesize solutions when ready
        let solution_synthesis = self.coordinate_solution_synthesis(task_assignments).await?;
        
        Ok(CoordinationResult {
            activity_type: SwarmActivityType::ProblemSolving,
            coordination_success: solution_synthesis.success,
            participant_contributions: task_assignments.keys().cloned().collect(),
            coordination_efficiency: self.calculate_coordination_efficiency(&progress_monitor).await?,
            outcome: CoordinationOutcome::ProblemSolution(solution_synthesis.solution),
        })
    }
    
    /// Monitor queen performance and trigger reelection if needed
    pub async fn monitor_queen_performance(&mut self) -> Result<PerformanceAssessment> {
        let queen_state = self.current_queen.as_ref()
            .ok_or(QueenSyncError::NoActiveQueen)?;
            
        // Collect performance metrics
        let performance_metrics = self.queen_performance_monitor.collect_metrics().await?;
        
        // Assess coordination effectiveness
        let coordination_effectiveness = self.assess_coordination_effectiveness().await?;
        
        // Measure participant satisfaction
        let participant_satisfaction = self.measure_participant_satisfaction().await?;
        
        // Check for performance issues
        let performance_issues = self.identify_performance_issues(&performance_metrics).await?;
        
        let assessment = PerformanceAssessment {
            queen_participant: queen_state.queen_participant_id,
            assessment_timestamp: SystemTime::now(),
            performance_score: performance_metrics.overall_score,
            coordination_effectiveness,
            participant_satisfaction,
            identified_issues: performance_issues.clone(),
            recommendation: if performance_issues.is_empty() {
                PerformanceRecommendation::Continue
            } else if performance_issues.iter().any(|i| matches!(i, PerformanceIssue::Critical(_))) {
                PerformanceRecommendation::ImmediateReelection
            } else {
                PerformanceRecommendation::MonitorClosely
            },
        };
        
        // Trigger reelection if performance is critically poor
        if matches!(assessment.recommendation, PerformanceRecommendation::ImmediateReelection) {
            log::warn!("Triggering queen reelection due to poor performance");
            self.initiate_election(ElectionTrigger::PerformanceFailure).await?;
        }
        
        Ok(assessment)
    }
}
```

---

## Consequences

### Positive

1. **Collective Intelligence**: Swarm capabilities exceed individual phone capabilities through collaboration
2. **Distributed Processing**: Complex problems solved in parallel across multiple phones
3. **Emergent Coordination**: QueenSync protocol enables self-organizing hierarchies without central control
4. **Resilient Operations**: Swarm continues functioning when individual phones leave or fail
5. **Shared Context**: Multiple phones create richer environmental understanding through sensor fusion
6. **Democratic Governance**: Fair, merit-based election of temporary coordinators
7. **Knowledge Synthesis**: MiroFish ontology enables collective concept formation and reasoning

### Negative

1. **Coordination Overhead**: Swarm synchronization and communication requires significant processing and bandwidth
2. **Privacy Concerns**: Participating in swarms requires sharing some data with other phones
3. **Battery Consumption**: Continuous swarm participation affects battery life
4. **Network Dependency**: Swarm coordination requires reliable network connectivity between phones
5. **Complexity**: Swarm behavior is more complex and harder to predict than individual operation

### Neutral

1. **Scale Effects**: Swarm benefits increase with more participants but coordination costs also increase
2. **Context Dependency**: Some tasks benefit greatly from swarms while others are better done individually
3. **Learning Curve**: Users need to understand when and how swarm participation benefits them

---

## Implementation Timeline

### Phase 1 (Weeks 1-3): Swarm Discovery and Formation
- Implement proximity detection via Bluetooth/WiFi Direct/Ultrasonic
- Create Flux-based swarm discovery system
- Build swarm formation protocols and invitation system

### Phase 2 (Weeks 4-6): Kuramoto Synchronization
- Extend Kuramoto sensor fusion to inter-phone synchronization
- Implement phase coupling and order parameter computation
- Create swarm synchronization monitoring and optimization

### Phase 3 (Weeks 7-9): MiroFish Ontology
- Build distributed concept formation and merging system
- Implement collective reasoning and pattern detection
- Create emergent knowledge synthesis capabilities

### Phase 4 (Weeks 10-12): QueenSync Protocol
- Implement democratic election system for swarm coordination
- Build queen performance monitoring and reelection mechanisms
- Create task delegation and coordination protocols

### Phase 5 (Weeks 13-15): Integration & Optimization
- Integrate with prediction markets for collective decision-making
- Add privacy controls and selective participation mechanisms
- Optimize battery usage and network efficiency for swarm operations

---

## References

- [resonant-swarm Research](../../../resonant-swarm/) — Original swarm intelligence research
- [MiroFish Ontology](https://en.wikipedia.org/wiki/MiroFish) — Collective intelligence framework
- [Kuramoto Model](https://en.wikipedia.org/wiki/Kuramoto_model) — Synchronization mathematics
- [Swarm Intelligence](https://www.amazon.com/Swarm-Intelligence-Artificial-Collective-Intelligence/dp/0195131592) — Theoretical foundation
- [Collective Intelligence](https://www.amazon.com/Collective-Intelligence-Creating-Connected-World/dp/0262516608) — Organizational principles
- [emergent-queen-state Papers](../../../resonant-swarm/papers/) — QueenSync protocol research
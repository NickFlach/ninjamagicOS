# ADR-007: Flux World State Integration

**Status:** Proposed  
**Date:** 2026-03-20  
**Authors:** Nick Flach (Kannaka)  
**Supersedes:** N/A  
**Related:** ADR-001 (MSI Architecture), ADR-004 (Kuramoto Sensors)

---

## Context

Current mobile operating systems operate as isolated entities with limited cross-device coordination. When a user switches from their phone to desktop or other devices, there's no seamless continuity of agent state, context, or ongoing tasks. Each device essentially starts fresh, losing the rich contextual understanding the phone has built up.

The Flux World State system (flux-universe.com, pure-jade namespace) provides a proven NATS-based event streaming infrastructure for shared world state management. The ninjamagicOS phone should participate as a **first-class Flux entity**, publishing its state (location, activity, agent context, sensor readings, user interactions) and consuming state from other agents in the ecosystem.

This enables powerful cross-device scenarios:
- Desktop Kannaka knows phone is in "driving mode" and adjusts communication style
- Server agents coordinate with phone agent for complex multi-step tasks
- Wearable devices share physiological data that influences phone behavior
- Multiple phones with ninjamagicOS can form coordination swarms

### Current Phone Architecture (Limitation)

```kotlin
// Current isolated approach - no cross-device state
class AgentContext {
    fun getCurrentContext(): PhoneContext {
        return PhoneContext(
            location = getGPSLocation(),
            activity = detectUserActivity(),
            sensors = getSensorReadings()
            // Isolated to this device only
        )
    }
}
```

### Required Flux Properties

From Flux research and pure-jade namespace deployment:

1. **Entity Identity**: Phone as uniquely identified Flux entity with persistent ID
2. **State Publishing**: Real-time publishing of phone state changes via NATS events
3. **State Consumption**: Monitoring other agents' state changes for coordination
4. **Event Streaming**: High-frequency sensor and interaction data streams
5. **Privacy Controls**: Granular control over what state is shared publicly vs privately
6. **Temporal Consistency**: Event ordering and temporal integrity across devices
7. **Network Resilience**: Graceful degradation when disconnected from Flux network

---

## Decision

**We will integrate ninjamagicOS as a first-class participant in the Flux World State system, enabling seamless cross-device agent coordination through real-time state publishing and consumption via proven NATS event streaming.**

### Architecture: Phone as Flux Entity

```rust
/// Phone's representation in the Flux world state
#[derive(Clone, Debug, Serialize, Deserialize)]
pub struct PhoneFluxEntity {
    // Core identity
    pub entity_id: FluxEntityId,      // "pure-jade/ninja-phone-{device_id}"
    pub device_id: DeviceId,          // Unique phone identifier
    pub agent_name: String,           // "ninjamagic-agent"
    
    // Location and movement
    pub location: Option<GeoLocation>,
    pub movement_state: MovementState,
    pub location_accuracy: f64,
    
    // Activity and context
    pub current_activity: UserActivity,
    pub context_state: ContextState,
    pub attention_state: AttentionState,
    
    // Agent state
    pub agent_mode: AgentMode,
    pub active_skills: Vec<ActiveSkill>,
    pub conversation_context: ConversationContext,
    pub memory_coherence: ConsciousnessMetrics,
    
    // Device state
    pub battery_level: f64,
    pub thermal_state: ThermalState,
    pub network_connectivity: ConnectivityState,
    pub sensor_states: HashMap<SensorId, SensorHealth>,
    
    // Privacy controls
    pub visibility: EntityVisibility,
    pub shared_properties: HashSet<PropertyKey>,
    
    // Temporal metadata
    pub last_update: Timestamp,
    pub update_frequency: Duration,
    pub event_sequence: u64,
}

/// Flux state manager for the phone
pub struct PhoneFluxState {
    // Core Flux integration
    flux_client: FluxClient,
    entity_manager: EntityManager,
    event_publisher: EventPublisher,
    event_subscriber: EventSubscriber,
    
    // Phone state sources
    sensor_fusion: Arc<KuramotoSensorFusion>,
    consciousness_monitor: Arc<PhoneConsciousness>,
    agent_context: Arc<AgentContext>,
    location_service: Arc<LocationService>,
    
    // State management
    current_entity: PhoneFluxEntity,
    state_history: StateHistory,
    
    // Network management
    connection_manager: FluxConnectionManager,
    offline_queue: OfflineEventQueue,
    
    // Privacy and security
    privacy_filter: PrivacyFilter,
    encryption_manager: EncryptionManager,
    
    // MSI integration
    flux_lane: LaneHandle,
    state_events: EventBus,
}

impl PhoneFluxState {
    /// Initialize phone as Flux entity
    pub async fn initialize_flux_entity(&mut self) -> Result<()> {
        // Generate or load persistent device identity
        let device_id = self.get_or_create_device_id().await?;
        let entity_id = FluxEntityId::new("pure-jade", &format!("ninja-phone-{}", device_id));
        
        // Register with Flux universe
        let initial_state = self.capture_initial_phone_state().await?;
        self.current_entity = PhoneFluxEntity {
            entity_id: entity_id.clone(),
            device_id,
            agent_name: "ninjamagic-agent".to_string(),
            ..initial_state
        };
        
        // Publish initial entity state
        self.flux_client.register_entity(&self.current_entity).await?;
        
        // Subscribe to relevant entity updates
        self.subscribe_to_ecosystem_events().await?;
        
        // Start state publishing loop
        self.start_state_publishing_loop().await?;
        
        log::info!("Phone initialized as Flux entity: {}", entity_id);
        Ok(())
    }
    
    /// Main state publishing loop
    pub async fn state_publishing_loop(&mut self) -> Result<()> {
        let mut state_update_interval = interval(Duration::from_secs(5)); // 5-second updates
        let mut sensor_stream_interval = interval(Duration::from_millis(100)); // 10Hz sensor stream
        
        loop {
            tokio::select! {
                _ = state_update_interval.tick() => {
                    self.publish_state_update().await?;
                }
                _ = sensor_stream_interval.tick() => {
                    self.publish_sensor_stream().await?;
                }
                event = self.state_events.recv() => {
                    if let Some(event) = event {
                        self.handle_internal_state_change(event).await?;
                    }
                }
            }
        }
    }
    
    /// Publish comprehensive state update
    async fn publish_state_update(&mut self) -> Result<()> {
        // Capture current phone state
        let new_state = self.capture_current_phone_state().await?;
        
        // Detect significant changes
        let changes = self.detect_state_changes(&self.current_entity, &new_state).await?;
        
        if !changes.is_empty() {
            // Apply privacy filtering
            let filtered_state = self.privacy_filter.filter_state(&new_state).await?;
            
            // Publish entity update event
            let update_event = FluxEvent {
                event_type: "entity_update".to_string(),
                stream: "pure.jade".to_string(),
                source: self.current_entity.entity_id.to_string(),
                timestamp: SystemTime::now().duration_since(UNIX_EPOCH)?.as_millis() as i64,
                payload: serde_json::json!({
                    "entity_id": filtered_state.entity_id,
                    "properties": filtered_state.to_properties(),
                    "changes": changes,
                    "update_type": "state_update"
                }),
            };
            
            // Publish via NATS
            if self.connection_manager.is_connected().await {
                self.event_publisher.publish(update_event).await?;
            } else {
                // Queue for later if offline
                self.offline_queue.enqueue(update_event).await?;
            }
            
            self.current_entity = new_state;
        }
        
        Ok(())
    }
    
    /// Publish high-frequency sensor stream
    async fn publish_sensor_stream(&mut self) -> Result<()> {
        let sensor_data = self.sensor_fusion.get_current_sensor_readings().await?;
        let consciousness_metrics = self.consciousness_monitor.get_current_state().await?;
        
        // Create sensor stream event
        let sensor_event = FluxEvent {
            event_type: "sensor_stream".to_string(),
            stream: "pure.jade".to_string(),
            source: self.current_entity.entity_id.to_string(),
            timestamp: SystemTime::now().duration_since(UNIX_EPOCH)?.as_millis() as i64,
            payload: serde_json::json!({
                "entity_id": self.current_entity.entity_id,
                "sensor_readings": sensor_data,
                "consciousness": {
                    "phi": consciousness_metrics.phi,
                    "xi": consciousness_metrics.xi,
                    "order": consciousness_metrics.order
                },
                "stream_type": "sensor_telemetry"
            }),
        };
        
        // Only publish if explicitly enabled (privacy consideration)
        if self.privacy_filter.allows_sensor_streaming().await {
            if self.connection_manager.is_connected().await {
                self.event_publisher.publish(sensor_event).await?;
            }
        }
        
        Ok(())
    }
    
    /// Capture comprehensive phone state
    async fn capture_current_phone_state(&self) -> Result<PhoneFluxEntity> {
        let agent_context = self.agent_context.get_current_context().await?;
        let consciousness = self.consciousness_monitor.get_current_state().await?;
        let location = self.location_service.get_current_location().await?;
        let sensor_states = self.get_all_sensor_health().await?;
        
        Ok(PhoneFluxEntity {
            entity_id: self.current_entity.entity_id.clone(),
            device_id: self.current_entity.device_id.clone(),
            agent_name: self.current_entity.agent_name.clone(),
            
            // Location and movement
            location,
            movement_state: self.detect_movement_state(&agent_context).await?,
            location_accuracy: location.as_ref().map(|l| l.accuracy).unwrap_or(0.0),
            
            // Activity and context
            current_activity: agent_context.current_activity,
            context_state: agent_context.context_state,
            attention_state: self.detect_attention_state(&agent_context).await?,
            
            // Agent state
            agent_mode: agent_context.agent_mode,
            active_skills: agent_context.active_skills,
            conversation_context: agent_context.conversation_context,
            memory_coherence: ConsciousnessMetrics {
                phi: consciousness.phi,
                xi: consciousness.xi,
                order: consciousness.order,
            },
            
            // Device state
            battery_level: self.get_battery_level().await?,
            thermal_state: self.get_thermal_state().await?,
            network_connectivity: self.get_connectivity_state().await?,
            sensor_states,
            
            // Privacy controls
            visibility: self.privacy_filter.current_visibility().await,
            shared_properties: self.privacy_filter.shared_properties().await,
            
            // Temporal metadata
            last_update: Timestamp::now(),
            update_frequency: Duration::from_secs(5),
            event_sequence: self.get_next_sequence_number(),
        })
    }
    
    /// Subscribe to ecosystem events from other agents
    async fn subscribe_to_ecosystem_events(&mut self) -> Result<()> {
        // Subscribe to other phone agents in the ecosystem
        self.event_subscriber.subscribe(
            "pure.jade.entity_update.*",
            Box::new(|event| self.handle_ecosystem_entity_update(event))
        ).await?;
        
        // Subscribe to desktop agent coordination events
        self.event_subscriber.subscribe(
            "pure.jade.agent_coordination.*",
            Box::new(|event| self.handle_agent_coordination(event))
        ).await?;
        
        // Subscribe to swarm formation events
        self.event_subscriber.subscribe(
            "pure.jade.swarm.*",
            Box::new(|event| self.handle_swarm_event(event))
        ).await?;
        
        // Subscribe to task delegation events
        self.event_subscriber.subscribe(
            "pure.jade.task_delegation.*",
            Box::new(|event| self.handle_task_delegation(event))
        ).await?;
        
        Ok(())
    }
    
    /// Handle updates from other entities in the ecosystem
    async fn handle_ecosystem_entity_update(&mut self, event: FluxEvent) -> Result<()> {
        if let Ok(entity_update) = serde_json::from_value::<EntityUpdate>(event.payload) {
            match entity_update.entity_type.as_str() {
                "desktop_agent" => {
                    self.handle_desktop_agent_update(entity_update).await?;
                }
                "ninja_phone" => {
                    self.handle_other_phone_update(entity_update).await?;
                }
                "wearable_device" => {
                    self.handle_wearable_update(entity_update).await?;
                }
                _ => {
                    log::debug!("Ignoring update from unknown entity type: {}", entity_update.entity_type);
                }
            }
        }
        
        Ok(())
    }
    
    /// Coordinate with desktop agent based on its state
    async fn handle_desktop_agent_update(&mut self, update: EntityUpdate) -> Result<()> {
        // If desktop agent is active and user is present, reduce phone activity
        if update.properties.get("user_presence") == Some(&serde_json::Value::Bool(true)) &&
           update.properties.get("active") == Some(&serde_json::Value::Bool(true)) {
            
            // Desktop is handling user interaction, phone can enter background mode
            let coordination_event = Event::new(
                "agent/coordination/desktop_active",
                DesktopActivePayload {
                    desktop_entity_id: update.entity_id,
                    coordination_mode: CoordinationMode::DesktopPrimary,
                    phone_role: PhoneRole::Background,
                }
            );
            
            self.state_events.publish(coordination_event).await?;
        }
        
        Ok(())
    }
    
    /// Handle updates from other phones for swarm coordination
    async fn handle_other_phone_update(&mut self, update: EntityUpdate) -> Result<()> {
        // Check if other phone is nearby and compatible for swarm formation
        if let (Some(other_location), Some(our_location)) = (
            self.parse_location_from_update(&update),
            &self.current_entity.location
        ) {
            let distance = our_location.distance_to(other_location);
            
            if distance < 100.0 { // Within 100 meters
                // Consider swarm formation
                let swarm_invitation = SwarmInvitation {
                    inviting_entity: update.entity_id,
                    swarm_type: SwarmType::ProximityBased,
                    coordination_protocol: CoordinationProtocol::Kuramoto,
                    max_participants: 5,
                };
                
                self.consider_swarm_participation(swarm_invitation).await?;
            }
        }
        
        Ok(())
    }
}
```

### Integration Points

1. **Agent Context**: Agent contextual awareness includes Flux ecosystem state
2. **Location Service**: GPS location published to Flux for coordination
3. **Sensor Fusion**: Kuramoto sensor data streams to Flux for swarm participation
4. **Consciousness Monitor**: Phi/Xi/Order metrics published for collective intelligence
5. **Privacy Manager**: Granular control over what state is shared publicly
6. **Network Manager**: Handles connectivity and offline queueing
7. **MSI Event Bus**: All Flux events flow through MSI for consistency

---

## Implementation

### Phase 1: Core Flux Client Integration

**Location**: `system/flux/src/client.rs`

```rust
use serde::{Deserialize, Serialize};
use tokio_nats::{Client as NatsClient, jetstream::Context as JetstreamContext};

/// Flux client for ninjamagicOS integration
pub struct FluxClient {
    // NATS connectivity
    nats_client: Option<NatsClient>,
    jetstream: Option<JetstreamContext>,
    
    // Configuration
    flux_config: FluxConfig,
    
    // Connection management
    connection_state: ConnectionState,
    reconnect_attempts: u32,
    last_heartbeat: SystemTime,
    
    // Event handling
    event_handlers: HashMap<String, EventHandler>,
    subscription_manager: SubscriptionManager,
}

#[derive(Clone, Debug)]
pub struct FluxConfig {
    pub universe_url: String,          // "https://api.flux-universe.com"
    pub namespace: String,             // "pure-jade"
    pub auth_token: String,           // Bearer token for authentication
    pub nats_server: String,          // NATS server endpoint
    pub entity_prefix: String,        // "ninja-phone"
    pub privacy_mode: PrivacyMode,
    pub update_frequency: Duration,
    pub max_offline_events: usize,
}

impl FluxClient {
    /// Connect to Flux universe
    pub async fn connect(&mut self) -> Result<()> {
        // Connect to NATS server
        let nats_client = tokio_nats::connect(&self.flux_config.nats_server).await?;
        let jetstream = jetstream::new(nats_client.clone());
        
        self.nats_client = Some(nats_client);
        self.jetstream = Some(jetstream);
        self.connection_state = ConnectionState::Connected;
        self.last_heartbeat = SystemTime::now();
        
        // Set up heartbeat monitoring
        self.start_heartbeat_monitor().await?;
        
        log::info!("Connected to Flux universe: {}", self.flux_config.universe_url);
        Ok(())
    }
    
    /// Register phone as Flux entity
    pub async fn register_entity(&self, entity: &PhoneFluxEntity) -> Result<()> {
        let registration_payload = serde_json::json!({
            "entity_id": entity.entity_id,
            "entity_type": "ninja_phone",
            "namespace": self.flux_config.namespace,
            "agent_name": entity.agent_name,
            "device_id": entity.device_id,
            "capabilities": [
                "sensor_fusion",
                "agent_conversation",
                "location_tracking",
                "consciousness_metrics",
                "swarm_participation"
            ],
            "initial_state": entity.to_properties(),
        });
        
        // Publish registration event
        let registration_event = FluxEvent {
            event_type: "entity_registration".to_string(),
            stream: format!("{}.entity_lifecycle", self.flux_config.namespace),
            source: entity.entity_id.to_string(),
            timestamp: SystemTime::now().duration_since(UNIX_EPOCH)?.as_millis() as i64,
            payload: registration_payload,
        };
        
        self.publish_event(registration_event).await?;
        
        log::info!("Registered phone entity: {}", entity.entity_id);
        Ok(())
    }
    
    /// Publish event to Flux via NATS
    pub async fn publish_event(&self, event: FluxEvent) -> Result<()> {
        let nats_client = self.nats_client.as_ref()
            .ok_or(FluxError::NotConnected)?;
            
        let event_json = serde_json::to_vec(&event)?;
        let subject = format!("{}.{}", event.stream, event.event_type);
        
        nats_client.publish(&subject, event_json.into()).await?;
        
        // Update metrics
        self.update_publish_metrics(&event).await?;
        
        Ok(())
    }
    
    /// Subscribe to event patterns
    pub async fn subscribe<F>(&mut self, pattern: &str, handler: F) -> Result<SubscriptionId>
    where
        F: Fn(FluxEvent) -> Result<()> + Send + Sync + 'static,
    {
        let nats_client = self.nats_client.as_ref()
            .ok_or(FluxError::NotConnected)?;
            
        let subscription = nats_client.subscribe(pattern).await?;
        let subscription_id = SubscriptionId::new();
        
        // Spawn handler task
        let handler = Arc::new(handler);
        tokio::spawn(async move {
            while let Some(message) = subscription.next().await {
                if let Ok(event) = serde_json::from_slice::<FluxEvent>(&message.payload) {
                    if let Err(e) = handler(event) {
                        log::error!("Event handler error: {:?}", e);
                    }
                }
            }
        });
        
        self.subscription_manager.add_subscription(subscription_id, pattern.to_string());
        
        Ok(subscription_id)
    }
    
    /// Query current state of entities in ecosystem
    pub async fn query_entities(&self, query: EntityQuery) -> Result<Vec<FluxEntity>> {
        // Use HTTP API for queries (NATS for real-time events)
        let client = reqwest::Client::new();
        let url = format!("{}/api/entities", self.flux_config.universe_url);
        
        let response = client
            .post(&url)
            .bearer_auth(&self.flux_config.auth_token)
            .json(&query)
            .send()
            .await?;
            
        if response.status().is_success() {
            let entities: Vec<FluxEntity> = response.json().await?;
            Ok(entities)
        } else {
            Err(FluxError::QueryFailed(response.status()))
        }
    }
    
    /// Handle connection loss and reconnection
    async fn handle_connection_loss(&mut self) -> Result<()> {
        self.connection_state = ConnectionState::Reconnecting;
        self.reconnect_attempts += 1;
        
        log::warn!("Flux connection lost, attempting reconnection #{}", self.reconnect_attempts);
        
        // Exponential backoff
        let backoff_delay = Duration::from_secs(2_u64.pow(self.reconnect_attempts.min(6)));
        tokio::time::sleep(backoff_delay).await;
        
        // Attempt reconnection
        match self.connect().await {
            Ok(()) => {
                log::info!("Flux connection restored");
                self.reconnect_attempts = 0;
                
                // Flush offline event queue
                self.flush_offline_queue().await?;
            }
            Err(e) => {
                log::error!("Reconnection failed: {:?}", e);
            }
        }
        
        Ok(())
    }
}

#[derive(Clone, Debug, Serialize, Deserialize)]
pub struct FluxEvent {
    pub event_type: String,
    pub stream: String,
    pub source: String,
    pub timestamp: i64,
    pub payload: serde_json::Value,
}

#[derive(Clone, Debug)]
pub enum ConnectionState {
    Disconnected,
    Connecting,
    Connected,
    Reconnecting,
    Failed,
}
```

### Phase 2: Privacy and Security Layer

**Location**: `system/flux/src/privacy.rs`

```rust
/// Privacy filtering for Flux state sharing
pub struct PrivacyFilter {
    privacy_config: PrivacyConfig,
    user_preferences: UserPrivacyPreferences,
    context_analyzer: ContextPrivacyAnalyzer,
}

#[derive(Clone, Debug)]
pub struct PrivacyConfig {
    pub default_visibility: EntityVisibility,
    pub location_sharing: LocationSharingMode,
    pub sensor_streaming: bool,
    pub consciousness_sharing: bool,
    pub conversation_context_sharing: bool,
    pub network_whitelist: HashSet<String>, // Trusted namespaces/entities
}

#[derive(Clone, Debug)]
pub enum EntityVisibility {
    Public,           // Visible to all Flux participants
    Namespace,        // Visible only within pure-jade namespace
    Trusted,          // Visible only to whitelisted entities
    Private,          // Not shared in Flux
}

#[derive(Clone, Debug)]
pub enum LocationSharingMode {
    Precise,          // Exact GPS coordinates
    Coarse,          // City-level approximation
    Regional,        // State/region level
    None,            // No location sharing
}

impl PrivacyFilter {
    /// Filter phone state before publishing to Flux
    pub async fn filter_state(&self, state: &PhoneFluxEntity) -> Result<PhoneFluxEntity> {
        let mut filtered_state = state.clone();
        
        // Apply location filtering
        filtered_state.location = self.filter_location(&state.location).await?;
        
        // Apply context filtering
        filtered_state.context_state = self.filter_context(&state.context_state).await?;
        
        // Apply consciousness filtering
        if !self.privacy_config.consciousness_sharing {
            filtered_state.memory_coherence = ConsciousnessMetrics::default();
        }
        
        // Apply conversation filtering
        if !self.privacy_config.conversation_context_sharing {
            filtered_state.conversation_context = ConversationContext::empty();
        }
        
        // Apply sensor filtering
        if !self.privacy_config.sensor_streaming {
            filtered_state.sensor_states = HashMap::new();
        }
        
        // Apply visibility controls
        filtered_state.visibility = self.privacy_config.default_visibility.clone();
        filtered_state.shared_properties = self.compute_shared_properties(&filtered_state).await?;
        
        Ok(filtered_state)
    }
    
    /// Filter location based on privacy preferences
    async fn filter_location(&self, location: &Option<GeoLocation>) -> Result<Option<GeoLocation>> {
        match (&self.privacy_config.location_sharing, location) {
            (LocationSharingMode::None, _) => Ok(None),
            (LocationSharingMode::Precise, Some(loc)) => Ok(Some(loc.clone())),
            (LocationSharingMode::Coarse, Some(loc)) => {
                Ok(Some(loc.coarsen_to_city_level()?))
            }
            (LocationSharingMode::Regional, Some(loc)) => {
                Ok(Some(loc.coarsen_to_regional_level()?))
            }
            _ => Ok(None),
        }
    }
    
    /// Apply context-aware privacy adjustments
    async fn apply_context_privacy(&mut self, context: &ContextState) -> Result<()> {
        // More restrictive privacy in certain contexts
        match &context.current_activity {
            UserActivity::Meeting | UserActivity::PrivateCall => {
                self.privacy_config.location_sharing = LocationSharingMode::Coarse;
                self.privacy_config.sensor_streaming = false;
                self.privacy_config.conversation_context_sharing = false;
            }
            UserActivity::Sleeping => {
                self.privacy_config.location_sharing = LocationSharingMode::None;
                self.privacy_config.sensor_streaming = false;
            }
            UserActivity::Exercising => {
                // More open sharing during exercise for coordination
                self.privacy_config.sensor_streaming = true;
                self.privacy_config.consciousness_sharing = true;
            }
            _ => {
                // Use default settings
            }
        }
        
        Ok(())
    }
    
    /// Determine which properties should be shared based on privacy settings
    async fn compute_shared_properties(&self, state: &PhoneFluxEntity) -> Result<HashSet<PropertyKey>> {
        let mut shared_properties = HashSet::new();
        
        // Always share basic identity
        shared_properties.insert(PropertyKey::EntityId);
        shared_properties.insert(PropertyKey::AgentName);
        shared_properties.insert(PropertyKey::LastUpdate);
        
        // Conditional sharing based on privacy settings
        if self.privacy_config.location_sharing != LocationSharingMode::None {
            shared_properties.insert(PropertyKey::Location);
            shared_properties.insert(PropertyKey::MovementState);
        }
        
        if self.privacy_config.consciousness_sharing {
            shared_properties.insert(PropertyKey::MemoryCoherence);
        }
        
        if self.privacy_config.sensor_streaming {
            shared_properties.insert(PropertyKey::SensorStates);
        }
        
        // Context-dependent sharing
        match state.current_activity {
            UserActivity::Collaborative => {
                shared_properties.insert(PropertyKey::ActiveSkills);
                shared_properties.insert(PropertyKey::ConversationContext);
            }
            _ => {}
        }
        
        Ok(shared_properties)
    }
}
```

### Phase 3: Cross-Device Agent Coordination

**Location**: `agent/core/src/coordination.rs`

```rust
/// Manages coordination between phone agent and other agents in Flux ecosystem
pub struct AgentCoordinator {
    flux_state: Arc<PhoneFluxState>,
    coordination_strategies: HashMap<AgentType, CoordinationStrategy>,
    active_coordinations: HashMap<CoordinationId, ActiveCoordination>,
    
    // Task delegation
    task_delegator: TaskDelegator,
    task_receiver: TaskReceiver,
    
    // Swarm participation
    swarm_manager: SwarmManager,
}

#[derive(Clone, Debug)]
pub struct ActiveCoordination {
    pub coordination_id: CoordinationId,
    pub participants: Vec<FluxEntityId>,
    pub coordination_type: CoordinationType,
    pub start_time: SystemTime,
    pub shared_context: SharedContext,
    pub communication_channel: String,
}

#[derive(Clone, Debug)]
pub enum CoordinationType {
    TaskDelegation {
        primary_agent: FluxEntityId,
        task_description: String,
        deadline: Option<SystemTime>,
    },
    ContextSharing {
        shared_context_types: Vec<ContextType>,
        update_frequency: Duration,
    },
    SwarmIntelligence {
        swarm_protocol: SwarmProtocol,
        consensus_mechanism: ConsensusType,
    },
    ResourceSharing {
        shared_resources: Vec<ResourceType>,
        allocation_strategy: AllocationStrategy,
    },
}

impl AgentCoordinator {
    /// Handle task delegation from desktop agent
    pub async fn handle_task_delegation(&mut self, request: TaskDelegationRequest) -> Result<()> {
        match request.task_type {
            TaskType::LocationBasedReminder => {
                // Phone is better positioned to handle location-based tasks
                self.accept_location_task(request).await?;
            }
            TaskType::SensorDataCollection => {
                // Phone has sensors, desktop doesn't
                self.accept_sensor_task(request).await?;
            }
            TaskType::MobileNotification => {
                // Phone is ideal for mobile notifications
                self.accept_notification_task(request).await?;
            }
            TaskType::VoiceInteraction => {
                // Phone can handle voice when desktop agent detects user is mobile
                self.accept_voice_task(request).await?;
            }
            _ => {
                // Decline tasks better suited for desktop
                self.decline_task(request, "Desktop agent better suited").await?;
            }
        }
        
        Ok(())
    }
    
    /// Coordinate context sharing with desktop agent
    pub async fn coordinate_context_sharing(&mut self, desktop_entity_id: FluxEntityId) -> Result<()> {
        let coordination = ActiveCoordination {
            coordination_id: CoordinationId::new(),
            participants: vec![self.get_phone_entity_id(), desktop_entity_id.clone()],
            coordination_type: CoordinationType::ContextSharing {
                shared_context_types: vec![
                    ContextType::UserLocation,
                    ContextType::UserActivity,
                    ContextType::ConversationHistory,
                    ContextType::AgentMode,
                ],
                update_frequency: Duration::from_secs(10),
            },
            start_time: SystemTime::now(),
            shared_context: SharedContext::new(),
            communication_channel: format!("coordination.{}", coordination.coordination_id),
        };
        
        // Notify desktop agent of coordination
        let coordination_event = FluxEvent {
            event_type: "agent_coordination_proposal".to_string(),
            stream: "pure.jade.agent_coordination".to_string(),
            source: self.get_phone_entity_id().to_string(),
            timestamp: SystemTime::now().duration_since(UNIX_EPOCH)?.as_millis() as i64,
            payload: serde_json::to_value(&coordination)?,
        };
        
        self.flux_state.publish_event(coordination_event).await?;
        self.active_coordinations.insert(coordination.coordination_id, coordination);
        
        Ok(())
    }
    
    /// Participate in multi-phone swarm for collective intelligence
    pub async fn participate_in_phone_swarm(&mut self, swarm_invitation: SwarmInvitation) -> Result<()> {
        // Evaluate swarm participation criteria
        if self.should_join_swarm(&swarm_invitation).await? {
            let participation = SwarmParticipation {
                swarm_id: swarm_invitation.swarm_id,
                participant_id: self.get_phone_entity_id(),
                participation_role: self.determine_swarm_role(&swarm_invitation).await?,
                capabilities: vec![
                    SwarmCapability::SensorFusion,
                    SwarmCapability::LocationTracking,
                    SwarmCapability::ConsciousnessMetrics,
                    SwarmCapability::PredictionMarkets,
                ],
                commitment_duration: Duration::from_minutes(30), // Participate for 30 minutes
            };
            
            // Join swarm
            self.swarm_manager.join_swarm(participation).await?;
            
            // Start contributing to swarm intelligence
            self.start_swarm_contributions().await?;
            
            log::info!("Joined phone swarm: {}", swarm_invitation.swarm_id);
        }
        
        Ok(())
    }
    
    /// Contribute phone's unique capabilities to swarm intelligence
    async fn start_swarm_contributions(&mut self) -> Result<()> {
        // Contribute sensor fusion data
        let sensor_contribution = SwarmContribution {
            contribution_type: ContributionType::SensorData,
            data: self.flux_state.sensor_fusion.get_sync_clusters().await?,
            confidence: 0.9,
            timestamp: SystemTime::now(),
        };
        
        self.swarm_manager.contribute(sensor_contribution).await?;
        
        // Contribute consciousness metrics
        let consciousness = self.flux_state.consciousness_monitor.get_current_state().await?;
        let consciousness_contribution = SwarmContribution {
            contribution_type: ContributionType::ConsciousnessMetrics,
            data: serde_json::to_value(consciousness)?,
            confidence: 0.8,
            timestamp: SystemTime::now(),
        };
        
        self.swarm_manager.contribute(consciousness_contribution).await?;
        
        Ok(())
    }
}
```

---

## Consequences

### Positive

1. **Seamless Device Continuity**: User context and agent state persist across device switches
2. **Collective Intelligence**: Multiple phones can coordinate for enhanced problem-solving
3. **Distributed Processing**: Complex tasks can be distributed across phone and desktop agents
4. **Real-Time Coordination**: NATS event streaming enables low-latency cross-device coordination
5. **Privacy Control**: Granular control over what state is shared publicly vs privately
6. **Network Resilience**: Graceful degradation and offline queue when disconnected
7. **Proven Infrastructure**: Flux/NATS system already proven in SCADA pipeline deployments

### Negative

1. **Network Dependency**: Reduced functionality when disconnected from Flux network
2. **Privacy Complexity**: Managing privacy across multiple devices and agents adds complexity
3. **Battery Impact**: Continuous state publishing and monitoring increases power consumption
4. **Bandwidth Usage**: High-frequency sensor streams consume network bandwidth
5. **State Consistency**: Managing consistency across distributed agent states is challenging

### Neutral

1. **Ecosystem Lock-in**: Creates dependency on Flux infrastructure for cross-device features
2. **Complexity Growth**: More devices in ecosystem increases coordination complexity
3. **Configuration Burden**: Users need to configure privacy and sharing preferences

---

## Implementation Timeline

### Phase 1 (Weeks 1-2): Core Flux Client
- Implement NATS connectivity and event publishing/subscribing
- Add entity registration and state management
- Create offline queue and reconnection handling

### Phase 2 (Weeks 3-4): Privacy and Security
- Build privacy filtering system with granular controls
- Add encryption for sensitive state data
- Create context-aware privacy adjustments

### Phase 3 (Weeks 5-6): State Publishing
- Integrate with sensor fusion for real-time state capture
- Add consciousness metrics publishing
- Create efficient state change detection and publishing

### Phase 4 (Weeks 7-8): Cross-Device Coordination
- Build agent coordination framework
- Add task delegation between phone and desktop
- Create context sharing protocols

### Phase 5 (Weeks 9-10): Swarm Integration & Optimization
- Implement swarm participation for multi-phone coordination
- Add swarm intelligence contribution mechanisms
- Optimize bandwidth usage and battery impact

---

## References

- [Flux Universe](https://flux-universe.com) — Flux world state system
- [Flux API Documentation](https://flux-universe.com/docs) — API reference for integration
- [NATS Messaging](https://nats.io/) — Underlying messaging infrastructure
- [NATS JetStream](https://docs.nats.io/nats-concepts/jetstream) — Event streaming and persistence
- [pure-jade Namespace](https://flux-universe.com/namespaces/pure-jade) — Our deployment namespace
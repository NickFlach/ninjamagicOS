# ADR-008: Blockchain Attestation Layer

**Status:** Proposed  
**Date:** 2026-03-20  
**Authors:** Nick Flach (Kannaka)  
**Supersedes:** N/A  
**Related:** ADR-001 (MSI Architecture), ADR-007 (Flux Integration), ADR-010 (Dual-Time Control)

---

## Context

Modern mobile devices operate in a landscape of increasing security threats where device identity, boot integrity, and software provenance are critical but difficult to verify. Current Android Verified Boot and attestation mechanisms are manufacturer-controlled and opaque, making it difficult for users and applications to independently verify device integrity.

The 0xSCADA blockchain system (NickFlach/0xSCADA v2.0) provides a proven NATS→blockchain pipeline with Blake3 hashing, signed anchor batches, and lightweight node participation. By integrating ninjamagicOS as a participant in the 0xSCADA network, we can provide:

- **Phone Identity**: Cryptographically verifiable device identity anchored on-chain
- **Boot Verification**: Complete boot chain from bootloader through OS initialization, hashed and attested
- **Agent Skill Provenance**: Cryptographic proof of agent skill source code and execution integrity  
- **Event Attestation**: Selective attestation of critical phone events (location claims, sensor data, user interactions)
- **Cross-Device Verification**: Other devices in ecosystem can verify phone claims independently

### Current Android Security (Limitation)

```java
// Current opaque approach - trust manufacturer attestation
public class DeviceAttestation {
    public boolean verifyDevice() {
        // Black box verification through manufacturer keys
        return GooglePlayIntegrity.verify(attestationChallenge);
    }
}
```

### Required Blockchain Properties

From 0xSCADA research and deployment:

1. **Lightweight Node**: Phone participates as lightweight blockchain node, not full validator
2. **Blake3 Hashing**: Efficient cryptographic hashing suitable for mobile hardware
3. **Anchor Batching**: Efficient batching of attestations to reduce blockchain overhead
4. **NATS Integration**: Seamless integration with existing NATS event infrastructure
5. **Selective Attestation**: Only critical events are attested on-chain, not everything
6. **Privacy Preservation**: Zero-knowledge proofs for privacy-sensitive attestations

---

## Decision

**We will integrate ninjamagicOS as a lightweight participant in the 0xSCADA blockchain network, providing cryptographic attestation for device identity, boot integrity, and agent skill provenance while maintaining privacy and mobile-appropriate resource usage.**

### Architecture: Blockchain-Attested Phone

```rust
/// Blockchain attestation manager for ninjamagicOS
pub struct BlockchainAttestation {
    // Core 0xSCADA integration
    scada_client: ScadaClient,
    blockchain_node: LightweightNode,
    anchor_batcher: AnchorBatcher,
    
    // Identity and keys
    device_identity: DeviceIdentity,
    attestation_keys: AttestationKeyPair,
    hardware_root: HardwareSecurityModule,
    
    // Boot verification
    boot_chain_hasher: BootChainHasher,
    verification_log: VerificationLog,
    
    // Skill provenance
    skill_registry: SkillRegistry,
    skill_attestor: SkillAttestor,
    
    // Event attestation
    event_selector: EventSelector,
    privacy_controller: PrivacyController,
    
    // MSI integration
    attestation_lane: LaneHandle,
    blockchain_events: EventBus,
    
    // Configuration
    attestation_config: AttestationConfig,
}

/// Device identity anchored on 0xSCADA blockchain
#[derive(Clone, Debug, Serialize, Deserialize)]
pub struct DeviceIdentity {
    pub device_id: DeviceId,              // Unique phone identifier
    pub hardware_attestation: HardwareAttestation, // Titan M2/Qualcomm SPU attestation
    pub public_key: PublicKey,            // Device signing key
    pub manufacturer_info: ManufacturerInfo,
    pub first_boot_hash: Blake3Hash,      // Hash of first secure boot
    pub blockchain_anchor: BlockchainAnchor, // On-chain anchor transaction
    pub creation_timestamp: Timestamp,
    pub last_attestation: Option<Timestamp>,
}

/// Boot chain verification with complete hash chain
#[derive(Clone, Debug, Serialize, Deserialize)]
pub struct BootChainAttestation {
    pub boot_stage_hashes: Vec<BootStageHash>,
    pub kernel_module_hashes: HashMap<String, Blake3Hash>,
    pub msi_initialization_hash: Blake3Hash,
    pub agent_binary_hash: Blake3Hash,
    pub complete_chain_hash: Blake3Hash,    // Hash of all previous hashes
    pub boot_timestamp: Timestamp,
    pub verification_status: BootVerificationStatus,
    pub blockchain_anchor: Option<BlockchainAnchor>,
}

/// Agent skill with cryptographic provenance
#[derive(Clone, Debug, Serialize, Deserialize)]
pub struct SkillAttestation {
    pub skill_id: SkillId,
    pub source_code_hash: Blake3Hash,      // Hash of skill source code
    pub compilation_hash: Blake3Hash,      // Hash of compiled bytecode
    pub dependency_hashes: Vec<Blake3Hash>, // Hashes of all dependencies
    pub signing_key: PublicKey,            // Key that signed this skill
    pub signature: Signature,              // Signature over skill hash
    pub skill_manifest: SkillManifest,     // Skill metadata and capabilities
    pub blockchain_anchor: Option<BlockchainAnchor>,
    pub attestation_timestamp: Timestamp,
}

impl BlockchainAttestation {
    /// Initialize device identity and anchor on blockchain
    pub async fn initialize_device_identity(&mut self) -> Result<DeviceIdentity> {
        // Generate or load device attestation keys
        let attestation_keys = self.get_or_generate_attestation_keys().await?;
        
        // Get hardware security attestation
        let hardware_attestation = self.hardware_root.create_attestation(
            &attestation_keys.public_key
        ).await?;
        
        // Hash the first boot for immutable identity anchor
        let first_boot_hash = self.compute_first_boot_hash().await?;
        
        // Create device identity
        let device_identity = DeviceIdentity {
            device_id: DeviceId::from_hardware_id(&hardware_attestation),
            hardware_attestation,
            public_key: attestation_keys.public_key,
            manufacturer_info: self.get_manufacturer_info(),
            first_boot_hash,
            blockchain_anchor: BlockchainAnchor::Pending,
            creation_timestamp: Timestamp::now(),
            last_attestation: None,
        };
        
        // Submit identity to blockchain for anchoring
        let anchor_request = AnchorRequest {
            request_type: AnchorType::DeviceIdentity,
            data_hash: Blake3Hash::hash(&device_identity),
            public_key: attestation_keys.public_key,
            metadata: serde_json::to_value(&device_identity)?,
        };
        
        let blockchain_anchor = self.submit_anchor_request(anchor_request).await?;
        let mut anchored_identity = device_identity;
        anchored_identity.blockchain_anchor = blockchain_anchor;
        
        self.device_identity = anchored_identity.clone();
        
        log::info!("Device identity initialized and anchored: {}", anchored_identity.device_id);
        Ok(anchored_identity)
    }
    
    /// Perform complete boot chain verification and attestation
    pub async fn attest_boot_chain(&mut self) -> Result<BootChainAttestation> {
        let mut boot_stage_hashes = Vec::new();
        let mut kernel_module_hashes = HashMap::new();
        
        // Hash each boot stage
        boot_stage_hashes.push(self.hash_bootloader_stage().await?);
        boot_stage_hashes.push(self.hash_kernel_stage().await?);
        boot_stage_hashes.push(self.hash_init_stage().await?);
        boot_stage_hashes.push(self.hash_framework_stage().await?);
        
        // Hash all MSI kernel modules
        for module_name in MSI_KERNEL_MODULES {
            let module_hash = self.hash_kernel_module(module_name).await?;
            kernel_module_hashes.insert(module_name.to_string(), module_hash);
        }
        
        // Hash MSI initialization
        let msi_init_hash = self.hash_msi_initialization().await?;
        
        // Hash agent binary
        let agent_hash = self.hash_agent_binary().await?;
        
        // Create complete chain hash
        let complete_chain_hash = self.compute_chain_hash(&boot_stage_hashes, 
                                                          &kernel_module_hashes,
                                                          &msi_init_hash,
                                                          &agent_hash).await?;
        
        let attestation = BootChainAttestation {
            boot_stage_hashes,
            kernel_module_hashes,
            msi_initialization_hash: msi_init_hash,
            agent_binary_hash: agent_hash,
            complete_chain_hash,
            boot_timestamp: Timestamp::now(),
            verification_status: BootVerificationStatus::Verified,
            blockchain_anchor: None, // Will be filled by anchor batcher
        };
        
        // Submit for blockchain attestation (async batched)
        self.submit_boot_attestation(&attestation).await?;
        
        log::info!("Boot chain attested: {}", complete_chain_hash);
        Ok(attestation)
    }
    
    /// Attest agent skill with cryptographic provenance
    pub async fn attest_agent_skill(&mut self, skill_info: &SkillInfo) -> Result<SkillAttestation> {
        // Hash skill source code
        let source_code_hash = Blake3Hash::hash_file(&skill_info.source_path).await?;
        
        // Hash compiled bytecode
        let compilation_hash = Blake3Hash::hash_file(&skill_info.bytecode_path).await?;
        
        // Hash all dependencies
        let mut dependency_hashes = Vec::new();
        for dep_path in &skill_info.dependency_paths {
            let dep_hash = Blake3Hash::hash_file(dep_path).await?;
            dependency_hashes.push(dep_hash);
        }
        
        // Create skill manifest
        let skill_manifest = SkillManifest {
            skill_name: skill_info.name.clone(),
            version: skill_info.version.clone(),
            capabilities: skill_info.capabilities.clone(),
            resource_requirements: skill_info.resource_requirements.clone(),
            author: skill_info.author.clone(),
            license: skill_info.license.clone(),
        };
        
        // Sign skill attestation
        let skill_data = SkillAttestationData {
            source_code_hash,
            compilation_hash,
            dependency_hashes: dependency_hashes.clone(),
            skill_manifest: skill_manifest.clone(),
        };
        
        let skill_signature = self.attestation_keys.sign(&skill_data).await?;
        
        let attestation = SkillAttestation {
            skill_id: skill_info.skill_id,
            source_code_hash,
            compilation_hash,
            dependency_hashes,
            signing_key: self.attestation_keys.public_key,
            signature: skill_signature,
            skill_manifest,
            blockchain_anchor: None, // Will be filled by anchor batcher
            attestation_timestamp: Timestamp::now(),
        };
        
        // Register in local skill registry
        self.skill_registry.register_skill(attestation.clone()).await?;
        
        // Submit for blockchain attestation
        self.submit_skill_attestation(&attestation).await?;
        
        log::info!("Skill attested: {} ({})", skill_info.name, attestation.source_code_hash);
        Ok(attestation)
    }
    
    /// Submit attestation to 0xSCADA blockchain via anchor batcher
    async fn submit_anchor_request(&self, request: AnchorRequest) -> Result<BlockchainAnchor> {
        // Add to local batch
        self.anchor_batcher.add_request(request.clone()).await?;
        
        // If batch is full or enough time has passed, submit batch
        if self.anchor_batcher.should_submit_batch().await {
            let batch = self.anchor_batcher.create_batch().await?;
            let batch_hash = Blake3Hash::hash(&batch);
            
            // Sign batch with device attestation key
            let batch_signature = self.attestation_keys.sign(&batch_hash).await?;
            
            // Submit to 0xSCADA network
            let signed_batch = SignedAnchorBatch {
                batch,
                signature: batch_signature,
                submitter: self.device_identity.device_id.clone(),
                submission_timestamp: Timestamp::now(),
            };
            
            let transaction_hash = self.scada_client.submit_anchor_batch(signed_batch).await?;
            
            log::info!("Submitted anchor batch to blockchain: {}", transaction_hash);
            
            // Return anchor for this specific request
            Ok(BlockchainAnchor {
                transaction_hash,
                batch_index: self.anchor_batcher.get_request_index(&request.data_hash)?,
                block_height: None, // Will be filled once confirmed
                confirmation_timestamp: None,
            })
        } else {
            // Return pending anchor
            Ok(BlockchainAnchor::Pending)
        }
    }
    
    /// Selectively attest critical phone events
    pub async fn attest_critical_event(&mut self, event: &CriticalEvent) -> Result<Option<EventAttestation>> {
        // Check if this event type should be attested
        if !self.event_selector.should_attest(event).await? {
            return Ok(None);
        }
        
        // Apply privacy filtering
        let filtered_event = self.privacy_controller.filter_event(event).await?;
        if filtered_event.is_none() {
            return Ok(None); // Event filtered for privacy
        }
        
        let attestable_event = filtered_event.unwrap();
        
        // Create event attestation
        let event_hash = Blake3Hash::hash(&attestable_event);
        let event_signature = self.attestation_keys.sign(&event_hash).await?;
        
        let attestation = EventAttestation {
            event_hash,
            event_type: attestable_event.event_type.clone(),
            timestamp: attestable_event.timestamp,
            device_id: self.device_identity.device_id.clone(),
            signature: event_signature,
            privacy_level: attestable_event.privacy_level,
            blockchain_anchor: None, // Will be filled by anchor batcher
        };
        
        // Submit for blockchain attestation
        self.submit_event_attestation(&attestation).await?;
        
        Ok(Some(attestation))
    }
}
```

### Integration Points

1. **MSI Boot Process**: Hash each stage of MSI initialization and kernel module loading
2. **Agent Runtime**: Attest agent skill loading and execution with provenance chain
3. **Hardware Security**: Integrate with Titan M2 (Pixel) and Qualcomm SPU (Nord) for root of trust
4. **NATS Events**: Publish attestations via existing NATS infrastructure to blockchain
5. **Flux Integration**: Publish verified device identity and attestations to Flux ecosystem
6. **Privacy Manager**: Control what events and data are attested publicly vs kept private

---

## Implementation

### Phase 1: Core Blockchain Integration

**Location**: `security/blockchain/src/scada_client.rs`

```rust
use blake3::{Hash as Blake3Hash, Hasher as Blake3Hasher};
use ed25519_dalek::{PublicKey, SecretKey, Signature, Signer, Verifier};

/// 0xSCADA blockchain client for ninjamagicOS
pub struct ScadaClient {
    // Network configuration
    scada_network: ScadaNetworkConfig,
    nats_client: Arc<NatsClient>,
    
    // Consensus participation
    consensus_participant: ConsensusParticipant,
    blockchain_state: BlockchainState,
    
    // Transaction management
    transaction_pool: TransactionPool,
    anchor_queue: AnchorQueue,
    
    // Cryptography
    signing_keys: Ed25519KeyPair,
    verification_cache: VerificationCache,
}

#[derive(Clone, Debug)]
pub struct ScadaNetworkConfig {
    pub network_id: String,           // "0xSCADA-mainnet" or "0xSCADA-testnet"
    pub nats_cluster: Vec<String>,    // NATS server endpoints
    pub consensus_nodes: Vec<PublicKey>, // Known validator public keys
    pub block_time: Duration,         // Target block time
    pub anchor_batch_size: usize,     // Anchors per batch
    pub anchor_batch_timeout: Duration, // Max time before submitting partial batch
}

impl ScadaClient {
    /// Connect to 0xSCADA network
    pub async fn connect(&mut self) -> Result<()> {
        // Connect to NATS cluster
        let nats_opts = NatsOptions::new()
            .with_name("ninjamagic-phone")
            .with_max_reconnects(10)
            .with_reconnect_buffer_size(1024 * 1024); // 1MB buffer
            
        self.nats_client = Arc::new(async_nats::connect_with_options(
            &self.scada_network.nats_cluster[0], // Primary NATS server
            nats_opts
        ).await?);
        
        // Subscribe to blockchain events
        self.subscribe_to_blockchain_events().await?;
        
        // Sync with current blockchain state
        self.sync_blockchain_state().await?;
        
        // Start consensus participation (lightweight)
        self.start_consensus_participation().await?;
        
        log::info!("Connected to 0xSCADA network: {}", self.scada_network.network_id);
        Ok(())
    }
    
    /// Submit anchor batch to blockchain
    pub async fn submit_anchor_batch(&mut self, signed_batch: SignedAnchorBatch) -> Result<TransactionHash> {
        // Validate batch signature
        let batch_hash = Blake3Hash::hash(&signed_batch.batch);
        signed_batch.signature.verify(
            &signed_batch.submitter_public_key,
            &batch_hash
        )?;
        
        // Create blockchain transaction
        let transaction = ScadaTransaction {
            transaction_type: TransactionType::AnchorBatch,
            data: serde_json::to_vec(&signed_batch)?,
            submitter: signed_batch.submitter.clone(),
            timestamp: Timestamp::now(),
            fee: self.calculate_anchor_fee(&signed_batch.batch).await?,
            nonce: self.get_next_nonce().await?,
        };
        
        // Sign transaction
        let tx_hash = Blake3Hash::hash(&transaction);
        let tx_signature = self.signing_keys.sign(&tx_hash);
        
        let signed_transaction = SignedTransaction {
            transaction,
            signature: tx_signature,
        };
        
        // Submit to transaction pool via NATS
        let tx_subject = format!("scada.{}.transactions.submit", self.scada_network.network_id);
        let tx_payload = serde_json::to_vec(&signed_transaction)?;
        
        self.nats_client.publish(&tx_subject, tx_payload.into()).await?;
        
        // Add to local pool
        self.transaction_pool.add_pending_transaction(signed_transaction.clone()).await?;
        
        log::info!("Submitted anchor batch transaction: {}", tx_hash);
        Ok(TransactionHash::from(tx_hash))
    }
    
    /// Verify attestation against blockchain
    pub async fn verify_attestation(&self, attestation: &Attestation) -> Result<VerificationResult> {
        match &attestation.blockchain_anchor {
            BlockchainAnchor::Confirmed { transaction_hash, block_height, .. } => {
                // Fetch transaction from blockchain
                let transaction = self.get_transaction(*transaction_hash).await?;
                
                // Verify transaction is in confirmed block
                let block = self.get_block(*block_height).await?;
                if !block.contains_transaction(*transaction_hash) {
                    return Ok(VerificationResult::Invalid("Transaction not in claimed block".to_string()));
                }
                
                // Verify attestation hash matches transaction data
                if let Some(anchor_batch) = self.extract_anchor_batch(&transaction).await? {
                    for anchor in &anchor_batch.anchors {
                        if anchor.data_hash == attestation.hash() {
                            return Ok(VerificationResult::Valid {
                                block_height: *block_height,
                                confirmation_depth: self.blockchain_state.current_height - block_height,
                                timestamp: block.timestamp,
                            });
                        }
                    }
                }
                
                Ok(VerificationResult::Invalid("Attestation hash not found in transaction".to_string()))
            }
            BlockchainAnchor::Pending => {
                Ok(VerificationResult::Pending)
            }
        }
    }
    
    /// Participate in consensus as lightweight node
    async fn start_consensus_participation(&mut self) -> Result<()> {
        // Subscribe to block proposals
        let proposal_subject = format!("scada.{}.consensus.proposals", self.scada_network.network_id);
        let proposal_subscription = self.nats_client.subscribe(&proposal_subject).await?;
        
        // Subscribe to block confirmations
        let confirmation_subject = format!("scada.{}.consensus.confirmations", self.scada_network.network_id);
        let confirmation_subscription = self.nats_client.subscribe(&confirmation_subject).await?;
        
        // Start consensus message handling
        tokio::spawn(async move {
            while let Some(message) = proposal_subscription.next().await {
                if let Ok(proposal) = serde_json::from_slice::<BlockProposal>(&message.payload) {
                    // Lightweight validation of block proposal
                    if self.validate_block_proposal(&proposal).await.unwrap_or(false) {
                        // Vote on proposal (lightweight nodes have minimal voting weight)
                        let vote = ConsensusVote {
                            block_hash: proposal.block.hash(),
                            voter: self.signing_keys.public_key,
                            vote_type: VoteType::Accept,
                            timestamp: Timestamp::now(),
                        };
                        
                        let vote_signature = self.signing_keys.sign(&vote);
                        let signed_vote = SignedConsensusVote { vote, signature: vote_signature };
                        
                        let vote_subject = format!("scada.{}.consensus.votes", self.scada_network.network_id);
                        let vote_payload = serde_json::to_vec(&signed_vote).unwrap();
                        
                        let _ = self.nats_client.publish(&vote_subject, vote_payload.into()).await;
                    }
                }
            }
        });
        
        Ok(())
    }
    
    /// Lightweight block proposal validation
    async fn validate_block_proposal(&self, proposal: &BlockProposal) -> Result<bool> {
        // Check block hash integrity
        let computed_hash = proposal.block.compute_hash();
        if computed_hash != proposal.block.hash() {
            return Ok(false);
        }
        
        // Check previous block hash
        if proposal.block.previous_hash != self.blockchain_state.latest_block_hash {
            return Ok(false);
        }
        
        // Check timestamp (should be recent)
        let now = Timestamp::now();
        if proposal.block.timestamp > now || 
           now.duration_since(proposal.block.timestamp) > Duration::from_minutes(10) {
            return Ok(false);
        }
        
        // Validate a sample of transactions (not all, for performance)
        let sample_size = proposal.block.transactions.len().min(10);
        for tx in proposal.block.transactions.iter().take(sample_size) {
            if !self.validate_transaction_basic(tx).await? {
                return Ok(false);
            }
        }
        
        Ok(true)
    }
}
```

### Phase 2: Boot Chain Attestation

**Location**: `security/attestation/src/boot_attestation.rs`

```rust
/// Boot chain attestation system
pub struct BootChainAttestor {
    // Hardware security module interface
    hsm: Box<dyn HardwareSecurityModule>,
    
    // Boot stage tracking
    boot_stages: Vec<BootStage>,
    boot_measurements: HashMap<String, Blake3Hash>,
    
    // Verification
    trusted_hashes: TrustedHashRegistry,
    verification_policies: VerificationPolicySet,
    
    // Blockchain integration
    blockchain_client: Arc<ScadaClient>,
}

/// Individual boot stage measurement
#[derive(Clone, Debug)]
pub struct BootStage {
    pub stage_name: String,
    pub stage_type: BootStageType,
    pub measurement_hash: Blake3Hash,
    pub stage_timestamp: Timestamp,
    pub verification_status: StageVerificationStatus,
    pub dependencies: Vec<String>, // Previous stages this depends on
}

#[derive(Clone, Debug)]
pub enum BootStageType {
    Bootloader,           // Primary bootloader (PBL)
    SecondaryBootloader,  // Android Bootloader (ABL)
    KernelImage,         // Linux kernel image
    KernelModules,       // Loadable kernel modules
    MSIInitialization,   // MSI substrate initialization
    SystemServices,      // Core system services
    AgentRuntime,        // NinjaMagic agent initialization
}

impl BootChainAttestor {
    /// Begin boot chain attestation process
    pub async fn start_boot_attestation(&mut self) -> Result<()> {
        log::info!("Starting boot chain attestation");
        
        // Initialize HSM and get hardware root of trust
        self.hsm.initialize().await?;
        let hardware_identity = self.hsm.get_device_identity().await?;
        
        // Start measuring boot stages
        self.measure_bootloader_stage().await?;
        self.measure_kernel_stage().await?;
        self.measure_msi_stage().await?;
        self.measure_agent_stage().await?;
        
        // Create complete boot attestation
        let boot_attestation = self.create_boot_attestation(&hardware_identity).await?;
        
        // Submit to blockchain for permanent record
        self.submit_boot_attestation(&boot_attestation).await?;
        
        log::info!("Boot chain attestation completed: {}", boot_attestation.complete_chain_hash);
        Ok(())
    }
    
    /// Measure bootloader stage
    async fn measure_bootloader_stage(&mut self) -> Result<()> {
        // On Pixel 7: measure ABL (Android Bootloader)
        // On Nord N30: measure proprietary Qualcomm bootloader
        
        let bootloader_measurement = match self.get_device_type() {
            DeviceType::PixelTensor => {
                self.measure_pixel_bootloader().await?
            }
            DeviceType::OnePlusSnapdragon => {
                self.measure_oneplus_bootloader().await?
            }
        };
        
        let boot_stage = BootStage {
            stage_name: "bootloader".to_string(),
            stage_type: BootStageType::Bootloader,
            measurement_hash: bootloader_measurement,
            stage_timestamp: Timestamp::from_boot_time(),
            verification_status: self.verify_against_trusted_hashes(&bootloader_measurement).await?,
            dependencies: vec![], // Bootloader has no dependencies
        };
        
        self.boot_stages.push(boot_stage);
        self.boot_measurements.insert("bootloader".to_string(), bootloader_measurement);
        
        Ok(())
    }
    
    /// Measure MSI kernel modules and initialization
    async fn measure_msi_stage(&mut self) -> Result<()> {
        let mut msi_hasher = Blake3Hasher::new();
        
        // Hash MSI kernel module
        let msi_module_path = "/system/lib/modules/msi_core.ko";
        if let Ok(module_data) = std::fs::read(msi_module_path) {
            msi_hasher.update(&module_data);
        }
        
        // Hash MSI userspace runtime
        let msi_runtime_path = "/system/bin/msi_runtime";
        if let Ok(runtime_data) = std::fs::read(msi_runtime_path) {
            msi_hasher.update(&runtime_data);
        }
        
        // Hash MSI configuration
        let msi_config_path = "/system/etc/msi/config.toml";
        if let Ok(config_data) = std::fs::read(msi_config_path) {
            msi_hasher.update(&config_data);
        }
        
        let msi_measurement = Blake3Hash::from(msi_hasher.finalize());
        
        let boot_stage = BootStage {
            stage_name: "msi_initialization".to_string(),
            stage_type: BootStageType::MSIInitialization,
            measurement_hash: msi_measurement,
            stage_timestamp: Timestamp::now(),
            verification_status: self.verify_against_trusted_hashes(&msi_measurement).await?,
            dependencies: vec!["kernel".to_string()],
        };
        
        self.boot_stages.push(boot_stage);
        self.boot_measurements.insert("msi_initialization".to_string(), msi_measurement);
        
        Ok(())
    }
    
    /// Measure NinjaMagic agent binary and initialization
    async fn measure_agent_stage(&mut self) -> Result<()> {
        let mut agent_hasher = Blake3Hasher::new();
        
        // Hash agent core binary
        let agent_core_path = "/system/bin/ninja_agent_core";
        if let Ok(core_data) = std::fs::read(agent_core_path) {
            agent_hasher.update(&core_data);
        }
        
        // Hash agent configuration
        let agent_config_path = "/system/etc/ninja_agent/config.toml";
        if let Ok(config_data) = std::fs::read(agent_config_path) {
            agent_hasher.update(&config_data);
        }
        
        // Hash built-in skills
        let skills_path = "/system/share/ninja_agent/skills/";
        if let Ok(skills_dir) = std::fs::read_dir(skills_path) {
            for entry in skills_dir {
                if let Ok(entry) = entry {
                    if let Ok(skill_data) = std::fs::read(entry.path()) {
                        agent_hasher.update(&skill_data);
                    }
                }
            }
        }
        
        let agent_measurement = Blake3Hash::from(agent_hasher.finalize());
        
        let boot_stage = BootStage {
            stage_name: "agent_runtime".to_string(),
            stage_type: BootStageType::AgentRuntime,
            measurement_hash: agent_measurement,
            stage_timestamp: Timestamp::now(),
            verification_status: self.verify_against_trusted_hashes(&agent_measurement).await?,
            dependencies: vec!["msi_initialization".to_string()],
        };
        
        self.boot_stages.push(boot_stage);
        self.boot_measurements.insert("agent_runtime".to_string(), agent_measurement);
        
        Ok(())
    }
    
    /// Create complete boot chain attestation
    async fn create_boot_attestation(&self, hardware_identity: &HardwareIdentity) -> Result<BootChainAttestation> {
        // Create hash chain: hash of all boot stage hashes in order
        let mut chain_hasher = Blake3Hasher::new();
        
        // Include hardware identity in chain
        chain_hasher.update(&hardware_identity.device_id);
        chain_hasher.update(&hardware_identity.hardware_attestation);
        
        // Include all boot stages in dependency order
        let ordered_stages = self.sort_stages_by_dependencies()?;
        for stage in &ordered_stages {
            chain_hasher.update(stage.measurement_hash.as_bytes());
            chain_hasher.update(stage.stage_timestamp.as_bytes());
        }
        
        let complete_chain_hash = Blake3Hash::from(chain_hasher.finalize());
        
        // Sign the complete chain hash with device key
        let chain_signature = self.hsm.sign_with_device_key(&complete_chain_hash).await?;
        
        Ok(BootChainAttestation {
            device_identity: hardware_identity.clone(),
            boot_stages: ordered_stages,
            complete_chain_hash,
            chain_signature,
            attestation_timestamp: Timestamp::now(),
            verification_status: BootVerificationStatus::Verified,
            blockchain_anchor: None, // Will be filled when submitted to blockchain
        })
    }
    
    /// Submit boot attestation to blockchain
    async fn submit_boot_attestation(&self, attestation: &BootChainAttestation) -> Result<BlockchainAnchor> {
        let anchor_request = AnchorRequest {
            request_type: AnchorType::BootChainAttestation,
            data_hash: attestation.complete_chain_hash,
            submitter_public_key: self.hsm.get_device_public_key().await?,
            metadata: serde_json::to_value(attestation)?,
            timestamp: Timestamp::now(),
        };
        
        let blockchain_anchor = self.blockchain_client.submit_anchor_request(anchor_request).await?;
        
        log::info!("Boot chain attestation anchored on blockchain: {:?}", blockchain_anchor);
        Ok(blockchain_anchor)
    }
}
```

### Phase 3: Skill Provenance System

**Location**: `security/attestation/src/skill_attestation.rs`

```rust
/// Agent skill attestation and provenance tracking
pub struct SkillAttestor {
    // Code signing
    signing_keys: CodeSigningKeys,
    verification_keys: TrustedKeyRegistry,
    
    // Source tracking
    skill_registry: SkillRegistry,
    source_hasher: SourceCodeHasher,
    dependency_tracker: DependencyTracker,
    
    // Blockchain integration
    blockchain_client: Arc<ScadaClient>,
    attestation_cache: AttestationCache,
}

/// Comprehensive skill attestation with full provenance
#[derive(Clone, Debug, Serialize, Deserialize)]
pub struct SkillProvenance {
    pub skill_id: SkillId,
    pub skill_name: String,
    pub version: SemVer,
    
    // Source code provenance
    pub source_hashes: SourceCodeHashes,
    pub compilation_info: CompilationInfo,
    pub dependency_tree: DependencyTree,
    
    // Signing and verification
    pub code_signature: CodeSignature,
    pub signing_certificate: SigningCertificate,
    pub verification_chain: Vec<CertificateLink>,
    
    // Execution attestation
    pub execution_measurements: Vec<ExecutionMeasurement>,
    pub runtime_integrity: RuntimeIntegrityProof,
    
    // Blockchain anchoring
    pub blockchain_anchors: Vec<BlockchainAnchor>,
    pub attestation_history: Vec<AttestationEvent>,
}

impl SkillAttestor {
    /// Attest skill during installation/compilation
    pub async fn attest_skill_installation(&mut self, skill_source: &SkillSource) -> Result<SkillProvenance> {
        log::info!("Attesting skill installation: {}", skill_source.skill_name);
        
        // Hash source code files
        let source_hashes = self.hash_skill_source(skill_source).await?;
        
        // Compile skill and hash bytecode
        let compilation_result = self.compile_skill(skill_source).await?;
        let bytecode_hash = Blake3Hash::hash(&compilation_result.bytecode);
        
        // Analyze and hash dependencies
        let dependency_tree = self.analyze_dependencies(skill_source).await?;
        let dependency_hashes = self.hash_dependency_tree(&dependency_tree).await?;
        
        // Create compilation info
        let compilation_info = CompilationInfo {
            compiler_version: compilation_result.compiler_version,
            compilation_flags: compilation_result.flags,
            compilation_timestamp: Timestamp::now(),
            bytecode_hash,
            optimization_level: compilation_result.optimization_level,
        };
        
        // Sign the skill package
        let skill_package = SkillPackage {
            source_hashes: source_hashes.clone(),
            compilation_info: compilation_info.clone(),
            dependency_hashes,
        };
        
        let code_signature = self.signing_keys.sign_skill_package(&skill_package).await?;
        let signing_certificate = self.signing_keys.get_certificate();
        
        // Create initial provenance record
        let provenance = SkillProvenance {
            skill_id: SkillId::from_source_hash(&source_hashes.main_source_hash),
            skill_name: skill_source.skill_name.clone(),
            version: skill_source.version.clone(),
            source_hashes,
            compilation_info,
            dependency_tree,
            code_signature,
            signing_certificate,
            verification_chain: self.build_verification_chain().await?,
            execution_measurements: Vec::new(), // Will be populated during execution
            runtime_integrity: RuntimeIntegrityProof::default(),
            blockchain_anchors: Vec::new(), // Will be populated when anchored
            attestation_history: vec![
                AttestationEvent {
                    event_type: AttestationEventType::SkillInstalled,
                    timestamp: Timestamp::now(),
                    attestor: self.get_attestor_identity(),
                }
            ],
        };
        
        // Register in local registry
        self.skill_registry.register_skill(provenance.clone()).await?;
        
        // Submit to blockchain for permanent record
        self.submit_skill_provenance(&provenance).await?;
        
        log::info!("Skill provenance established: {}", provenance.skill_id);
        Ok(provenance)
    }
    
    /// Attest skill execution with runtime measurements
    pub async fn attest_skill_execution(
        &mut self,
        skill_id: SkillId,
        execution_context: &ExecutionContext
    ) -> Result<ExecutionAttestation> {
        // Get skill provenance
        let mut skill_provenance = self.skill_registry.get_skill(skill_id).await?
            .ok_or(SkillError::ProvenanceNotFound(skill_id))?;
            
        // Measure execution environment
        let env_measurement = self.measure_execution_environment(execution_context).await?;
        
        // Measure loaded bytecode integrity
        let bytecode_measurement = self.measure_loaded_bytecode(skill_id).await?;
        
        // Verify bytecode matches compilation hash
        if bytecode_measurement != skill_provenance.compilation_info.bytecode_hash {
            return Err(SkillError::BytecodeIntegrityFailure {
                expected: skill_provenance.compilation_info.bytecode_hash,
                actual: bytecode_measurement,
            });
        }
        
        // Create execution measurement
        let execution_measurement = ExecutionMeasurement {
            measurement_id: ExecutionMeasurementId::new(),
            skill_id,
            execution_timestamp: Timestamp::now(),
            environment_hash: env_measurement,
            bytecode_integrity_hash: bytecode_measurement,
            input_data_hash: Blake3Hash::hash(&execution_context.input_data),
            execution_signature: None, // Will be filled below
        };
        
        // Sign execution measurement
        let measurement_signature = self.signing_keys.sign_execution_measurement(&execution_measurement).await?;
        let mut signed_measurement = execution_measurement;
        signed_measurement.execution_signature = Some(measurement_signature);
        
        // Add to skill provenance
        skill_provenance.execution_measurements.push(signed_measurement.clone());
        skill_provenance.attestation_history.push(AttestationEvent {
            event_type: AttestationEventType::SkillExecuted,
            timestamp: Timestamp::now(),
            attestor: self.get_attestor_identity(),
        });
        
        // Update registry
        self.skill_registry.update_skill(skill_provenance).await?;
        
        // Create execution attestation
        let execution_attestation = ExecutionAttestation {
            measurement: signed_measurement,
            verification_status: VerificationStatus::Verified,
            blockchain_anchor: None, // Will be filled if submitted to blockchain
        };
        
        // Optionally submit critical executions to blockchain
        if self.should_attest_execution_on_chain(&execution_context).await {
            self.submit_execution_attestation(&execution_attestation).await?;
        }
        
        Ok(execution_attestation)
    }
    
    /// Hash complete skill source code
    async fn hash_skill_source(&self, source: &SkillSource) -> Result<SourceCodeHashes> {
        let mut main_hasher = Blake3Hasher::new();
        let mut file_hashes = HashMap::new();
        
        // Hash main skill file
        let main_source = std::fs::read(&source.main_file_path)?;
        main_hasher.update(&main_source);
        let main_source_hash = Blake3Hash::from(main_hasher.finalize());
        
        file_hashes.insert(source.main_file_path.clone(), main_source_hash);
        
        // Hash additional source files
        for additional_file in &source.additional_files {
            let file_data = std::fs::read(additional_file)?;
            let file_hash = Blake3Hash::hash(&file_data);
            file_hashes.insert(additional_file.clone(), file_hash);
        }
        
        // Hash skill manifest/metadata
        let manifest_hash = Blake3Hash::hash(&serde_json::to_vec(&source.manifest)?);
        
        Ok(SourceCodeHashes {
            main_source_hash,
            file_hashes,
            manifest_hash,
            combined_hash: self.compute_combined_source_hash(&file_hashes, &manifest_hash),
        })
    }
    
    /// Verify skill against blockchain attestation
    pub async fn verify_skill_provenance(&self, skill_id: SkillId) -> Result<ProvenanceVerificationResult> {
        let skill_provenance = self.skill_registry.get_skill(skill_id).await?
            .ok_or(SkillError::ProvenanceNotFound(skill_id))?;
            
        // Verify code signature
        let signature_valid = self.verification_keys.verify_signature(
            &skill_provenance.code_signature,
            &skill_provenance.signing_certificate
        ).await?;
        
        if !signature_valid {
            return Ok(ProvenanceVerificationResult::Invalid("Invalid code signature".to_string()));
        }
        
        // Verify certificate chain
        let cert_chain_valid = self.verify_certificate_chain(&skill_provenance.verification_chain).await?;
        if !cert_chain_valid {
            return Ok(ProvenanceVerificationResult::Invalid("Invalid certificate chain".to_string()));
        }
        
        // Verify blockchain anchors
        let mut blockchain_verifications = Vec::new();
        for anchor in &skill_provenance.blockchain_anchors {
            let verification = self.blockchain_client.verify_anchor(anchor).await?;
            blockchain_verifications.push(verification);
        }
        
        let blockchain_valid = blockchain_verifications.iter()
            .all(|v| matches!(v, AnchorVerificationResult::Valid { .. }));
            
        if blockchain_valid {
            Ok(ProvenanceVerificationResult::Valid {
                signature_verified: true,
                certificate_chain_verified: true,
                blockchain_anchored: true,
                blockchain_confirmations: blockchain_verifications,
            })
        } else {
            Ok(ProvenanceVerificationResult::Invalid("Blockchain anchoring failed verification".to_string()))
        }
    }
}
```

---

## Consequences

### Positive

1. **Cryptographic Integrity**: Complete cryptographic provenance from bootloader through agent skills
2. **Tamper Detection**: Any modification to boot chain or skills is detectable and verifiable
3. **Cross-Device Trust**: Other devices can verify phone claims independently via blockchain
4. **Audit Trail**: Permanent, immutable record of all critical device and agent events
5. **Zero-Trust Architecture**: No need to trust manufacturer attestation, everything is independently verifiable
6. **Proven Infrastructure**: 0xSCADA v2.0 already proven in production NATS→blockchain deployments

### Negative

1. **Computational Overhead**: Continuous hashing, signing, and verification increases CPU usage
2. **Network Dependency**: Some verification requires connectivity to 0xSCADA blockchain network
3. **Storage Requirements**: Attestation history and blockchain anchors require storage space
4. **Battery Impact**: Cryptographic operations and network communication affect battery life
5. **Bootstrap Complexity**: Initial device identity and key establishment is complex

### Neutral

1. **Blockchain Network Effects**: Value increases as more devices participate in 0xSCADA network
2. **Legal Implications**: Cryptographic attestation may have legal weight for compliance scenarios
3. **Privacy Trade-offs**: Blockchain attestation provides integrity but may reduce anonymity

---

## Implementation Timeline

### Phase 1 (Weeks 1-3): Core Blockchain Integration
- Implement 0xSCADA client with NATS connectivity
- Add lightweight consensus participation
- Create anchor batching and submission system

### Phase 2 (Weeks 4-6): Boot Chain Attestation
- Build hardware security module integration (Titan M2 / Qualcomm SPU)
- Implement complete boot chain measurement and verification
- Add trusted hash registry for known-good boot measurements

### Phase 3 (Weeks 7-9): Skill Provenance System
- Create skill source code hashing and compilation attestation
- Implement code signing and verification chain management
- Add execution measurement and runtime integrity proofs

### Phase 4 (Weeks 10-12): Event Attestation & Privacy
- Build selective event attestation with privacy filtering
- Add zero-knowledge proof support for sensitive attestations
- Create attestation verification APIs for other devices

### Phase 5 (Weeks 13-15): Integration & Optimization
- Integrate with existing MSI, Flux, and agent systems
- Optimize cryptographic performance for mobile hardware
- Add comprehensive testing and security auditing

---

## References

- [0xSCADA Repository](https://github.com/NickFlach/0xSCADA) — v2.0 blockchain system with proven NATS pipeline
- [0xSCADA ADR-0021](../../../0xSCADA/docs/adr/ADR-0021-dual-time-control-plane.md) — Temporal integrity foundation
- [Blake3 Cryptographic Hash](https://github.com/BLAKE3-team/BLAKE3) — Fast, secure hashing for mobile
- [Ed25519 Digital Signatures](https://ed25519.cr.yp.to/) — High-performance signing suitable for mobile
- [Android Verified Boot](https://source.android.com/docs/security/features/verifiedboot) — Current approach for comparison
- [Trusted Platform Module (TPM)](https://trustedcomputinggroup.org/work-groups/trusted-platform-module/) — Hardware security foundation
# ADR-011: Audio Perception Layer

**Status:** Proposed  
**Date:** 2026-03-20  
**Authors:** Nick Flach (Kannaka)  
**Supersedes:** N/A  
**Related:** ADR-004 (Kuramoto Sensors), ADR-002 (Wave Memory), ADR-005 (Consciousness Metrics)

---

## Context

Current mobile operating systems treat audio as raw waveform data that applications must individually process and interpret. This approach misses a fundamental opportunity: **native audio perception** at the OS level, where the phone inherently understands what it hears rather than simply recording acoustic vibrations.

The kannaka-ear research demonstrates sophisticated audio perception through 296-dimensional perceptual vectors extracted from multiple acoustic analysis domains: mel spectrograms, MFCC coefficients, spectral features, rhythm patterns, pitch contours, chromatic harmony, and emotional valence. This creates a rich, multi-dimensional understanding of audio that goes far beyond simple recording.

In ninjamagicOS, native audio perception enables:
- **Ambient Sound Understanding**: Phone knows if environment is quiet office, busy street, nature sounds, etc.
- **Music Comprehension**: Phone understands genre, mood, energy, rhythm of playing music
- **Speech Pattern Recognition**: Phone detects conversation patterns, emotional tone, language characteristics
- **Context-Aware Adaptation**: Agent behavior adapts to acoustic environment automatically
- **Predictive Audio Intelligence**: Phone anticipates audio changes and prepares appropriate responses

### Current Android Audio Architecture (Limitation)

```java
// Current approach - raw audio data without understanding
public class AudioManager {
    public void onAudioData(byte[] audioBuffer) {
        // Raw PCM data, no semantic understanding
        audioRenderer.render(audioBuffer);
    }
}
```

### Required Audio Perception Properties

From kannaka-ear research and perceptual audio analysis:

1. **296-Dimensional Perception**: Multi-domain feature extraction creating rich audio understanding
2. **Real-Time Processing**: Continuous audio perception with minimal latency for real-time adaptation
3. **Semantic Classification**: High-level understanding of audio content (speech, music, environment, etc.)
4. **Emotional Perception**: Valence and arousal detection from audio characteristics
5. **Temporal Patterns**: Rhythm, tempo, and temporal structure understanding
6. **Harmonic Analysis**: Pitch, key, chord progression, and tonal characteristics

---

## Decision

**We will implement a native audio perception layer in ninjamagicOS that continuously extracts 296-dimensional perceptual vectors from all audio input, providing semantic understanding of the acoustic environment to enable context-aware agent behavior and rich audio intelligence.**

### Architecture: Native Audio Perception System

```rust
/// Native audio perception layer for ninjamagicOS
pub struct AudioPerceptionLayer {
    // Core audio processing pipeline
    audio_capture: AudioCapture,
    perception_engine: PerceptionEngine,
    feature_extractors: FeatureExtractorSet,
    
    // 296-dimensional perception
    perceptual_vector: PerceptualVector,
    perception_history: PerceptionHistory,
    
    // Classification and understanding
    semantic_classifier: SemanticAudioClassifier,
    emotion_detector: EmotionDetector,
    pattern_recognizer: PatternRecognizer,
    
    // Integration with phone systems
    kuramoto_sensor_fusion: Arc<KuramotoSensorFusion>,
    consciousness_monitor: Arc<PhoneConsciousness>,
    agent_context: Arc<AgentContext>,
    
    // Real-time processing
    perception_lane: LaneHandle,
    audio_events: EventBus,
    
    // Configuration
    perception_config: AudioPerceptionConfig,
}

/// 296-dimensional perceptual vector from kannaka-ear research
#[derive(Clone, Debug, Serialize, Deserialize)]
pub struct PerceptualVector {
    // Spectral features (128 dimensions)
    pub mel_spectrogram: Vec<f32>,      // 64 mel frequency bins
    pub mfcc_coefficients: Vec<f32>,    // 13 MFCC coefficients
    pub spectral_centroid: f32,         // Brightness of sound
    pub spectral_bandwidth: f32,        // Spread of spectrum
    pub spectral_rolloff: f32,          // Frequency below which 85% of energy lies
    pub zero_crossing_rate: f32,        // Rate of sign changes
    pub spectral_flux: f32,             // Rate of change in spectrum
    pub spectral_slope: f32,            // Tilt of spectrum
    pub spectral_spread: f32,           // Standard deviation around centroid
    pub spectral_skewness: f32,         // Asymmetry of spectrum
    pub spectral_kurtosis: f32,         // Peakedness of spectrum
    pub spectral_entropy: f32,          // Disorder in frequency domain
    pub spectral_flatness: f32,         // Measure of noise vs tones
    
    // Temporal features (64 dimensions)
    pub tempo_estimate: f32,            // Beats per minute
    pub rhythm_strength: f32,           // How rhythmic the audio is
    pub onset_density: f32,             // Rate of new events
    pub pulse_clarity: f32,             // How clear the beat is
    pub tempo_stability: f32,           // Consistency of tempo
    pub rhythm_regularity: f32,         // Regularity of rhythmic patterns
    pub temporal_centroid: f32,         // Time-weighted center of energy
    pub attack_time: f32,               // Time to reach peak amplitude
    pub decay_time: f32,                // Time to decay from peak
    pub sustain_level: f32,             // Steady state amplitude level
    pub release_time: f32,              // Time to fade to silence
    pub envelope_dynamics: Vec<f32>,    // 53 envelope shape features
    
    // Harmonic features (48 dimensions)
    pub pitch_estimate: f32,            // Fundamental frequency
    pub pitch_confidence: f32,          // Confidence in pitch estimate
    pub pitch_salience: f32,            // Strength of pitch perception
    pub harmonic_ratio: f32,            // Ratio of harmonic to noise energy
    pub inharmonicity: f32,             // Deviation from perfect harmonics
    pub chroma_vector: Vec<f32>,        // 12-dimensional chroma (pitch classes)
    pub key_estimate: u8,               // Estimated musical key
    pub key_confidence: f32,            // Confidence in key estimate
    pub mode_estimate: u8,              // Major/minor mode estimate
    pub chord_estimate: Option<String>, // Estimated chord if detectable
    pub harmonic_energy: Vec<f32>,      // Energy in harmonic bands (20 dims)
    pub spectral_peaks: Vec<f32>,       // Prominent frequency peaks (10 dims)
    
    // Perceptual features (32 dimensions)
    pub loudness_sone: f32,             // Perceptual loudness in sones
    pub sharpness_acum: f32,            // Perceptual sharpness in acums
    pub roughness_asper: f32,           // Perceptual roughness in aspers
    pub fluctuation_strength: f32,      // Strength of amplitude modulation
    pub tonality: f32,                  // Tonal vs noisy character
    pub brightness: f32,                // Perceptual brightness
    pub warmth: f32,                    // Perceptual warmth
    pub fullness: f32,                  // Perceptual fullness
    pub clarity: f32,                   // Perceptual clarity
    pub presence: f32,                  // Perceptual presence
    pub spaciousness: f32,              // Perceptual spaciousness
    pub envelopment: f32,               // Perceptual envelopment
    pub intimacy: f32,                  // Perceptual intimacy
    pub reverberance: f32,              // Perceptual reverberation
    pub echo_density: f32,              // Density of echoes
    pub stereo_width: f32,              // Perceived stereo width
    pub binaural_qualities: Vec<f32>,   // Spatial audio qualities (16 dims)
    
    // Emotional/valence features (24 dimensions)
    pub valence: f32,                   // Positive vs negative emotion (-1 to 1)
    pub arousal: f32,                   // Energy level (0 to 1)
    pub dominance: f32,                 // Sense of control/power (0 to 1)
    pub tension: f32,                   // Musical/emotional tension
    pub energy: f32,                    // Perceived energy level
    pub danceability: f32,              // How suitable for dancing
    pub happiness: f32,                 // Perceived happiness
    pub sadness: f32,                   // Perceived sadness
    pub anger: f32,                     // Perceived anger
    pub fear: f32,                      // Perceived fear
    pub surprise: f32,                  // Perceived surprise
    pub disgust: f32,                   // Perceived disgust
    pub emotional_dynamics: Vec<f32>,   // Emotional change patterns (12 dims)
    
    // Metadata
    pub timestamp: SystemTime,
    pub confidence: f32,                // Overall confidence in perception
    pub audio_quality: f32,             // Quality of input audio
    pub context_type: AudioContextType, // Speech, music, environment, etc.
}

/// Real-time audio perception engine
pub struct PerceptionEngine {
    // Audio processing pipeline
    preprocessing: AudioPreprocessor,
    windowing: WindowingFunction,
    fft_processor: FFTProcessor,
    
    // Feature extractors
    spectral_extractor: SpectralFeatureExtractor,
    temporal_extractor: TemporalFeatureExtractor,
    harmonic_extractor: HarmonicFeatureExtractor,
    perceptual_extractor: PerceptualFeatureExtractor,
    emotion_extractor: EmotionFeatureExtractor,
    
    // Real-time optimization
    processing_buffer: CircularBuffer<f32>,
    feature_cache: FeatureCache,
    adaptive_processor: AdaptiveProcessor,
    
    // Hardware acceleration
    npu_accelerator: Option<NPUAudioProcessor>,
    dsp_accelerator: Option<DSPAudioProcessor>,
}

impl AudioPerceptionLayer {
    /// Initialize audio perception system
    pub async fn initialize(&mut self) -> Result<()> {
        // Initialize audio capture
        self.audio_capture.initialize().await?;
        
        // Configure perception engine for mobile hardware
        self.perception_engine.configure_for_mobile().await?;
        
        // Initialize hardware accelerators if available
        if let Ok(npu) = self.initialize_npu_acceleration().await {
            self.perception_engine.npu_accelerator = Some(npu);
        }
        
        // Start real-time perception loop
        self.start_perception_loop().await?;
        
        // Integrate with Kuramoto sensor fusion
        self.integrate_with_kuramoto_sensors().await?;
        
        log::info!("Audio perception layer initialized with 296-dimensional perception");
        Ok(())
    }
    
    /// Main real-time audio perception loop
    pub async fn perception_loop(&mut self) -> Result<()> {
        let mut audio_buffer = vec![0.0f32; self.perception_config.buffer_size];
        
        loop {
            // Capture audio frame
            self.audio_capture.read_frame(&mut audio_buffer).await?;
            
            // Extract 296-dimensional perceptual vector
            let perceptual_vector = self.extract_perceptual_vector(&audio_buffer).await?;
            
            // Update perception state
            self.update_perception_state(perceptual_vector.clone()).await?;
            
            // Classify audio content
            let semantic_classification = self.semantic_classifier.classify(&perceptual_vector).await?;
            
            // Detect emotional content
            let emotional_state = self.emotion_detector.analyze(&perceptual_vector).await?;
            
            // Update agent context with audio understanding
            self.update_agent_context(&perceptual_vector, &semantic_classification, &emotional_state).await?;
            
            // Feed into Kuramoto sensor fusion
            self.feed_kuramoto_sensors(&perceptual_vector).await?;
            
            // Update consciousness metrics
            self.update_consciousness_metrics(&perceptual_vector).await?;
            
            // Publish perception events
            self.publish_perception_events(&perceptual_vector, &semantic_classification).await?;
            
            // Adaptive sleep based on audio activity
            let sleep_duration = self.calculate_adaptive_sleep(&perceptual_vector);
            tokio::time::sleep(sleep_duration).await;
        }
    }
    
    /// Extract 296-dimensional perceptual vector from audio buffer
    async fn extract_perceptual_vector(&mut self, audio_buffer: &[f32]) -> Result<PerceptualVector> {
        // Preprocess audio (normalize, filter, window)
        let processed_audio = self.perception_engine.preprocessing.process(audio_buffer)?;
        
        // Apply windowing function
        let windowed_audio = self.perception_engine.windowing.apply(&processed_audio)?;
        
        // Compute FFT for frequency domain analysis
        let frequency_spectrum = self.perception_engine.fft_processor.compute_fft(&windowed_audio)?;
        
        // Extract spectral features (128 dimensions)
        let spectral_features = self.perception_engine.spectral_extractor.extract(
            &frequency_spectrum,
            self.perception_config.sample_rate
        ).await?;
        
        // Extract temporal features (64 dimensions)
        let temporal_features = self.perception_engine.temporal_extractor.extract(
            audio_buffer,
            &self.perception_history
        ).await?;
        
        // Extract harmonic features (48 dimensions)  
        let harmonic_features = self.perception_engine.harmonic_extractor.extract(
            &frequency_spectrum,
            &processed_audio
        ).await?;
        
        // Extract perceptual features (32 dimensions)
        let perceptual_features = self.perception_engine.perceptual_extractor.extract(
            &frequency_spectrum,
            &processed_audio
        ).await?;
        
        // Extract emotional/valence features (24 dimensions)
        let emotion_features = self.perception_engine.emotion_extractor.extract(
            &spectral_features,
            &temporal_features,
            &harmonic_features
        ).await?;
        
        // Combine into 296-dimensional vector
        let perceptual_vector = PerceptualVector {
            // Spectral features (128 dims)
            mel_spectrogram: spectral_features.mel_spectrogram,
            mfcc_coefficients: spectral_features.mfcc_coefficients,
            spectral_centroid: spectral_features.centroid,
            spectral_bandwidth: spectral_features.bandwidth,
            spectral_rolloff: spectral_features.rolloff,
            zero_crossing_rate: spectral_features.zero_crossing_rate,
            spectral_flux: spectral_features.flux,
            spectral_slope: spectral_features.slope,
            spectral_spread: spectral_features.spread,
            spectral_skewness: spectral_features.skewness,
            spectral_kurtosis: spectral_features.kurtosis,
            spectral_entropy: spectral_features.entropy,
            spectral_flatness: spectral_features.flatness,
            
            // Temporal features (64 dims)
            tempo_estimate: temporal_features.tempo,
            rhythm_strength: temporal_features.rhythm_strength,
            onset_density: temporal_features.onset_density,
            pulse_clarity: temporal_features.pulse_clarity,
            tempo_stability: temporal_features.tempo_stability,
            rhythm_regularity: temporal_features.rhythm_regularity,
            temporal_centroid: temporal_features.temporal_centroid,
            attack_time: temporal_features.attack_time,
            decay_time: temporal_features.decay_time,
            sustain_level: temporal_features.sustain_level,
            release_time: temporal_features.release_time,
            envelope_dynamics: temporal_features.envelope_dynamics,
            
            // Harmonic features (48 dims)
            pitch_estimate: harmonic_features.pitch_estimate,
            pitch_confidence: harmonic_features.pitch_confidence,
            pitch_salience: harmonic_features.pitch_salience,
            harmonic_ratio: harmonic_features.harmonic_ratio,
            inharmonicity: harmonic_features.inharmonicity,
            chroma_vector: harmonic_features.chroma_vector,
            key_estimate: harmonic_features.key_estimate,
            key_confidence: harmonic_features.key_confidence,
            mode_estimate: harmonic_features.mode_estimate,
            chord_estimate: harmonic_features.chord_estimate,
            harmonic_energy: harmonic_features.harmonic_energy,
            spectral_peaks: harmonic_features.spectral_peaks,
            
            // Perceptual features (32 dims)
            loudness_sone: perceptual_features.loudness_sone,
            sharpness_acum: perceptual_features.sharpness_acum,
            roughness_asper: perceptual_features.roughness_asper,
            fluctuation_strength: perceptual_features.fluctuation_strength,
            tonality: perceptual_features.tonality,
            brightness: perceptual_features.brightness,
            warmth: perceptual_features.warmth,
            fullness: perceptual_features.fullness,
            clarity: perceptual_features.clarity,
            presence: perceptual_features.presence,
            spaciousness: perceptual_features.spaciousness,
            envelopment: perceptual_features.envelopment,
            intimacy: perceptual_features.intimacy,
            reverberance: perceptual_features.reverberance,
            echo_density: perceptual_features.echo_density,
            stereo_width: perceptual_features.stereo_width,
            binaural_qualities: perceptual_features.binaural_qualities,
            
            // Emotional/valence features (24 dims)
            valence: emotion_features.valence,
            arousal: emotion_features.arousal,
            dominance: emotion_features.dominance,
            tension: emotion_features.tension,
            energy: emotion_features.energy,
            danceability: emotion_features.danceability,
            happiness: emotion_features.happiness,
            sadness: emotion_features.sadness,
            anger: emotion_features.anger,
            fear: emotion_features.fear,
            surprise: emotion_features.surprise,
            disgust: emotion_features.disgust,
            emotional_dynamics: emotion_features.emotional_dynamics,
            
            // Metadata
            timestamp: SystemTime::now(),
            confidence: self.calculate_overall_confidence(&spectral_features, &temporal_features, &harmonic_features)?,
            audio_quality: self.assess_audio_quality(audio_buffer)?,
            context_type: self.classify_audio_context(&spectral_features, &temporal_features)?,
        };
        
        Ok(perceptual_vector)
    }
    
    /// Feed audio perception into Kuramoto sensor fusion
    async fn feed_kuramoto_sensors(&self, perception: &PerceptualVector) -> Result<()> {
        // Create audio sensor oscillator for Kuramoto coupling
        let audio_oscillator = SensorOscillator {
            sensor_id: SensorId::AudioPerception,
            phase: self.compute_audio_phase(perception).await?,
            frequency: self.compute_audio_frequency(perception).await?,
            amplitude: perception.confidence,
            last_reading: SensorReading {
                timestamp_ns: perception.timestamp.duration_since(UNIX_EPOCH)?.as_nanos() as u64,
                values: Vector3::new(
                    perception.valence,
                    perception.arousal, 
                    perception.energy
                ),
                accuracy: perception.confidence,
                temperature: None,
            },
            quality_metric: perception.audio_quality,
            couplings: vec![
                // Couple with accelerometer (movement affects audio perception)
                SensorCoupling {
                    target_sensor: SensorId::Accelerometer,
                    strength: 0.3,
                    phase_offset: 0.0,
                    coupling_type: CouplingType::Semantic,
                },
                // Couple with microphone (for speech detection)
                SensorCoupling {
                    target_sensor: SensorId::Microphone,
                    strength: 0.8,
                    phase_offset: 0.0,
                    coupling_type: CouplingType::Spatial,
                },
            ],
        };
        
        // Update Kuramoto sensor fusion
        self.kuramoto_sensor_fusion.update_sensor_oscillator(audio_oscillator).await?;
        
        Ok(())
    }
    
    /// Update phone consciousness metrics based on audio perception
    async fn update_consciousness_metrics(&self, perception: &PerceptualVector) -> Result<()> {
        // Audio diversity contributes to consciousness complexity (Xi)
        let audio_diversity = self.calculate_audio_diversity(perception);
        
        // Audio coherence contributes to consciousness integration (Phi)
        let audio_coherence = self.calculate_audio_coherence(perception);
        
        // Audio synchronization contributes to consciousness order
        let audio_synchronization = self.calculate_audio_synchronization(perception);
        
        // Update consciousness monitor
        let consciousness_update = ConsciousnessUpdate {
            source: ConsciousnessSource::AudioPerception,
            diversity_contribution: audio_diversity,
            coherence_contribution: audio_coherence,
            synchronization_contribution: audio_synchronization,
            timestamp: SystemTime::now(),
        };
        
        self.consciousness_monitor.update_from_subsystem(consciousness_update).await?;
        
        Ok(())
    }
    
    /// Update agent context with audio understanding
    async fn update_agent_context(
        &self,
        perception: &PerceptualVector,
        classification: &AudioClassification,
        emotion: &EmotionalState
    ) -> Result<()> {
        // Create rich audio context for agent
        let audio_context = AudioContext {
            environment_type: classification.environment_type.clone(),
            content_type: classification.content_type.clone(),
            emotional_atmosphere: emotion.clone(),
            acoustic_properties: AcousticProperties {
                loudness_level: perception.loudness_sone,
                frequency_character: perception.brightness,
                temporal_character: perception.rhythm_strength,
                spatial_character: perception.spaciousness,
            },
            conversation_indicators: ConversationIndicators {
                speech_detected: classification.contains_speech,
                speaker_count: classification.estimated_speaker_count,
                conversation_energy: perception.energy,
                emotional_tone: emotion.valence,
            },
            music_understanding: if let AudioContentType::Music(music_info) = &classification.content_type {
                Some(MusicUnderstanding {
                    genre: music_info.genre.clone(),
                    tempo: perception.tempo_estimate,
                    key: perception.key_estimate,
                    mode: perception.mode_estimate,
                    energy: perception.energy,
                    danceability: perception.danceability,
                    emotional_character: emotion.clone(),
                })
            } else {
                None
            },
            adaptation_recommendations: self.generate_adaptation_recommendations(
                perception,
                classification,
                emotion
            ).await?,
        };
        
        // Update agent context
        let context_event = Event::new(
            "agent/context/audio_update",
            AudioContextUpdatePayload {
                audio_context,
                perception_vector: perception.clone(),
                classification: classification.clone(),
                emotional_state: emotion.clone(),
            }
        );
        
        self.agent_context.update_from_audio_perception(context_event).await?;
        
        Ok(())
    }
    
    /// Generate recommendations for agent adaptation based on audio
    async fn generate_adaptation_recommendations(
        &self,
        perception: &PerceptualVector,
        classification: &AudioClassification,
        emotion: &EmotionalState
    ) -> Result<Vec<AdaptationRecommendation>> {
        let mut recommendations = Vec::new();
        
        // Volume adaptation
        if perception.loudness_sone > 40.0 { // Very loud environment
            recommendations.push(AdaptationRecommendation::VolumeIncrease {
                reason: "Loud environment detected".to_string(),
                suggested_increase: 0.3,
            });
        } else if perception.loudness_sone < 5.0 { // Very quiet environment
            recommendations.push(AdaptationRecommendation::VolumeDecrease {
                reason: "Quiet environment detected".to_string(),
                suggested_decrease: 0.2,
            });
        }
        
        // Speech detection adaptations
        if classification.contains_speech {
            recommendations.push(AdaptationRecommendation::ReduceInterruptions {
                reason: "Speech/conversation detected".to_string(),
                duration: Duration::from_minutes(5),
            });
            
            if emotion.arousal > 0.7 { // High energy conversation
                recommendations.push(AdaptationRecommendation::MinimizeNotifications {
                    reason: "High-energy conversation detected".to_string(),
                    duration: Duration::from_minutes(10),
                });
            }
        }
        
        // Music adaptation
        if let AudioContentType::Music(music_info) = &classification.content_type {
            if perception.energy > 0.8 { // High energy music
                recommendations.push(AdaptationRecommendation::EnhancedVisualization {
                    reason: "High-energy music detected".to_string(),
                    visualization_type: VisualizationType::Energetic,
                });
            }
            
            if emotion.valence > 0.7 { // Happy music
                recommendations.push(AdaptationRecommendation::PositiveInterfaceTheme {
                    reason: "Positive music mood detected".to_string(),
                    theme: "vibrant".to_string(),
                });
            }
        }
        
        // Environmental adaptations
        match classification.environment_type {
            EnvironmentType::Office => {
                recommendations.push(AdaptationRecommendation::ProductivityMode {
                    reason: "Office environment detected".to_string(),
                });
            }
            EnvironmentType::Nature => {
                recommendations.push(AdaptationRecommendation::RelaxedMode {
                    reason: "Natural environment detected".to_string(),
                });
            }
            EnvironmentType::Urban => {
                recommendations.push(AdaptationRecommendation::UrbanMode {
                    reason: "Urban environment detected".to_string(),
                });
            }
            _ => {}
        }
        
        Ok(recommendations)
    }
}
```

### Integration Points

1. **Kuramoto Sensor Fusion**: Audio perception as synchronized sensor input with phase coupling
2. **Agent Context**: Rich audio understanding feeds agent contextual awareness
3. **Consciousness Metrics**: Audio diversity contributes to phone consciousness complexity (Xi)
4. **Wave Memory**: Audio experiences stored as memory waves with acoustic properties
5. **MSI Events**: All audio perception events flow through MSI event bus
6. **Hardware Acceleration**: NPU/DSP acceleration for real-time feature extraction

---

## Implementation

### Phase 1: Core Audio Perception Pipeline

**Location**: `system/audio/perception/src/perception_engine.rs`

```rust
use rustfft::{FftPlanner, num_complex::Complex};
use apodize::{hanning_iter, hamming_iter};

/// Core audio perception engine with 296-dimensional feature extraction
pub struct PerceptionEngine {
    // Audio processing
    sample_rate: f32,
    frame_size: usize,
    hop_size: usize,
    
    // FFT processing
    fft_planner: FftPlanner<f32>,
    fft_buffer: Vec<Complex<f32>>,
    window_function: WindowFunction,
    
    // Feature extractors
    spectral_extractor: SpectralFeatureExtractor,
    temporal_extractor: TemporalFeatureExtractor,
    harmonic_extractor: HarmonicFeatureExtractor,
    perceptual_extractor: PerceptualFeatureExtractor,
    emotion_extractor: EmotionFeatureExtractor,
    
    // Processing optimization
    feature_cache: FeatureCache,
    processing_pool: ThreadPool,
}

/// Spectral feature extraction (128 dimensions)
pub struct SpectralFeatureExtractor {
    mel_filter_bank: MelFilterBank,
    mfcc_calculator: MFCCCalculator,
    spectral_moments_calculator: SpectralMomentsCalculator,
}

impl SpectralFeatureExtractor {
    /// Extract comprehensive spectral features
    pub async fn extract(
        &mut self,
        frequency_spectrum: &[Complex<f32>],
        sample_rate: f32
    ) -> Result<SpectralFeatures> {
        // Convert complex spectrum to magnitude
        let magnitude_spectrum: Vec<f32> = frequency_spectrum
            .iter()
            .map(|c| c.norm())
            .collect();
            
        // Extract mel spectrogram (64 dimensions)
        let mel_spectrogram = self.mel_filter_bank.apply(&magnitude_spectrum, sample_rate)?;
        
        // Extract MFCC coefficients (13 dimensions)
        let mfcc_coefficients = self.mfcc_calculator.compute(&mel_spectrogram)?;
        
        // Extract spectral moments and characteristics
        let spectral_moments = self.spectral_moments_calculator.compute(&magnitude_spectrum, sample_rate)?;
        
        Ok(SpectralFeatures {
            mel_spectrogram,
            mfcc_coefficients,
            centroid: spectral_moments.centroid,
            bandwidth: spectral_moments.bandwidth,
            rolloff: spectral_moments.rolloff,
            zero_crossing_rate: self.calculate_zero_crossing_rate(frequency_spectrum)?,
            flux: self.calculate_spectral_flux(&magnitude_spectrum)?,
            slope: spectral_moments.slope,
            spread: spectral_moments.spread,
            skewness: spectral_moments.skewness,
            kurtosis: spectral_moments.kurtosis,
            entropy: self.calculate_spectral_entropy(&magnitude_spectrum)?,
            flatness: self.calculate_spectral_flatness(&magnitude_spectrum)?,
        })
    }
    
    /// Calculate spectral entropy
    fn calculate_spectral_entropy(&self, magnitude_spectrum: &[f32]) -> Result<f32> {
        let total_energy: f32 = magnitude_spectrum.iter().map(|x| x * x).sum();
        
        if total_energy == 0.0 {
            return Ok(0.0);
        }
        
        let entropy: f32 = magnitude_spectrum
            .iter()
            .map(|&x| {
                let normalized_energy = (x * x) / total_energy;
                if normalized_energy > 0.0 {
                    -normalized_energy * normalized_energy.log2()
                } else {
                    0.0
                }
            })
            .sum();
            
        Ok(entropy)
    }
    
    /// Calculate spectral flatness (Wiener entropy)
    fn calculate_spectral_flatness(&self, magnitude_spectrum: &[f32]) -> Result<f32> {
        let geometric_mean = self.geometric_mean(magnitude_spectrum)?;
        let arithmetic_mean = magnitude_spectrum.iter().sum::<f32>() / magnitude_spectrum.len() as f32;
        
        if arithmetic_mean == 0.0 {
            Ok(0.0)
        } else {
            Ok(geometric_mean / arithmetic_mean)
        }
    }
    
    /// Calculate geometric mean
    fn geometric_mean(&self, values: &[f32]) -> Result<f32> {
        let log_sum: f32 = values
            .iter()
            .map(|&x| (x + 1e-10).ln()) // Add small epsilon to avoid log(0)
            .sum();
            
        Ok((log_sum / values.len() as f32).exp())
    }
}

/// Temporal feature extraction (64 dimensions)
pub struct TemporalFeatureExtractor {
    onset_detector: OnsetDetector,
    tempo_estimator: TempoEstimator,
    envelope_analyzer: EnvelopeAnalyzer,
    rhythm_analyzer: RhythmAnalyzer,
}

impl TemporalFeatureExtractor {
    /// Extract temporal features from audio signal
    pub async fn extract(
        &mut self,
        audio_signal: &[f32],
        perception_history: &PerceptionHistory
    ) -> Result<TemporalFeatures> {
        // Detect onsets (note beginnings)
        let onsets = self.onset_detector.detect_onsets(audio_signal)?;
        let onset_density = onsets.len() as f32 / audio_signal.len() as f32;
        
        // Estimate tempo
        let tempo_result = self.tempo_estimator.estimate_tempo(audio_signal, &onsets)?;
        
        // Analyze envelope characteristics
        let envelope = self.envelope_analyzer.extract_envelope(audio_signal)?;
        let envelope_features = self.envelope_analyzer.analyze_envelope(&envelope)?;
        
        // Analyze rhythmic patterns
        let rhythm_features = self.rhythm_analyzer.analyze_rhythm(audio_signal, &onsets)?;
        
        // Calculate temporal centroid
        let temporal_centroid = self.calculate_temporal_centroid(audio_signal)?;
        
        Ok(TemporalFeatures {
            tempo: tempo_result.tempo,
            rhythm_strength: rhythm_features.strength,
            onset_density,
            pulse_clarity: tempo_result.clarity,
            tempo_stability: self.calculate_tempo_stability(&tempo_result, perception_history)?,
            rhythm_regularity: rhythm_features.regularity,
            temporal_centroid,
            attack_time: envelope_features.attack_time,
            decay_time: envelope_features.decay_time,
            sustain_level: envelope_features.sustain_level,
            release_time: envelope_features.release_time,
            envelope_dynamics: envelope_features.dynamics, // 53-dimensional envelope shape
        })
    }
    
    /// Calculate temporal centroid (center of mass in time domain)
    fn calculate_temporal_centroid(&self, audio_signal: &[f32]) -> Result<f32> {
        let squared_signal: Vec<f32> = audio_signal.iter().map(|x| x * x).collect();
        let total_energy: f32 = squared_signal.iter().sum();
        
        if total_energy == 0.0 {
            return Ok(0.5); // Middle of signal if no energy
        }
        
        let weighted_sum: f32 = squared_signal
            .iter()
            .enumerate()
            .map(|(i, &energy)| i as f32 * energy)
            .sum();
            
        Ok(weighted_sum / total_energy / audio_signal.len() as f32)
    }
    
    /// Calculate tempo stability over time
    fn calculate_tempo_stability(
        &self,
        current_tempo: &TempoEstimationResult,
        history: &PerceptionHistory
    ) -> Result<f32> {
        let recent_tempos: Vec<f32> = history.recent_perceptions(Duration::from_secs(10))
            .iter()
            .map(|p| p.tempo_estimate)
            .collect();
            
        if recent_tempos.len() < 3 {
            return Ok(1.0); // Assume stable if insufficient history
        }
        
        // Calculate coefficient of variation
        let mean_tempo = recent_tempos.iter().sum::<f32>() / recent_tempos.len() as f32;
        let variance = recent_tempos
            .iter()
            .map(|tempo| (tempo - mean_tempo).powi(2))
            .sum::<f32>() / recent_tempos.len() as f32;
            
        let std_dev = variance.sqrt();
        let coefficient_variation = std_dev / mean_tempo;
        
        // Stability is inverse of variation (clamped to [0, 1])
        Ok((1.0 - coefficient_variation).max(0.0).min(1.0))
    }
}

/// Harmonic feature extraction (48 dimensions)
pub struct HarmonicFeatureExtractor {
    pitch_detector: PitchDetector,
    chroma_extractor: ChromaExtractor,
    key_estimator: KeyEstimator,
    harmonic_analyzer: HarmonicAnalyzer,
}

impl HarmonicFeatureExtractor {
    /// Extract harmonic and tonal features
    pub async fn extract(
        &mut self,
        frequency_spectrum: &[Complex<f32>],
        time_signal: &[f32]
    ) -> Result<HarmonicFeatures> {
        // Detect fundamental pitch
        let pitch_result = self.pitch_detector.detect_pitch(time_signal)?;
        
        // Extract chroma features (12 pitch classes)
        let chroma_vector = self.chroma_extractor.extract_chroma(frequency_spectrum)?;
        
        // Estimate musical key
        let key_result = self.key_estimator.estimate_key(&chroma_vector)?;
        
        // Analyze harmonic content
        let harmonic_analysis = self.harmonic_analyzer.analyze_harmonics(
            frequency_spectrum,
            pitch_result.pitch
        )?;
        
        // Find spectral peaks
        let spectral_peaks = self.find_spectral_peaks(frequency_spectrum, 10)?;
        
        Ok(HarmonicFeatures {
            pitch_estimate: pitch_result.pitch,
            pitch_confidence: pitch_result.confidence,
            pitch_salience: pitch_result.salience,
            harmonic_ratio: harmonic_analysis.harmonic_ratio,
            inharmonicity: harmonic_analysis.inharmonicity,
            chroma_vector,
            key_estimate: key_result.key,
            key_confidence: key_result.confidence,
            mode_estimate: key_result.mode,
            chord_estimate: self.estimate_chord(&chroma_vector)?,
            harmonic_energy: harmonic_analysis.harmonic_energy, // 20 harmonic bands
            spectral_peaks, // 10 most prominent peaks
        })
    }
    
    /// Estimate chord from chroma vector
    fn estimate_chord(&self, chroma: &[f32]) -> Result<Option<String>> {
        // Simple chord templates (major and minor triads)
        let chord_templates = [
            ("C", vec![1.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0]),
            ("Dm", vec![0.0, 0.0, 1.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0]),
            // ... more chord templates
        ];
        
        let mut best_match = None;
        let mut best_correlation = 0.5; // Minimum correlation threshold
        
        for (chord_name, template) in &chord_templates {
            let correlation = self.calculate_correlation(chroma, template)?;
            if correlation > best_correlation {
                best_correlation = correlation;
                best_match = Some(chord_name.to_string());
            }
        }
        
        Ok(best_match)
    }
    
    /// Calculate correlation between two vectors
    fn calculate_correlation(&self, a: &[f32], b: &[f32]) -> Result<f32> {
        if a.len() != b.len() {
            return Err(AudioError::VectorLengthMismatch);
        }
        
        let dot_product: f32 = a.iter().zip(b.iter()).map(|(x, y)| x * y).sum();
        let norm_a: f32 = a.iter().map(|x| x * x).sum::<f32>().sqrt();
        let norm_b: f32 = b.iter().map(|x| x * x).sum::<f32>().sqrt();
        
        if norm_a == 0.0 || norm_b == 0.0 {
            Ok(0.0)
        } else {
            Ok(dot_product / (norm_a * norm_b))
        }
    }
}
```

### Phase 2: Emotional and Perceptual Feature Extraction

**Location**: `system/audio/perception/src/emotion_extractor.rs`

```rust
/// Emotional and valence feature extraction (24 dimensions)
pub struct EmotionFeatureExtractor {
    valence_model: ValenceModel,
    arousal_model: ArousalModel,
    emotion_classifier: EmotionClassifier,
    emotional_dynamics_analyzer: EmotionalDynamicsAnalyzer,
}

/// Valence model for positive/negative emotion detection
pub struct ValenceModel {
    feature_weights: Vec<f32>,
    emotional_mappings: HashMap<String, f32>,
    temporal_smoothing: ExponentialSmoothing,
}

impl EmotionFeatureExtractor {
    /// Extract emotional features from audio characteristics
    pub async fn extract(
        &mut self,
        spectral_features: &SpectralFeatures,
        temporal_features: &TemporalFeatures,
        harmonic_features: &HarmonicFeatures
    ) -> Result<EmotionFeatures> {
        // Calculate valence (positive vs negative emotion)
        let valence = self.calculate_valence(spectral_features, harmonic_features).await?;
        
        // Calculate arousal (energy/activation level)
        let arousal = self.calculate_arousal(spectral_features, temporal_features).await?;
        
        // Calculate dominance (sense of power/control)
        let dominance = self.calculate_dominance(spectral_features, harmonic_features).await?;
        
        // Calculate musical tension
        let tension = self.calculate_musical_tension(harmonic_features).await?;
        
        // Calculate discrete emotions
        let emotions = self.classify_discrete_emotions(
            spectral_features,
            temporal_features,
            harmonic_features
        ).await?;
        
        // Analyze emotional dynamics over time
        let emotional_dynamics = self.emotional_dynamics_analyzer.analyze(
            valence,
            arousal,
            &emotions
        ).await?;
        
        Ok(EmotionFeatures {
            valence,
            arousal,
            dominance,
            tension,
            energy: temporal_features.rhythm_strength * spectral_features.centroid / 2000.0,
            danceability: self.calculate_danceability(temporal_features)?,
            happiness: emotions.happiness,
            sadness: emotions.sadness,
            anger: emotions.anger,
            fear: emotions.fear,
            surprise: emotions.surprise,
            disgust: emotions.disgust,
            emotional_dynamics, // 12-dimensional vector of emotional change patterns
        })
    }
    
    /// Calculate valence using spectral and harmonic characteristics
    async fn calculate_valence(
        &self,
        spectral: &SpectralFeatures,
        harmonic: &HarmonicFeatures
    ) -> Result<f32> {
        let mut valence_score = 0.0;
        
        // Major keys tend to be more positive
        if harmonic.mode_estimate == 0 { // Major mode
            valence_score += 0.3;
        } else {
            valence_score -= 0.2; // Minor mode
        }
        
        // Brightness contributes to positive valence
        let brightness = spectral.centroid / 2000.0; // Normalize
        valence_score += brightness * 0.4;
        
        // Harmonic ratio (tonal vs noise) affects valence
        valence_score += harmonic.harmonic_ratio * 0.3;
        
        // Apply smoothing
        let smoothed_valence = self.valence_model.temporal_smoothing.update(valence_score);
        
        // Normalize to [-1, 1] range
        Ok(smoothed_valence.tanh())
    }
    
    /// Calculate arousal using energy and temporal characteristics
    async fn calculate_arousal(
        &self,
        spectral: &SpectralFeatures,
        temporal: &TemporalFeatures
    ) -> Result<f32> {
        let mut arousal_score = 0.0;
        
        // Tempo contributes to arousal
        arousal_score += (temporal.tempo / 180.0).min(1.0) * 0.4;
        
        // Spectral energy contributes to arousal
        let spectral_energy = spectral.bandwidth / 1000.0;
        arousal_score += spectral_energy.min(1.0) * 0.3;
        
        // Attack time affects arousal (quick attacks = high arousal)
        if temporal.attack_time > 0.0 {
            arousal_score += (1.0 / temporal.attack_time).min(1.0) * 0.3;
        }
        
        // Normalize to [0, 1] range
        Ok(arousal_score.min(1.0))
    }
    
    /// Calculate danceability based on rhythmic characteristics
    fn calculate_danceability(&self, temporal: &TemporalFeatures) -> Result<f32> {
        let mut danceability = 0.0;
        
        // Ideal tempo range for dancing (120-140 BPM)
        let tempo_factor = if temporal.tempo >= 120.0 && temporal.tempo <= 140.0 {
            1.0
        } else {
            let distance_from_ideal = (temporal.tempo - 130.0).abs();
            (1.0 - distance_from_ideal / 50.0).max(0.0)
        };
        
        danceability += tempo_factor * 0.4;
        
        // Strong rhythm increases danceability
        danceability += temporal.rhythm_strength * 0.3;
        
        // Regular rhythm increases danceability
        danceability += temporal.rhythm_regularity * 0.3;
        
        Ok(danceability.min(1.0))
    }
}

/// Perceptual feature extraction (32 dimensions)
pub struct PerceptualFeatureExtractor {
    loudness_calculator: LoudnessCalculator,
    sharpness_calculator: SharpnessCalculator,
    roughness_calculator: RoughnessCalculator,
    binaural_analyzer: BinauralAnalyzer,
}

impl PerceptualFeatureExtractor {
    /// Extract perceptual features based on psychoacoustic models
    pub async fn extract(
        &mut self,
        frequency_spectrum: &[Complex<f32>],
        time_signal: &[f32]
    ) -> Result<PerceptualFeatures> {
        // Calculate loudness in sones (perceptual loudness unit)
        let loudness_sone = self.loudness_calculator.calculate_loudness(frequency_spectrum)?;
        
        // Calculate sharpness in acums (perceptual sharpness unit)
        let sharpness_acum = self.sharpness_calculator.calculate_sharpness(frequency_spectrum)?;
        
        // Calculate roughness in aspers (perceptual roughness unit)
        let roughness_asper = self.roughness_calculator.calculate_roughness(frequency_spectrum)?;
        
        // Analyze binaural/spatial characteristics
        let binaural_qualities = self.binaural_analyzer.analyze(time_signal)?;
        
        // Calculate perceptual brightness
        let brightness = self.calculate_brightness(frequency_spectrum)?;
        
        // Calculate perceptual warmth
        let warmth = self.calculate_warmth(frequency_spectrum)?;
        
        Ok(PerceptualFeatures {
            loudness_sone,
            sharpness_acum,
            roughness_asper,
            fluctuation_strength: self.calculate_fluctuation_strength(time_signal)?,
            tonality: self.calculate_tonality(frequency_spectrum)?,
            brightness,
            warmth,
            fullness: self.calculate_fullness(frequency_spectrum)?,
            clarity: self.calculate_clarity(frequency_spectrum)?,
            presence: self.calculate_presence(frequency_spectrum)?,
            spaciousness: binaural_qualities.spaciousness,
            envelopment: binaural_qualities.envelopment,
            intimacy: binaural_qualities.intimacy,
            reverberance: self.calculate_reverberance(time_signal)?,
            echo_density: self.calculate_echo_density(time_signal)?,
            stereo_width: binaural_qualities.stereo_width,
            binaural_qualities: binaural_qualities.detailed_features, // 16 dimensions
        })
    }
}
```

---

## Consequences

### Positive

1. **Semantic Audio Understanding**: Phone inherently understands what it hears, not just raw audio data
2. **Context-Aware Adaptation**: Agent behavior adapts automatically to acoustic environment
3. **Rich Environmental Awareness**: 296-dimensional perception provides nuanced audio understanding
4. **Emotional Intelligence**: Phone detects and responds to emotional content in audio
5. **Predictive Audio Behavior**: Phone anticipates audio changes and prepares appropriate responses
6. **Enhanced User Experience**: Audio-aware interactions feel more natural and responsive
7. **Cross-Modal Integration**: Audio perception integrates with other sensors via Kuramoto coupling

### Negative

1. **Computational Overhead**: Real-time 296-dimensional feature extraction requires significant processing
2. **Battery Impact**: Continuous audio perception and analysis affects battery life
3. **Privacy Concerns**: Always-on audio analysis may raise privacy questions
4. **Storage Requirements**: Rich audio perception history requires storage space
5. **Hardware Requirements**: NPU/DSP acceleration needed for optimal performance

### Neutral

1. **User Awareness**: Users may need education about audio perception capabilities and privacy controls
2. **Application Adaptation**: Apps may need updates to take advantage of semantic audio understanding
3. **Privacy Controls**: Need granular controls for what audio perception data is used and shared

---

## Implementation Timeline

### Phase 1 (Weeks 1-3): Core Perception Pipeline
- Implement spectral feature extraction with mel spectrograms and MFCC
- Build temporal feature extraction for rhythm and envelope analysis
- Create harmonic feature extraction for pitch and chroma analysis

### Phase 2 (Weeks 4-6): Advanced Perception Features
- Implement perceptual feature extraction based on psychoacoustic models
- Build emotional and valence feature extraction
- Add real-time processing optimization and hardware acceleration

### Phase 3 (Weeks 7-9): Classification and Understanding
- Create semantic audio classification (speech, music, environment)
- Build emotion detection and musical understanding
- Add context-aware adaptation recommendation system

### Phase 4 (Weeks 10-12): System Integration
- Integrate with Kuramoto sensor fusion for synchronized audio perception
- Connect to agent context for audio-aware behavior adaptation
- Feed into consciousness metrics for audio diversity contribution

### Phase 5 (Weeks 13-15): Optimization & Privacy
- Optimize for mobile hardware performance and battery usage
- Add comprehensive privacy controls and user preferences
- Create audio perception analytics and debugging tools

---

## References

- [kannaka-ear Repository](../../../kannaka-ear/) — Original 296-dimensional audio perception research
- [Audio Signal Processing](https://www.amazon.com/Audio-Signal-Processing-MATLAB-Examples/dp/0470875993) — Signal processing foundation
- [Music Information Retrieval](https://musicinformationretrieval.com/) — MIR techniques and features
- [Psychoacoustics](https://www.amazon.com/Psychoacoustics-Facts-Models-Hugo-Fastl/dp/3540231595) — Perceptual audio modeling
- [Emotion Recognition from Audio](https://link.springer.com/book/10.1007/978-3-319-75427-9) — Emotional audio analysis
- [Real-Time Audio Processing](https://www.amazon.com/Real-Time-Digital-Signal-Processing-Implementation/dp/0470014954) — Real-time implementation techniques
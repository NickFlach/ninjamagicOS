# ADR-006: Prediction Markets for Agent Decision-Making

**Status:** Proposed  
**Date:** 2026-03-20  
**Authors:** Nick Flach (Kannaka)  
**Supersedes:** N/A  
**Related:** ADR-001 (MSI Architecture), ADR-002 (Wave Memory)

---

## Context

Current AI agent decision-making typically uses naive approaches like argmax over model outputs or simple confidence scoring. This leads to overconfident decisions and poor uncertainty calibration. When an agent needs to choose between multiple skills, route user intents, or make predictions about outcomes, these approaches don't capture the nuanced uncertainty and competing hypotheses that characterize real-world decision scenarios.

The ghostsignals-rs research demonstrates a superior approach: **internal prediction markets** using LMSR (Logarithmic Market Scoring Rule) where multiple "expert" subsystems bet on outcomes. Market prices naturally represent calibrated probability estimates, and the scoring rule incentivizes honest reporting of beliefs. This has been proven with 61 comprehensive tests in a clean Rust implementation.

In ninjamagicOS, the agent should use prediction markets for all major decision points: skill selection, intent routing, context prediction, resource allocation, and user preference modeling. Instead of single-model confidence, we get multi-model consensus with proper uncertainty quantification.

### Current Agent Decision-Making (Limitation)

```rust
// Current naive approach in agent/core/src/intent.rs
pub fn route_intent(intent: &UserIntent) -> Result<SkillId> {
    let mut best_skill = None;
    let mut best_score = 0.0;
    
    for skill in &self.skills {
        let confidence = skill.calculate_confidence(intent);
        if confidence > best_score {
            best_score = confidence;
            best_skill = Some(skill.id);
        }
    }
    
    best_skill.ok_or(IntentError::NoMatchingSkill)
}
```

### Required Prediction Market Properties

From ghostsignals-rs research, effective internal prediction markets require:

1. **LMSR Implementation**: Logarithmic Market Scoring Rule for proper scoring incentives
2. **Multiple Expert Agents**: Different subsystems as market participants with specialized knowledge
3. **Continuous Price Discovery**: Real-time market price updates as new evidence arrives
4. **Proper Scoring**: Incentive alignment for honest belief reporting
5. **Market Clearing**: Efficient computation of market equilibrium prices
6. **Temporal Markets**: Markets for time-dependent predictions with decay

---

## Decision

**We will integrate LMSR-based prediction markets as the core decision-making mechanism for the ninjamagicOS agent, replacing naive confidence scoring with calibrated uncertainty quantification through multi-expert market consensus.**

### Architecture: Agent Prediction Market System

```rust
/// Internal prediction market for agent decision-making
pub struct AgentPredictionMarket {
    market_id: MarketId,
    question: MarketQuestion,
    outcomes: Vec<MarketOutcome>,
    
    // LMSR market maker
    market_maker: LMSRMarketMaker,
    
    // Expert participants
    experts: HashMap<ExpertId, ExpertAgent>,
    positions: HashMap<ExpertId, HashMap<OutcomeId, f64>>, // Expert positions
    
    // Market state
    current_prices: HashMap<OutcomeId, f64>,
    total_liquidity: f64,
    market_volume: f64,
    
    // Temporal properties
    creation_time: SystemTime,
    resolution_time: Option<SystemTime>,
    decay_rate: f64, // For time-sensitive decisions
    
    // MSI integration
    market_lane: LaneHandle,
    decision_events: EventBus,
}

/// LMSR (Logarithmic Market Scoring Rule) market maker
pub struct LMSRMarketMaker {
    liquidity_parameter: f64,  // b in LMSR formula
    outcome_shares: HashMap<OutcomeId, f64>,
    cost_function_cache: f64,
}

/// Expert agent participating in prediction markets
pub struct ExpertAgent {
    expert_id: ExpertId,
    expert_type: ExpertType,
    specialization: ExpertSpecialization,
    
    // Performance tracking
    prediction_history: VecDeque<PredictionRecord>,
    accuracy_score: f64,
    calibration_score: f64,
    
    // Market participation
    current_budget: f64,
    risk_tolerance: f64,
    trading_frequency: f64,
}

#[derive(Clone, Debug)]
pub enum ExpertType {
    ContextAnalyzer,      // Specializes in user context understanding
    SkillMatcher,         // Specializes in skill-intent matching
    ResourcePredictor,    // Specializes in resource availability prediction
    UserBehaviorModeler,  // Specializes in user preference prediction
    EnvironmentSensor,    // Specializes in environmental context
    TemporalPatternMiner, // Specializes in temporal patterns
    SemanticAnalyzer,     // Specializes in semantic understanding
    AnomalyDetector,      // Specializes in detecting unusual situations
}

impl AgentPredictionMarket {
    /// Create prediction market for intent routing decision
    pub async fn create_intent_routing_market(
        intent: &UserIntent,
        available_skills: &[SkillId]
    ) -> Result<Self> {
        let outcomes: Vec<MarketOutcome> = available_skills.iter()
            .map(|&skill_id| MarketOutcome {
                id: OutcomeId::Skill(skill_id),
                description: format!("Intent matches skill {}", skill_id),
                initial_probability: 1.0 / available_skills.len() as f64,
            })
            .collect();
            
        let market = Self::new(
            MarketQuestion::IntentRouting {
                intent: intent.clone(),
                context: self.get_current_context().await?,
            },
            outcomes,
            INTENT_ROUTING_LIQUIDITY,
            Some(Duration::from_secs(30)), // Quick resolution for real-time routing
        )?;
        
        Ok(market)
    }
    
    /// Expert agents submit predictions to the market
    pub async fn submit_expert_prediction(
        &mut self,
        expert_id: ExpertId,
        predictions: HashMap<OutcomeId, f64>
    ) -> Result<MarketTransaction> {
        let expert = self.experts.get(&expert_id)
            .ok_or(MarketError::ExpertNotFound(expert_id))?;
            
        // Validate prediction probabilities sum to 1.0
        let total_prob: f64 = predictions.values().sum();
        if (total_prob - 1.0).abs() > 0.001 {
            return Err(MarketError::InvalidProbabilities(total_prob));
        }
        
        // Convert predictions to desired share positions
        let mut desired_positions = HashMap::new();
        for (outcome_id, probability) in predictions {
            let current_price = self.current_prices[&outcome_id];
            let desired_shares = (probability - current_price) * expert.risk_tolerance * expert.current_budget;
            desired_positions.insert(outcome_id, desired_shares);
        }
        
        // Execute trades through LMSR market maker
        let transaction = self.execute_lmsr_trades(expert_id, desired_positions).await?;
        
        // Update expert positions
        for (outcome_id, share_change) in &transaction.share_changes {
            let current_position = self.positions.get(&expert_id)
                .and_then(|positions| positions.get(&outcome_id))
                .copied()
                .unwrap_or(0.0);
            self.positions.entry(expert_id)
                .or_default()
                .insert(outcome_id, current_position + share_change);
        }
        
        // Update market prices based on new equilibrium
        self.update_market_prices().await?;
        
        Ok(transaction)
    }
    
    /// Execute trades using LMSR market maker
    async fn execute_lmsr_trades(
        &mut self,
        expert_id: ExpertId,
        desired_positions: HashMap<OutcomeId, f64>
    ) -> Result<MarketTransaction> {
        let mut total_cost = 0.0;
        let mut share_changes = HashMap::new();
        
        for (outcome_id, desired_shares) in desired_positions {
            let current_shares = self.market_maker.outcome_shares[&outcome_id];
            let share_change = desired_shares;
            
            // LMSR cost function: C(q) = b * ln(Σ exp(q_i / b))
            let cost_before = self.market_maker.compute_cost_function();
            
            // Update shares for this outcome
            self.market_maker.outcome_shares.insert(outcome_id, current_shares + share_change);
            
            let cost_after = self.market_maker.compute_cost_function();
            let marginal_cost = cost_after - cost_before;
            
            total_cost += marginal_cost;
            share_changes.insert(outcome_id, share_change);
        }
        
        // Deduct cost from expert budget
        if let Some(expert) = self.experts.get_mut(&expert_id) {
            expert.current_budget -= total_cost;
        }
        
        Ok(MarketTransaction {
            expert_id,
            timestamp: SystemTime::now(),
            total_cost,
            share_changes,
        })
    }
    
    /// Update current market prices based on LMSR equilibrium
    async fn update_market_prices(&mut self) -> Result<()> {
        let b = self.market_maker.liquidity_parameter;
        let total_exp_sum: f64 = self.market_maker.outcome_shares.values()
            .map(|&shares| (shares / b).exp())
            .sum();
            
        for (outcome_id, &shares) in &self.market_maker.outcome_shares {
            // LMSR price formula: p_i = exp(q_i / b) / Σ exp(q_j / b)
            let price = (shares / b).exp() / total_exp_sum;
            self.current_prices.insert(outcome_id, price);
        }
        
        Ok(())
    }
    
    /// Resolve market and settle payments
    pub async fn resolve_market(&mut self, actual_outcome: OutcomeId) -> Result<MarketResolution> {
        let resolution_time = SystemTime::now();
        let mut expert_payoffs = HashMap::new();
        
        // Calculate payoffs for each expert based on their positions
        for (expert_id, positions) in &self.positions {
            let mut total_payoff = 0.0;
            
            for (outcome_id, &shares) in positions {
                if outcome_id == actual_outcome {
                    total_payoff += shares; // Winning shares pay 1.0 each
                }
                // Losing shares pay 0.0
            }
            
            expert_payoffs.insert(*expert_id, total_payoff);
        }
        
        // Update expert performance metrics
        self.update_expert_performance(&expert_payoffs, actual_outcome).await?;
        
        // Publish resolution event
        let resolution_event = Event::new(
            "market/resolution",
            MarketResolutionPayload {
                market_id: self.market_id,
                resolved_outcome: actual_outcome,
                final_prices: self.current_prices.clone(),
                expert_payoffs,
                resolution_time,
            }
        );
        
        self.decision_events.publish(resolution_event).await?;
        
        Ok(MarketResolution {
            market_id: self.market_id,
            resolved_outcome: actual_outcome,
            resolution_time,
            total_volume: self.market_volume,
        })
    }
}
```

### Integration Points

1. **Intent Router**: Replace confidence scoring with prediction market consensus for skill selection
2. **Context Predictor**: Market-based prediction of user context changes and environment shifts
3. **Resource Allocator**: Prediction markets for NPU/CPU/memory demand forecasting
4. **Skill Scheduler**: Markets predict which skills will be needed next for proactive loading
5. **User Model**: Markets aggregate different models of user preferences and behavior patterns
6. **Anomaly Detection**: Markets predict likelihood of various anomaly types from sensor patterns

---

## Implementation

### Phase 1: LMSR Market Maker Core

**Location**: `agent/core/src/markets/lmsr.rs`

```rust
use std::collections::HashMap;
use std::f64::consts::E;

/// LMSR (Logarithmic Market Scoring Rule) market maker implementation
pub struct LMSRMarketMaker {
    pub liquidity_parameter: f64,  // b - controls market liquidity vs price sensitivity
    pub outcome_shares: HashMap<OutcomeId, f64>, // q_i - shares outstanding for each outcome
    cost_cache: Option<f64>,       // Cached cost function value
    cache_valid: bool,             // Whether cache is still valid
}

impl LMSRMarketMaker {
    /// Create new LMSR market maker with uniform initial distribution
    pub fn new(outcomes: Vec<OutcomeId>, liquidity_parameter: f64) -> Self {
        let n_outcomes = outcomes.len() as f64;
        let initial_shares_per_outcome = 0.0; // Start with no outstanding shares
        
        let mut outcome_shares = HashMap::new();
        for outcome in outcomes {
            outcome_shares.insert(outcome, initial_shares_per_outcome);
        }
        
        Self {
            liquidity_parameter,
            outcome_shares,
            cost_cache: None,
            cache_valid: false,
        }
    }
    
    /// LMSR cost function: C(q) = b * ln(Σ exp(q_i / b))
    pub fn compute_cost_function(&mut self) -> f64 {
        if self.cache_valid {
            return self.cost_cache.unwrap_or(0.0);
        }
        
        let b = self.liquidity_parameter;
        let sum_exp: f64 = self.outcome_shares.values()
            .map(|&shares| (shares / b).exp())
            .sum();
            
        let cost = b * sum_exp.ln();
        
        self.cost_cache = Some(cost);
        self.cache_valid = true;
        
        cost
    }
    
    /// Compute marginal cost of purchasing delta shares of outcome i
    pub fn compute_marginal_cost(&mut self, outcome_id: OutcomeId, delta_shares: f64) -> Result<f64> {
        let cost_before = self.compute_cost_function();
        
        // Temporarily update shares
        let original_shares = self.outcome_shares[&outcome_id];
        self.outcome_shares.insert(outcome_id, original_shares + delta_shares);
        self.invalidate_cache();
        
        let cost_after = self.compute_cost_function();
        
        // Restore original shares
        self.outcome_shares.insert(outcome_id, original_shares);
        self.invalidate_cache();
        
        Ok(cost_after - cost_before)
    }
    
    /// Get current market price for outcome i: p_i = ∂C/∂q_i = exp(q_i/b) / Σ exp(q_j/b)
    pub fn get_price(&mut self, outcome_id: OutcomeId) -> Result<f64> {
        let b = self.liquidity_parameter;
        let outcome_shares = self.outcome_shares[&outcome_id];
        
        let numerator = (outcome_shares / b).exp();
        let denominator: f64 = self.outcome_shares.values()
            .map(|&shares| (shares / b).exp())
            .sum();
            
        Ok(numerator / denominator)
    }
    
    /// Get all current prices
    pub fn get_all_prices(&mut self) -> Result<HashMap<OutcomeId, f64>> {
        let b = self.liquidity_parameter;
        let total_exp_sum: f64 = self.outcome_shares.values()
            .map(|&shares| (shares / b).exp())
            .sum();
            
        let mut prices = HashMap::new();
        for (outcome_id, &shares) in &self.outcome_shares {
            let price = (shares / b).exp() / total_exp_sum;
            prices.insert(*outcome_id, price);
        }
        
        Ok(prices)
    }
    
    /// Execute trade: buy delta_shares of outcome_id
    pub fn execute_trade(&mut self, outcome_id: OutcomeId, delta_shares: f64) -> Result<TradeCost> {
        let cost_before = self.compute_cost_function();
        
        // Update shares
        let original_shares = self.outcome_shares[&outcome_id];
        self.outcome_shares.insert(outcome_id, original_shares + delta_shares);
        self.invalidate_cache();
        
        let cost_after = self.compute_cost_function();
        let trade_cost = cost_after - cost_before;
        
        Ok(TradeCost {
            outcome_id,
            shares_purchased: delta_shares,
            cost: trade_cost,
            new_total_shares: original_shares + delta_shares,
        })
    }
    
    fn invalidate_cache(&mut self) {
        self.cache_valid = false;
    }
}

#[derive(Clone, Debug)]
pub struct TradeCost {
    pub outcome_id: OutcomeId,
    pub shares_purchased: f64,
    pub cost: f64,
    pub new_total_shares: f64,
}
```

### Phase 2: Expert Agent Framework

**Location**: `agent/core/src/markets/experts.rs`

```rust
/// Expert agent that participates in prediction markets
pub struct ExpertAgent {
    pub expert_id: ExpertId,
    pub expert_type: ExpertType,
    pub specialization: ExpertSpecialization,
    
    // Models and data sources
    prediction_model: Box<dyn PredictionModel>,
    data_sources: Vec<DataSourceId>,
    
    // Performance tracking  
    prediction_history: VecDeque<PredictionRecord>,
    rolling_accuracy: RollingAverage,
    calibration_bins: CalibrationBins,
    
    // Market participation parameters
    current_budget: f64,
    risk_tolerance: f64,          // 0.0 (risk-averse) to 1.0 (risk-seeking)
    trading_frequency: f64,       // How often to update predictions
    confidence_threshold: f64,    // Minimum confidence to participate
    
    // Adaptation parameters
    learning_rate: f64,
    model_update_frequency: Duration,
    last_model_update: SystemTime,
}

#[derive(Clone, Debug)]
pub enum ExpertSpecialization {
    UserIntentMatching {
        intent_types: Vec<IntentType>,
        accuracy_per_type: HashMap<IntentType, f64>,
    },
    ContextPrediction {
        context_dimensions: Vec<ContextDimension>,
        temporal_horizon: Duration,
    },
    ResourceForecasting {
        resource_types: Vec<ResourceType>,
        prediction_horizon: Duration,
    },
    UserBehaviorModeling {
        behavior_patterns: Vec<BehaviorPattern>,
        user_segments: Vec<UserSegment>,
    },
    AnomalyDetection {
        anomaly_types: Vec<AnomalyType>,
        detection_sensitivity: f64,
    },
}

impl ExpertAgent {
    /// Generate prediction for a market question
    pub async fn generate_prediction(
        &mut self,
        question: &MarketQuestion
    ) -> Result<HashMap<OutcomeId, f64>> {
        // Check if this expert should participate based on specialization
        if !self.should_participate(question).await? {
            return Ok(HashMap::new()); // Don't participate if outside specialization
        }
        
        // Gather relevant data for prediction
        let input_data = self.gather_prediction_data(question).await?;
        
        // Generate prediction using expert's model
        let raw_predictions = self.prediction_model.predict(&input_data).await?;
        
        // Apply expert's confidence and risk tolerance
        let adjusted_predictions = self.apply_confidence_adjustment(raw_predictions).await?;
        
        // Normalize to ensure probabilities sum to 1.0
        let normalized_predictions = self.normalize_predictions(adjusted_predictions)?;
        
        // Update prediction history for performance tracking
        self.record_prediction(question.clone(), normalized_predictions.clone()).await?;
        
        Ok(normalized_predictions)
    }
    
    /// Determine if expert should participate based on specialization match
    async fn should_participate(&self, question: &MarketQuestion) -> Result<bool> {
        match (&self.specialization, question) {
            (ExpertSpecialization::UserIntentMatching { intent_types, .. }, 
             MarketQuestion::IntentRouting { intent, .. }) => {
                Ok(intent_types.contains(&intent.intent_type))
            }
            (ExpertSpecialization::ContextPrediction { temporal_horizon, .. }, 
             MarketQuestion::ContextChange { prediction_time, .. }) => {
                let time_diff = prediction_time.duration_since(SystemTime::now()).unwrap_or_default();
                Ok(time_diff <= *temporal_horizon)
            }
            (ExpertSpecialization::ResourceForecasting { resource_types, .. }, 
             MarketQuestion::ResourceDemand { resource_type, .. }) => {
                Ok(resource_types.contains(resource_type))
            }
            _ => Ok(false), // Not specialized for this question type
        }
    }
    
    /// Gather input data for prediction based on question type and expert data sources
    async fn gather_prediction_data(&self, question: &MarketQuestion) -> Result<PredictionInput> {
        let mut input_data = PredictionInput::new();
        
        for &data_source_id in &self.data_sources {
            let data = match data_source_id {
                DataSourceId::SensorFusion => {
                    self.get_sensor_fusion_data().await?
                }
                DataSourceId::UserInteractionHistory => {
                    self.get_interaction_history_data().await?
                }
                DataSourceId::ContextMemory => {
                    self.get_context_memory_data().await?
                }
                DataSourceId::ResourceMetrics => {
                    self.get_resource_metrics_data().await?
                }
                DataSourceId::TemporalPatterns => {
                    self.get_temporal_patterns_data().await?
                }
            };
            
            input_data.add_source_data(data_source_id, data);
        }
        
        // Add question-specific context
        input_data.add_question_context(question.clone());
        
        Ok(input_data)
    }
    
    /// Apply expert's confidence level and risk tolerance to raw predictions
    async fn apply_confidence_adjustment(
        &self,
        mut predictions: HashMap<OutcomeId, f64>
    ) -> Result<HashMap<OutcomeId, f64>> {
        // Calculate overall confidence in this prediction
        let prediction_confidence = self.compute_prediction_confidence(&predictions).await?;
        
        if prediction_confidence < self.confidence_threshold {
            // Low confidence: move predictions toward uniform distribution
            let n_outcomes = predictions.len() as f64;
            let uniform_prob = 1.0 / n_outcomes;
            
            for (_, prob) in predictions.iter_mut() {
                *prob = (1.0 - self.confidence_threshold) * uniform_prob + 
                       self.confidence_threshold * (*prob);
            }
        }
        
        // Apply risk tolerance: high risk tolerance = more extreme predictions
        for (_, prob) in predictions.iter_mut() {
            let deviation = *prob - 0.5; // Deviation from neutral 50%
            *prob = 0.5 + deviation * self.risk_tolerance;
        }
        
        Ok(predictions)
    }
    
    /// Update expert's model based on market resolution outcomes
    pub async fn update_from_resolution(
        &mut self,
        question: MarketQuestion,
        prediction: HashMap<OutcomeId, f64>,
        actual_outcome: OutcomeId,
        market_payoff: f64
    ) -> Result<()> {
        // Update accuracy tracking
        let predicted_prob = prediction.get(&actual_outcome).copied().unwrap_or(0.0);
        self.rolling_accuracy.add_sample(predicted_prob);
        
        // Update calibration bins
        self.calibration_bins.add_prediction(predicted_prob, 1.0); // Actual outcome occurred
        for (outcome_id, prob) in &prediction {
            if *outcome_id != actual_outcome {
                self.calibration_bins.add_prediction(*prob, 0.0); // Other outcomes didn't occur
            }
        }
        
        // Adjust risk tolerance based on performance
        if market_payoff > 0.0 {
            self.risk_tolerance = (self.risk_tolerance + 0.01).min(1.0); // Increase risk tolerance
        } else {
            self.risk_tolerance = (self.risk_tolerance - 0.01).max(0.1); // Decrease risk tolerance
        }
        
        // Update prediction model if enough time has passed
        if self.last_model_update.elapsed().unwrap_or_default() > self.model_update_frequency {
            self.update_prediction_model().await?;
            self.last_model_update = SystemTime::now();
        }
        
        Ok(())
    }
}
```

### Phase 3: Intent Routing Integration

**Location**: `agent/core/src/intent.rs` (modify existing)

```rust
use crate::markets::{AgentPredictionMarket, ExpertAgent, MarketQuestion};

impl IntentRouter {
    /// Route user intent using prediction market consensus
    pub async fn route_intent_market_based(
        &mut self,
        intent: &UserIntent
    ) -> Result<SkillRoutingDecision> {
        // Get available skills for this intent type
        let available_skills = self.get_available_skills(&intent.intent_type).await?;
        
        if available_skills.is_empty() {
            return Err(IntentError::NoAvailableSkills(intent.intent_type.clone()));
        }
        
        if available_skills.len() == 1 {
            // Only one skill available, no need for market
            return Ok(SkillRoutingDecision {
                chosen_skill: available_skills[0],
                confidence: 1.0,
                reasoning: "Only one skill available".to_string(),
                market_consensus: None,
            });
        }
        
        // Create prediction market for intent routing
        let mut market = AgentPredictionMarket::create_intent_routing_market(
            intent,
            &available_skills
        ).await?;
        
        // Get predictions from all relevant expert agents
        let expert_predictions = self.gather_expert_predictions(&intent, &market).await?;
        
        // Submit expert predictions to market
        for (expert_id, predictions) in expert_predictions {
            market.submit_expert_prediction(expert_id, predictions).await?;
        }
        
        // Market reaches equilibrium, get final prices
        let final_prices = market.get_current_prices().await?;
        
        // Choose skill with highest market price (highest consensus probability)
        let (chosen_skill_outcome, &consensus_probability) = final_prices.iter()
            .max_by(|(_, a), (_, b)| a.partial_cmp(b).unwrap_or(std::cmp::Ordering::Equal))
            .ok_or(IntentError::MarketConsensusFailure)?;
            
        let chosen_skill = match chosen_skill_outcome {
            OutcomeId::Skill(skill_id) => *skill_id,
            _ => return Err(IntentError::InvalidOutcomeType),
        };
        
        // Store market for later resolution
        let market_id = market.market_id;
        self.active_markets.insert(market_id, market);
        
        Ok(SkillRoutingDecision {
            chosen_skill,
            confidence: consensus_probability,
            reasoning: format!("Market consensus: {:.1}% probability", consensus_probability * 100.0),
            market_consensus: Some(MarketConsensus {
                market_id,
                final_prices,
                participating_experts: expert_predictions.keys().cloned().collect(),
            }),
        })
    }
    
    /// Gather predictions from expert agents specialized in intent matching
    async fn gather_expert_predictions(
        &self,
        intent: &UserIntent,
        market: &AgentPredictionMarket
    ) -> Result<HashMap<ExpertId, HashMap<OutcomeId, f64>>> {
        let mut expert_predictions = HashMap::new();
        
        for expert in &self.expert_agents {
            if expert.specializes_in_intent_matching() {
                let predictions = expert.generate_prediction(&market.question).await?;
                if !predictions.is_empty() {
                    expert_predictions.insert(expert.expert_id, predictions);
                }
            }
        }
        
        Ok(expert_predictions)
    }
    
    /// Resolve market based on actual skill execution outcome
    pub async fn resolve_intent_market(
        &mut self,
        market_id: MarketId,
        executed_skill: SkillId,
        execution_success: bool
    ) -> Result<()> {
        if let Some(mut market) = self.active_markets.remove(&market_id) {
            let actual_outcome = if execution_success {
                OutcomeId::Skill(executed_skill)
            } else {
                OutcomeId::ExecutionFailure
            };
            
            let resolution = market.resolve_market(actual_outcome).await?;
            
            // Update expert performance based on market resolution
            for expert in &mut self.expert_agents {
                if let Some(prediction) = market.get_expert_prediction(expert.expert_id) {
                    let payoff = market.get_expert_payoff(expert.expert_id);
                    expert.update_from_resolution(
                        market.question.clone(),
                        prediction,
                        actual_outcome,
                        payoff
                    ).await?;
                }
            }
        }
        
        Ok(())
    }
}

#[derive(Clone, Debug)]
pub struct SkillRoutingDecision {
    pub chosen_skill: SkillId,
    pub confidence: f64,
    pub reasoning: String,
    pub market_consensus: Option<MarketConsensus>,
}

#[derive(Clone, Debug)]
pub struct MarketConsensus {
    pub market_id: MarketId,
    pub final_prices: HashMap<OutcomeId, f64>,
    pub participating_experts: Vec<ExpertId>,
}
```

---

## Consequences

### Positive

1. **Calibrated Uncertainty**: Market prices provide well-calibrated probability estimates rather than overconfident predictions
2. **Multi-Model Consensus**: Combines insights from specialized expert agents rather than relying on single model
3. **Incentive Alignment**: LMSR scoring rule incentivizes honest reporting of beliefs from expert agents
4. **Adaptive Learning**: Expert performance tracking and model updates improve decision quality over time
5. **Uncertainty Quantification**: Clear probability distributions rather than binary confident/uncertain classifications
6. **Market Efficiency**: Prices efficiently aggregate information from multiple sources
7. **Proven Mathematics**: LMSR has solid theoretical foundations and proven convergence properties

### Negative

1. **Computational Overhead**: Running multiple expert models and market computation requires significant processing
2. **Implementation Complexity**: LMSR mathematics and market mechanics are more complex than simple confidence scoring
3. **Expert Coordination**: Managing multiple expert agents and their specializations adds coordination complexity
4. **Market Resolution**: Need to track outcomes and resolve markets to maintain expert performance metrics
5. **Calibration Time**: Expert agents need time and data to achieve good calibration

### Neutral

1. **Expertise Development**: System gradually develops specialized expert agents for different decision domains
2. **Market Granularity**: Can create markets for fine-grained or coarse-grained decisions as needed
3. **Budget Management**: Expert agents need budget allocation and management systems

---

## Implementation Timeline

### Phase 1 (Weeks 1-2): LMSR Core
- Implement LMSR market maker with cost function and pricing
- Add trade execution and market equilibrium computation
- Create comprehensive test suite for market mechanics

### Phase 2 (Weeks 3-4): Expert Agent Framework
- Build expert agent abstraction with specializations
- Add prediction model interfaces for different expert types
- Create performance tracking and calibration measurement

### Phase 3 (Weeks 5-6): Intent Routing Integration
- Replace existing intent router with market-based approach
- Create expert agents for intent matching and context analysis
- Add market resolution based on skill execution outcomes

### Phase 4 (Weeks 7-8): Additional Market Applications
- Create markets for resource allocation decisions
- Add context change prediction markets
- Build user preference modeling markets

### Phase 5 (Weeks 9-10): Optimization & Monitoring
- Optimize market computation performance for mobile hardware
- Add market monitoring and debugging interfaces
- Create market analytics for system performance insights

---

## References

- [ghostsignals-rs Repository](https://github.com/NickFlach/ghostsignals-rs) — Clean LMSR implementation with comprehensive tests
- [Logarithmic Market Scoring Rules for Modular Combinatorial Information Aggregation](https://www.cs.cmu.edu/~sandholm/LMSR.pdf) — LMSR theoretical foundation
- [Prediction Markets: Theory and Applications](https://www.amazon.com/Prediction-Markets-Theory-Applications-Economics/dp/1848441444) — Market design principles
- [Proper Scoring Rules](https://en.wikipedia.org/wiki/Scoring_rule#Proper_scoring_rules) — Mathematical foundation for incentive alignment
- [MSI Event System](../../msi/spec/README.md) — Event bus for market resolution events
//! SP Abstract Syntax Tree
//!
//! Represents the parsed structure of a Shinobi.Substrate program.

use serde::{Serialize, Deserialize};

/// A complete SP program.
#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct Program {
    pub domains: Vec<DomainDecl>,
    pub lanes: Vec<LaneDecl>,
}

/// Domain declaration with grants.
#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct DomainDecl {
    pub name: String,
    pub grants: Vec<GrantDecl>,
    pub sealed: bool,
    pub span: Span,
}

/// A grant within a domain.
#[derive(Debug, Clone, Serialize, Deserialize)]
pub enum GrantDecl {
    Events(String),
    State(String, PermMode),
    Assoc(String, PermMode),
    Clock,
    Accel(String),
}

/// Permission mode for state/assoc grants.
#[derive(Debug, Clone, Copy, PartialEq, Eq, Serialize, Deserialize)]
pub enum PermMode {
    Read,
    ReadWrite,
}

/// Lane declaration with policy and body.
#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct LaneDecl {
    pub name: String,
    pub domain: String,
    pub policy: LanePolicy,
    pub body: Vec<Stmt>,
    pub span: Span,
}

/// Lane scheduling policy.
#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct LanePolicy {
    pub priority: Option<PriorityLevel>,
    pub energy: Option<EnergyLevel>,
    pub affinity: Option<AffinityTarget>,
}

#[derive(Debug, Clone, Copy, Serialize, Deserialize)]
pub enum PriorityLevel { Low, Normal, High, Realtime }

#[derive(Debug, Clone, Copy, Serialize, Deserialize)]
pub enum EnergyLevel { Low, Balanced, Unbounded }

#[derive(Debug, Clone, Copy, Serialize, Deserialize)]
pub enum AffinityTarget { Any, Little, Big, Npu, Gpu, Dsp }

/// Statement in a lane body.
#[derive(Debug, Clone, Serialize, Deserialize)]
pub enum Stmt {
    /// `let x = <expr>`
    Let { name: String, value: Expr, span: Span },

    /// `<expr>` (expression statement)
    Expr(Expr),

    /// `loop { <body> }`
    Loop { body: Vec<Stmt>, span: Span },

    /// `if <cond> { <then> } else { <else> }`
    If { cond: Box<Expr>, then_body: Vec<Stmt>, else_body: Vec<Stmt>, span: Span },

    /// `return <expr>`
    Return { value: Option<Expr>, span: Span },
}

/// Expression.
#[derive(Debug, Clone, Serialize, Deserialize)]
pub enum Expr {
    /// String literal
    StringLit(String),

    /// Integer literal
    IntLit(i64),

    /// Float literal
    FloatLit(f64),

    /// Boolean literal
    BoolLit(bool),

    /// Duration literal (e.g., `1s`, `500ms`, `100us`)
    DurationLit { nanos: u64 },

    /// Variable reference
    Ident(String),

    /// Field access: `expr.field`
    Field { object: Box<Expr>, field: String },

    /// MSI call: `event.publish(topic, payload)`, `assoc.get(space, key)`, etc.
    MsiCall { module: String, method: String, args: Vec<Expr>, span: Span },

    /// Binary operation
    BinOp { op: BinOp, left: Box<Expr>, right: Box<Expr> },

    /// Unary operation
    UnaryOp { op: UnaryOp, operand: Box<Expr> },
}

#[derive(Debug, Clone, Copy, Serialize, Deserialize)]
pub enum BinOp {
    Add, Sub, Mul, Div, Mod,
    Eq, Neq, Lt, Gt, Lte, Gte,
    And, Or,
}

#[derive(Debug, Clone, Copy, Serialize, Deserialize)]
pub enum UnaryOp {
    Not, Neg,
}

/// Source location span.
#[derive(Debug, Clone, Copy, Default, Serialize, Deserialize)]
pub struct Span {
    pub line: usize,
    pub col: usize,
    pub len: usize,
}

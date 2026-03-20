//! SP Parser — builds AST from token stream
//!
//! Recursive descent parser for the Shinobi.Substrate language.

use crate::ast::*;
use crate::lexer::{Token, TokenKind};
use crate::error::{SpError, Result};

pub struct Parser {
    tokens: Vec<Token>,
    pos: usize,
}

impl Parser {
    pub fn new(tokens: Vec<Token>) -> Self {
        Parser { tokens, pos: 0 }
    }

    /// Parse a complete SP program.
    pub fn parse(&mut self) -> Result<Program> {
        let mut domains = Vec::new();
        let mut lanes = Vec::new();

        self.skip_newlines();

        while !self.at_eof() {
            match self.peek_kind() {
                TokenKind::Domain => domains.push(self.parse_domain()?),
                TokenKind::Lane => lanes.push(self.parse_lane()?),
                TokenKind::Newline => { self.advance(); }
                _ => {
                    let tok = self.peek();
                    return Err(SpError::Parse {
                        line: tok.line,
                        msg: format!("expected 'domain' or 'lane', got {:?}", tok.kind),
                    });
                }
            }
            self.skip_newlines();
        }

        Ok(Program { domains, lanes })
    }

    // ===== Domain parsing =====

    fn parse_domain(&mut self) -> Result<DomainDecl> {
        let span = self.span();
        self.expect(TokenKind::Domain)?;
        let name = self.expect_ident()?;
        self.expect(TokenKind::LBrace)?;
        self.skip_newlines();

        let mut grants = Vec::new();
        let mut sealed = false;

        while !self.check(TokenKind::RBrace) && !self.at_eof() {
            match self.peek_kind() {
                TokenKind::Grant => {
                    self.advance();
                    grants.push(self.parse_grant()?);
                }
                TokenKind::Sealed => {
                    self.advance();
                    sealed = true;
                }
                TokenKind::Newline => { self.advance(); }
                _ => {
                    let tok = self.peek();
                    return Err(SpError::Parse {
                        line: tok.line,
                        msg: format!("expected 'grant' or 'sealed', got {:?}", tok.kind),
                    });
                }
            }
            self.skip_newlines();
        }

        self.expect(TokenKind::RBrace)?;

        Ok(DomainDecl { name, grants, sealed, span })
    }

    fn parse_grant(&mut self) -> Result<GrantDecl> {
        match self.peek_kind() {
            TokenKind::Events => {
                self.advance();
                let prefix = self.expect_string()?;
                Ok(GrantDecl::Events(prefix))
            }
            TokenKind::State => {
                self.advance();
                let name = self.expect_string()?;
                let perm = self.parse_perm()?;
                Ok(GrantDecl::State(name, perm))
            }
            TokenKind::Assoc => {
                self.advance();
                let space = self.expect_string()?;
                let perm = self.parse_perm()?;
                Ok(GrantDecl::Assoc(space, perm))
            }
            TokenKind::Clock => {
                self.advance();
                Ok(GrantDecl::Clock)
            }
            TokenKind::Accel => {
                self.advance();
                let which = self.expect_string()?;
                Ok(GrantDecl::Accel(which))
            }
            _ => {
                let tok = self.peek();
                Err(SpError::Parse {
                    line: tok.line,
                    msg: format!("expected grant type (events/state/assoc/clock/accel), got {:?}", tok.kind),
                })
            }
        }
    }

    fn parse_perm(&mut self) -> Result<PermMode> {
        match self.peek_kind() {
            TokenKind::Rw => { self.advance(); Ok(PermMode::ReadWrite) }
            TokenKind::Ro => { self.advance(); Ok(PermMode::Read) }
            _ => Ok(PermMode::Read) // default
        }
    }

    // ===== Lane parsing =====

    fn parse_lane(&mut self) -> Result<LaneDecl> {
        let span = self.span();
        self.expect(TokenKind::Lane)?;
        let name = self.expect_ident()?;
        self.expect(TokenKind::In)?;
        let domain = self.expect_ident()?;
        self.expect(TokenKind::LBrace)?;
        self.skip_newlines();

        let mut policy = LanePolicy {
            priority: None,
            energy: None,
            affinity: None,
        };

        // Parse policy directives at the top of the lane body
        loop {
            match self.peek_kind() {
                TokenKind::Priority => {
                    self.advance();
                    policy.priority = Some(self.parse_priority()?);
                }
                TokenKind::Energy => {
                    self.advance();
                    policy.energy = Some(self.parse_energy()?);
                }
                TokenKind::Affinity => {
                    self.advance();
                    policy.affinity = Some(self.parse_affinity()?);
                }
                TokenKind::Newline => { self.advance(); continue; }
                _ => break,
            }
            self.skip_newlines();
        }

        // Parse body statements
        let body = self.parse_block_body()?;
        self.expect(TokenKind::RBrace)?;

        Ok(LaneDecl { name, domain, policy, body, span })
    }

    fn parse_priority(&mut self) -> Result<PriorityLevel> {
        match self.peek_kind() {
            TokenKind::Low => { self.advance(); Ok(PriorityLevel::Low) }
            TokenKind::Normal => { self.advance(); Ok(PriorityLevel::Normal) }
            TokenKind::High => { self.advance(); Ok(PriorityLevel::High) }
            TokenKind::Realtime => { self.advance(); Ok(PriorityLevel::Realtime) }
            _ => {
                let tok = self.peek();
                Err(SpError::Parse {
                    line: tok.line,
                    msg: format!("expected priority level, got {:?}", tok.kind),
                })
            }
        }
    }

    fn parse_energy(&mut self) -> Result<EnergyLevel> {
        match self.peek_kind() {
            TokenKind::Low => { self.advance(); Ok(EnergyLevel::Low) }
            TokenKind::Balanced => { self.advance(); Ok(EnergyLevel::Balanced) }
            TokenKind::Unbounded => { self.advance(); Ok(EnergyLevel::Unbounded) }
            _ => {
                let tok = self.peek();
                Err(SpError::Parse {
                    line: tok.line,
                    msg: format!("expected energy level, got {:?}", tok.kind),
                })
            }
        }
    }

    fn parse_affinity(&mut self) -> Result<AffinityTarget> {
        match self.peek_kind() {
            TokenKind::Any => { self.advance(); Ok(AffinityTarget::Any) }
            TokenKind::Little => { self.advance(); Ok(AffinityTarget::Little) }
            TokenKind::Big => { self.advance(); Ok(AffinityTarget::Big) }
            TokenKind::Npu => { self.advance(); Ok(AffinityTarget::Npu) }
            TokenKind::Gpu => { self.advance(); Ok(AffinityTarget::Gpu) }
            TokenKind::Dsp => { self.advance(); Ok(AffinityTarget::Dsp) }
            _ => {
                let tok = self.peek();
                Err(SpError::Parse {
                    line: tok.line,
                    msg: format!("expected affinity target, got {:?}", tok.kind),
                })
            }
        }
    }

    // ===== Statement parsing =====

    fn parse_block_body(&mut self) -> Result<Vec<Stmt>> {
        let mut stmts = Vec::new();
        self.skip_newlines();

        while !self.check(TokenKind::RBrace) && !self.at_eof() {
            stmts.push(self.parse_stmt()?);
            self.skip_newlines();
        }

        Ok(stmts)
    }

    fn parse_stmt(&mut self) -> Result<Stmt> {
        match self.peek_kind() {
            TokenKind::Let => self.parse_let(),
            TokenKind::Loop => self.parse_loop(),
            TokenKind::If => self.parse_if(),
            TokenKind::Return => self.parse_return(),
            _ => {
                let expr = self.parse_expr()?;
                Ok(Stmt::Expr(expr))
            }
        }
    }

    fn parse_let(&mut self) -> Result<Stmt> {
        let span = self.span();
        self.expect(TokenKind::Let)?;
        let name = self.expect_ident()?;
        self.expect(TokenKind::Eq)?;
        let value = self.parse_expr()?;
        Ok(Stmt::Let { name, value, span })
    }

    fn parse_loop(&mut self) -> Result<Stmt> {
        let span = self.span();
        self.expect(TokenKind::Loop)?;
        self.expect(TokenKind::LBrace)?;
        let body = self.parse_block_body()?;
        self.expect(TokenKind::RBrace)?;
        Ok(Stmt::Loop { body, span })
    }

    fn parse_if(&mut self) -> Result<Stmt> {
        let span = self.span();
        self.expect(TokenKind::If)?;
        let cond = Box::new(self.parse_expr()?);
        self.expect(TokenKind::LBrace)?;
        let then_body = self.parse_block_body()?;
        self.expect(TokenKind::RBrace)?;

        let else_body = if self.check(TokenKind::Else) {
            self.advance();
            self.expect(TokenKind::LBrace)?;
            let body = self.parse_block_body()?;
            self.expect(TokenKind::RBrace)?;
            body
        } else {
            Vec::new()
        };

        Ok(Stmt::If { cond, then_body, else_body, span })
    }

    fn parse_return(&mut self) -> Result<Stmt> {
        let span = self.span();
        self.expect(TokenKind::Return)?;
        let value = if self.check(TokenKind::Newline) || self.check(TokenKind::RBrace) || self.at_eof() {
            None
        } else {
            Some(self.parse_expr()?)
        };
        Ok(Stmt::Return { value, span })
    }

    // ===== Expression parsing =====

    fn parse_expr(&mut self) -> Result<Expr> {
        self.parse_or()
    }

    fn parse_or(&mut self) -> Result<Expr> {
        let mut left = self.parse_and()?;
        while self.check(TokenKind::PipePipe) {
            self.advance();
            let right = self.parse_and()?;
            left = Expr::BinOp { op: BinOp::Or, left: Box::new(left), right: Box::new(right) };
        }
        Ok(left)
    }

    fn parse_and(&mut self) -> Result<Expr> {
        let mut left = self.parse_comparison()?;
        while self.check(TokenKind::AmpAmp) {
            self.advance();
            let right = self.parse_comparison()?;
            left = Expr::BinOp { op: BinOp::And, left: Box::new(left), right: Box::new(right) };
        }
        Ok(left)
    }

    fn parse_comparison(&mut self) -> Result<Expr> {
        let mut left = self.parse_additive()?;
        loop {
            let op = match self.peek_kind() {
                TokenKind::EqEq => BinOp::Eq,
                TokenKind::BangEq => BinOp::Neq,
                TokenKind::Lt => BinOp::Lt,
                TokenKind::Gt => BinOp::Gt,
                TokenKind::LtEq => BinOp::Lte,
                TokenKind::GtEq => BinOp::Gte,
                _ => break,
            };
            self.advance();
            let right = self.parse_additive()?;
            left = Expr::BinOp { op, left: Box::new(left), right: Box::new(right) };
        }
        Ok(left)
    }

    fn parse_additive(&mut self) -> Result<Expr> {
        let mut left = self.parse_multiplicative()?;
        loop {
            let op = match self.peek_kind() {
                TokenKind::Plus => BinOp::Add,
                TokenKind::Minus => BinOp::Sub,
                _ => break,
            };
            self.advance();
            let right = self.parse_multiplicative()?;
            left = Expr::BinOp { op, left: Box::new(left), right: Box::new(right) };
        }
        Ok(left)
    }

    fn parse_multiplicative(&mut self) -> Result<Expr> {
        let mut left = self.parse_unary()?;
        loop {
            let op = match self.peek_kind() {
                TokenKind::Star => BinOp::Mul,
                TokenKind::Slash => BinOp::Div,
                TokenKind::Percent => BinOp::Mod,
                _ => break,
            };
            self.advance();
            let right = self.parse_unary()?;
            left = Expr::BinOp { op, left: Box::new(left), right: Box::new(right) };
        }
        Ok(left)
    }

    fn parse_unary(&mut self) -> Result<Expr> {
        match self.peek_kind() {
            TokenKind::Bang => {
                self.advance();
                let operand = self.parse_unary()?;
                Ok(Expr::UnaryOp { op: UnaryOp::Not, operand: Box::new(operand) })
            }
            TokenKind::Minus => {
                self.advance();
                let operand = self.parse_unary()?;
                Ok(Expr::UnaryOp { op: UnaryOp::Neg, operand: Box::new(operand) })
            }
            _ => self.parse_postfix(),
        }
    }

    fn parse_postfix(&mut self) -> Result<Expr> {
        let mut expr = self.parse_primary()?;

        loop {
            if self.check(TokenKind::Dot) {
                self.advance();
                let field = self.expect_ident()?;

                // Check if this is a method call: expr.method(args)
                if self.check(TokenKind::LParen) {
                    self.advance();
                    let args = self.parse_arg_list()?;
                    self.expect(TokenKind::RParen)?;

                    // Extract module name from the expression
                    let module = match &expr {
                        Expr::Ident(name) => name.clone(),
                        _ => "?".to_string(),
                    };

                    expr = Expr::MsiCall {
                        module,
                        method: field,
                        args,
                        span: self.span(),
                    };
                } else {
                    expr = Expr::Field {
                        object: Box::new(expr),
                        field,
                    };
                }
            } else {
                break;
            }
        }

        Ok(expr)
    }

    fn parse_primary(&mut self) -> Result<Expr> {
        let tok = self.peek().clone();
        match tok.kind {
            TokenKind::StringLit(ref s) => {
                let val = s.clone();
                self.advance();
                Ok(Expr::StringLit(val))
            }
            TokenKind::IntLit(n) => {
                self.advance();
                Ok(Expr::IntLit(n))
            }
            TokenKind::FloatLit(f) => {
                self.advance();
                Ok(Expr::FloatLit(f))
            }
            TokenKind::DurationLit(ns) => {
                self.advance();
                Ok(Expr::DurationLit { nanos: ns })
            }
            TokenKind::BoolTrue => {
                self.advance();
                Ok(Expr::BoolLit(true))
            }
            TokenKind::BoolFalse => {
                self.advance();
                Ok(Expr::BoolLit(false))
            }
            TokenKind::Ident(ref name) => {
                let name = name.clone();
                self.advance();

                // Check for function call: ident(args)
                if self.check(TokenKind::LParen) {
                    self.advance();
                    let args = self.parse_arg_list()?;
                    self.expect(TokenKind::RParen)?;
                    Ok(Expr::MsiCall {
                        module: String::new(),
                        method: name,
                        args,
                        span: self.span(),
                    })
                } else {
                    Ok(Expr::Ident(name))
                }
            }
            TokenKind::LParen => {
                self.advance();
                let expr = self.parse_expr()?;
                self.expect(TokenKind::RParen)?;
                Ok(expr)
            }
            _ => Err(SpError::Parse {
                line: tok.line,
                msg: format!("expected expression, got {:?}", tok.kind),
            }),
        }
    }

    fn parse_arg_list(&mut self) -> Result<Vec<Expr>> {
        let mut args = Vec::new();
        if !self.check(TokenKind::RParen) {
            args.push(self.parse_expr()?);
            while self.check(TokenKind::Comma) {
                self.advance();
                args.push(self.parse_expr()?);
            }
        }
        Ok(args)
    }

    // ===== Token utilities =====

    fn peek(&self) -> &Token {
        self.tokens.get(self.pos).unwrap_or(&Token {
            kind: TokenKind::Eof,
            line: 0,
            col: 0,
        })
    }

    fn peek_kind(&self) -> TokenKind {
        self.peek().kind.clone()
    }

    fn advance(&mut self) -> &Token {
        let tok = &self.tokens[self.pos.min(self.tokens.len() - 1)];
        if self.pos < self.tokens.len() {
            self.pos += 1;
        }
        tok
    }

    fn check(&self, kind: TokenKind) -> bool {
        std::mem::discriminant(&self.peek_kind()) == std::mem::discriminant(&kind)
    }

    fn at_eof(&self) -> bool {
        self.pos >= self.tokens.len() || matches!(self.peek_kind(), TokenKind::Eof)
    }

    fn expect(&mut self, kind: TokenKind) -> Result<&Token> {
        if self.check(kind.clone()) {
            Ok(self.advance())
        } else {
            let tok = self.peek();
            Err(SpError::Parse {
                line: tok.line,
                msg: format!("expected {:?}, got {:?}", kind, tok.kind),
            })
        }
    }

    fn expect_ident(&mut self) -> Result<String> {
        match self.peek_kind() {
            TokenKind::Ident(name) => {
                let name = name.clone();
                self.advance();
                Ok(name)
            }
            _ => {
                let tok = self.peek();
                Err(SpError::Parse {
                    line: tok.line,
                    msg: format!("expected identifier, got {:?}", tok.kind),
                })
            }
        }
    }

    fn expect_string(&mut self) -> Result<String> {
        match self.peek_kind() {
            TokenKind::StringLit(s) => {
                let s = s.clone();
                self.advance();
                Ok(s)
            }
            _ => {
                let tok = self.peek();
                Err(SpError::Parse {
                    line: tok.line,
                    msg: format!("expected string literal, got {:?}", tok.kind),
                })
            }
        }
    }

    fn skip_newlines(&mut self) {
        while self.check(TokenKind::Newline) {
            self.advance();
        }
    }

    fn span(&self) -> Span {
        let tok = self.peek();
        Span { line: tok.line, col: tok.col, len: 0 }
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::lexer::Lexer;

    #[test]
    fn test_parse_domain() {
        let src = r#"
domain TestDomain {
    grant events "phone/"
    grant assoc "working" rw
    grant clock
    sealed
}
"#;
        let tokens = Lexer::new(src).tokenize().unwrap();
        let program = Parser::new(tokens).parse().unwrap();
        assert_eq!(program.domains.len(), 1);
        assert_eq!(program.domains[0].name, "TestDomain");
        assert_eq!(program.domains[0].grants.len(), 3);
        assert!(program.domains[0].sealed);
    }

    #[test]
    fn test_parse_lane() {
        let src = r#"
domain D { sealed }

lane main in D {
    priority high
    energy balanced
    affinity big

    let x = 42
    event.publish("test", "hello")
}
"#;
        let tokens = Lexer::new(src).tokenize().unwrap();
        let program = Parser::new(tokens).parse().unwrap();
        assert_eq!(program.lanes.len(), 1);
        assert_eq!(program.lanes[0].name, "main");
        assert_eq!(program.lanes[0].domain, "D");
        assert!(matches!(program.lanes[0].policy.priority, Some(PriorityLevel::High)));
        assert_eq!(program.lanes[0].body.len(), 2);
    }
}

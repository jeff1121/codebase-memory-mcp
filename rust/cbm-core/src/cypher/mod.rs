//! `src/cypher/cypher.c` lexer 的 test-only parity helper。
//!
//! 此模組固定 token kind、byte offset、字串 escape、註解/空白略過規則，
//! 以及一小段 normalized AST summary；不替換 production parser / executor。

const TOK_MATCH: i32 = 0;
const TOK_WHERE: i32 = 1;
const TOK_RETURN: i32 = 2;
const TOK_ORDER: i32 = 3;
const TOK_BY: i32 = 4;
const TOK_LIMIT: i32 = 5;
const TOK_AND: i32 = 6;
const TOK_OR: i32 = 7;
const TOK_AS: i32 = 8;
const TOK_DISTINCT: i32 = 9;
const TOK_COUNT: i32 = 10;
const TOK_CONTAINS: i32 = 11;
const TOK_STARTS: i32 = 12;
const TOK_WITH: i32 = 13;
const TOK_NOT: i32 = 14;
const TOK_ASC: i32 = 15;
const TOK_DESC: i32 = 16;
const TOK_NEQ: i32 = 17;
const TOK_ENDS: i32 = 18;
const TOK_IN: i32 = 19;
const TOK_IS: i32 = 20;
const TOK_NULL_KW: i32 = 21;
const TOK_XOR: i32 = 22;
const TOK_SKIP: i32 = 23;
const TOK_UNION: i32 = 24;
const TOK_UNWIND: i32 = 25;
const TOK_SUM: i32 = 26;
const TOK_AVG: i32 = 27;
const TOK_MIN_KW: i32 = 28;
const TOK_MAX_KW: i32 = 29;
const TOK_COLLECT: i32 = 30;
const TOK_TOLOWER: i32 = 31;
const TOK_TOUPPER: i32 = 32;
const TOK_TOSTRING: i32 = 33;
const TOK_CASE: i32 = 34;
const TOK_WHEN: i32 = 35;
const TOK_THEN: i32 = 36;
const TOK_ELSE: i32 = 37;
const TOK_END: i32 = 38;
const TOK_CREATE: i32 = 39;
const TOK_DELETE: i32 = 40;
const TOK_DETACH: i32 = 41;
const TOK_SET: i32 = 42;
const TOK_REMOVE: i32 = 43;
const TOK_MERGE: i32 = 44;
const TOK_OPTIONAL: i32 = 45;
const TOK_YIELD: i32 = 46;
const TOK_CALL: i32 = 47;
const TOK_ALL: i32 = 48;
const TOK_TRUE: i32 = 49;
const TOK_FALSE: i32 = 50;
const TOK_EXISTS: i32 = 51;
const TOK_MANDATORY: i32 = 52;
const TOK_FOREACH: i32 = 53;
const TOK_ON: i32 = 54;
const TOK_ADD: i32 = 55;
const TOK_CONSTRAINT: i32 = 56;
const TOK_DO: i32 = 57;
const TOK_DROP: i32 = 58;
const TOK_FOR: i32 = 59;
const TOK_FROM: i32 = 60;
const TOK_GRAPH: i32 = 61;
const TOK_OF: i32 = 62;
const TOK_REQUIRE: i32 = 63;
const TOK_SCALAR: i32 = 64;
const TOK_UNIQUE: i32 = 65;
const TOK_LPAREN: i32 = 66;
const TOK_RPAREN: i32 = 67;
const TOK_LBRACKET: i32 = 68;
const TOK_RBRACKET: i32 = 69;
const TOK_DASH: i32 = 70;
const TOK_GT: i32 = 71;
const TOK_LT: i32 = 72;
const TOK_COLON: i32 = 73;
const TOK_DOT: i32 = 74;
const TOK_LBRACE: i32 = 75;
const TOK_RBRACE: i32 = 76;
const TOK_STAR: i32 = 77;
const TOK_COMMA: i32 = 78;
const TOK_EQ: i32 = 79;
const TOK_EQTILDE: i32 = 80;
const TOK_GTE: i32 = 81;
const TOK_LTE: i32 = 82;
const TOK_PIPE: i32 = 83;
const TOK_DOTDOT: i32 = 84;
const TOK_IDENT: i32 = 85;
const TOK_STRING: i32 = 86;
const TOK_NUMBER: i32 = 87;
const TOK_EOF: i32 = 88;

const STRING_BUF_MAX: usize = 4095;

#[derive(Debug, Clone, PartialEq, Eq)]
pub struct Token {
    pub kind: i32,
    pub pos: i32,
    pub text: Vec<u8>,
}

const SCALAR_FUNC_NAMES: [&[u8]; 14] = [
    b"labels",
    b"type",
    b"id",
    b"keys",
    b"properties",
    b"toInteger",
    b"toFloat",
    b"toBoolean",
    b"size",
    b"length",
    b"trim",
    b"ltrim",
    b"rtrim",
    b"reverse",
];

const MULTIARG_FUNC_NAMES: [&[u8]; 5] = [b"coalesce", b"substring", b"replace", b"left", b"right"];
/// 對齊 `src/cypher/cypher.c` `scalar_func_canonical`：以 ASCII 大小寫不敏感
/// （對齊 `cyp_ci_eq`）比對 `input`，回傳符合的 canonical 名稱索引（順序需與 C
/// `names[]` 一致），或 `None`。索引由 C 端映射回其自身的 static 名稱字串，因此
/// 保留原本回傳 static 指標的語意。
#[must_use]
pub fn scalar_func_index(input: Option<&[u8]>) -> Option<usize> {
    let input = input?;
    SCALAR_FUNC_NAMES
        .iter()
        .position(|name| input.eq_ignore_ascii_case(name))
}

/// 對齊 `src/cypher/cypher.c` `multiarg_func_canonical`。Rust 只回傳
/// C `names[]` 的索引，C 仍回傳自己的 static 名稱字串並執行既有 evaluator。
#[must_use]
pub fn multiarg_func_index(input: Option<&[u8]>) -> Option<usize> {
    let input = input?;
    MULTIARG_FUNC_NAMES
        .iter()
        .position(|name| input.eq_ignore_ascii_case(name))
}

/// 對齊 `src/cypher/cypher.c` `agg_func_name` 的 token -> aggregate 名稱 mapping。
/// Rust 只回傳 C 本地 `names[]` 的索引；parser item 建立與 aggregation executor 仍由 C 執行。
#[must_use]
pub fn aggregate_func_index(token_kind: i32) -> Option<usize> {
    match token_kind {
        TOK_COUNT => Some(0),
        TOK_SUM => Some(1),
        TOK_AVG => Some(2),
        TOK_MIN_KW => Some(3),
        TOK_MAX_KW => Some(4),
        TOK_COLLECT => Some(5),
        _ => None,
    }
}

/// 對齊 `src/cypher/cypher.c` `str_func_name` 的 token -> string function 名稱 mapping。
/// Rust 只回傳 C 本地 `names[]` 的索引；parser item 建立與 scalar evaluator 仍由 C 執行。
#[must_use]
pub fn string_func_index(token_kind: i32) -> Option<usize> {
    match token_kind {
        TOK_TOLOWER => Some(0),
        TOK_TOUPPER => Some(1),
        TOK_TOSTRING => Some(2),
        _ => None,
    }
}

#[must_use]
pub fn lex(input: &[u8]) -> Vec<Token> {
    let mut tokens = Vec::new();
    let mut i = 0usize;
    while i < input.len() {
        if skip_whitespace_comments(input, &mut i) {
            continue;
        }

        let c = input[i];
        if c == b'"' || c == b'\'' {
            let quote = c;
            i += 1;
            tokens.push(lex_string_literal(input, &mut i, quote));
            continue;
        }
        if let Some(token) = try_number(input, &mut i) {
            tokens.push(token);
            continue;
        }
        if let Some(token) = try_ident(input, &mut i) {
            tokens.push(token);
            continue;
        }
        if let Some(token) = try_two_char(input, &mut i) {
            tokens.push(token);
            continue;
        }
        if let Some(kind) = single_char_kind(c) {
            tokens.push(Token {
                kind,
                pos: i as i32,
                text: vec![c],
            });
            i += 1;
            continue;
        }
        i += 1;
    }

    tokens.push(Token {
        kind: TOK_EOF,
        pos: i as i32,
        text: Vec::new(),
    });
    tokens
}

#[must_use]
pub fn summary(input: &[u8]) -> Vec<u8> {
    let tokens = lex(input);
    let mut out = Vec::new();
    for (idx, token) in tokens.iter().enumerate() {
        if idx > 0 {
            out.push(b'|');
        }
        out.extend_from_slice(token_name(token.kind).as_bytes());
        out.push(b'@');
        out.extend_from_slice(token.pos.to_string().as_bytes());
        out.push(b':');
        push_escaped(&mut out, &token.text);
    }
    out
}

#[must_use]
pub fn parse_summary(input: &[u8]) -> Option<Vec<u8>> {
    let tokens = lex(input);
    let mut parser = Parser::new(tokens);
    let query = parser.parse_union_query()?;
    if !parser.at(TOK_EOF) {
        return None;
    }
    let mut out = Vec::new();
    render_query(&query, &mut out);
    Some(out)
}

#[derive(Debug, Clone, PartialEq, Eq)]
struct QuerySummary {
    patterns: Vec<PatternSummary>,
    where_expr: Option<ExprSummary>,
    with_items: Vec<ProjectionItem>,
    with_distinct: bool,
    post_with_where: Option<ExprSummary>,
    return_items: Vec<ProjectionItem>,
    return_distinct: bool,
    union_next: Option<Box<QuerySummary>>,
    union_all: bool,
}

impl QuerySummary {
    fn new() -> Self {
        Self {
            patterns: Vec::new(),
            where_expr: None,
            with_items: Vec::new(),
            with_distinct: false,
            post_with_where: None,
            return_items: Vec::new(),
            return_distinct: false,
            union_next: None,
            union_all: false,
        }
    }
}

#[derive(Debug, Clone, PartialEq, Eq)]
struct PatternSummary {
    optional: bool,
    nodes: Vec<NodeSummary>,
    rels: Vec<RelSummary>,
}

#[derive(Debug, Clone, PartialEq, Eq)]
struct NodeSummary {
    variable: Vec<u8>,
    label: Vec<u8>,
}

#[derive(Debug, Clone, PartialEq, Eq)]
struct RelSummary {
    variable: Vec<u8>,
    types: Vec<Vec<u8>>,
    direction: DirectionSummary,
    min_hops: i32,
    max_hops: i32,
}

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
enum DirectionSummary {
    Outbound,
    Inbound,
    Any,
}

#[derive(Debug, Clone, PartialEq, Eq)]
struct ProjectionItem {
    func: Option<&'static str>,
    distinct: bool,
    expr: Vec<u8>,
    alias: Option<Vec<u8>>,
}

#[derive(Debug, Clone, PartialEq, Eq)]
enum ExprSummary {
    Condition(ConditionSummary),
    And(Box<ExprSummary>, Box<ExprSummary>),
    Or(Box<ExprSummary>, Box<ExprSummary>),
    Not(Box<ExprSummary>),
}

#[derive(Debug, Clone, PartialEq, Eq)]
struct ConditionSummary {
    variable: Vec<u8>,
    property: Option<Vec<u8>>,
    op: &'static str,
    value: Vec<u8>,
    exists_dir: Option<DirectionSummary>,
}

struct Parser {
    tokens: Vec<Token>,
    pos: usize,
}

impl Parser {
    fn new(tokens: Vec<Token>) -> Self {
        Self { tokens, pos: 0 }
    }

    fn parse_union_query(&mut self) -> Option<QuerySummary> {
        let mut query = self.parse_query_part()?;
        if self.eat(TOK_UNION) {
            query.union_all = self.eat(TOK_ALL);
            query.union_next = Some(Box::new(self.parse_union_query()?));
        }
        Some(query)
    }

    fn parse_query_part(&mut self) -> Option<QuerySummary> {
        let mut query = QuerySummary::new();
        loop {
            if self.at(TOK_MATCH) || self.at(TOK_OPTIONAL) {
                query.patterns.push(self.parse_pattern_clause()?);
            } else if self.eat(TOK_WHERE) {
                query.where_expr =
                    Some(self.parse_expr_until(&[TOK_WITH, TOK_RETURN, TOK_UNION, TOK_EOF])?);
            } else if self.eat(TOK_WITH) {
                query.with_distinct = self.eat(TOK_DISTINCT);
                query.with_items =
                    self.parse_projection_items(&[TOK_WHERE, TOK_RETURN, TOK_UNION, TOK_EOF])?;
                if self.eat(TOK_WHERE) {
                    query.post_with_where =
                        Some(self.parse_expr_until(&[TOK_RETURN, TOK_UNION, TOK_EOF])?);
                }
            } else if self.eat(TOK_RETURN) {
                query.return_distinct = self.eat(TOK_DISTINCT);
                query.return_items = self.parse_projection_items(&[
                    TOK_ORDER, TOK_SKIP, TOK_LIMIT, TOK_UNION, TOK_EOF,
                ])?;
                self.skip_return_tail();
            } else if self.at(TOK_UNION) || self.at(TOK_EOF) {
                break;
            } else {
                return None;
            }
        }
        if query.patterns.is_empty() && query.return_items.is_empty() {
            return None;
        }
        Some(query)
    }

    fn parse_pattern_clause(&mut self) -> Option<PatternSummary> {
        let optional = self.eat(TOK_OPTIONAL);
        self.expect(TOK_MATCH)?;
        let mut nodes = Vec::new();
        let mut rels = Vec::new();
        nodes.push(self.parse_node()?);
        while self.starts_relationship() {
            rels.push(self.parse_relationship()?);
            nodes.push(self.parse_node()?);
        }
        Some(PatternSummary {
            optional,
            nodes,
            rels,
        })
    }

    fn parse_node(&mut self) -> Option<NodeSummary> {
        self.expect(TOK_LPAREN)?;
        let mut variable = Vec::new();
        let mut label = Vec::new();
        if self.at_name() {
            variable = self.take_name()?;
        }
        if self.eat(TOK_COLON) {
            label = self.parse_pipe_joined_names()?;
        }
        if self.eat(TOK_LBRACE) {
            self.skip_balanced(TOK_LBRACE, TOK_RBRACE)?;
        }
        self.expect(TOK_RPAREN)?;
        Some(NodeSummary { variable, label })
    }

    fn parse_relationship(&mut self) -> Option<RelSummary> {
        let inbound_prefix = if self.eat(TOK_LT) {
            self.expect(TOK_DASH)?;
            true
        } else {
            self.expect(TOK_DASH)?;
            false
        };
        let mut rel = RelSummary {
            variable: Vec::new(),
            types: Vec::new(),
            direction: DirectionSummary::Any,
            min_hops: 1,
            max_hops: 1,
        };
        if self.eat(TOK_LBRACKET) {
            if self.at_name() && matches!(self.peek_kind(), TOK_COLON | TOK_RBRACKET) {
                rel.variable = self.take_name()?;
            }
            if self.eat(TOK_COLON) {
                rel.types = self.parse_name_list()?;
            }
            if self.eat(TOK_STAR) {
                (rel.min_hops, rel.max_hops) = self.parse_hops();
            }
            self.expect(TOK_RBRACKET)?;
        }
        self.expect(TOK_DASH)?;
        rel.direction = if inbound_prefix {
            DirectionSummary::Inbound
        } else if self.eat(TOK_GT) {
            DirectionSummary::Outbound
        } else {
            DirectionSummary::Any
        };
        Some(rel)
    }

    fn parse_projection_items(&mut self, stops: &[i32]) -> Option<Vec<ProjectionItem>> {
        let mut items = Vec::new();
        if self.at_stop(stops) {
            return Some(items);
        }
        loop {
            items.push(self.parse_projection_item()?);
            if !self.eat(TOK_COMMA) {
                break;
            }
            if self.at_stop(stops) {
                return None;
            }
        }
        Some(items)
    }

    fn parse_projection_item(&mut self) -> Option<ProjectionItem> {
        if let Some(func) = self.projection_func() {
            self.pos += 1;
            self.expect(TOK_LPAREN)?;
            let distinct = self.eat(TOK_DISTINCT);
            let expr = if self.eat(TOK_STAR) {
                b"*".to_vec()
            } else {
                self.parse_ref()?
            };
            self.expect(TOK_RPAREN)?;
            let alias = self.parse_alias();
            return Some(ProjectionItem {
                func: Some(func),
                distinct,
                expr,
                alias,
            });
        }
        let expr = self.parse_ref()?;
        let alias = self.parse_alias();
        Some(ProjectionItem {
            func: None,
            distinct: false,
            expr,
            alias,
        })
    }

    fn parse_alias(&mut self) -> Option<Vec<u8>> {
        if self.eat(TOK_AS) {
            return self.take_name();
        }
        None
    }

    fn parse_expr_until(&mut self, stops: &[i32]) -> Option<ExprSummary> {
        self.parse_or(stops)
    }

    fn parse_or(&mut self, stops: &[i32]) -> Option<ExprSummary> {
        let mut expr = self.parse_and(stops)?;
        while !self.at_stop(stops) && self.eat(TOK_OR) {
            let right = self.parse_and(stops)?;
            expr = ExprSummary::Or(Box::new(expr), Box::new(right));
        }
        Some(expr)
    }

    fn parse_and(&mut self, stops: &[i32]) -> Option<ExprSummary> {
        let mut expr = self.parse_unary()?;
        while !self.at_stop(stops) && self.eat(TOK_AND) {
            let right = self.parse_unary()?;
            expr = ExprSummary::And(Box::new(expr), Box::new(right));
        }
        Some(expr)
    }

    fn parse_unary(&mut self) -> Option<ExprSummary> {
        if self.eat(TOK_NOT) {
            return Some(ExprSummary::Not(Box::new(self.parse_unary()?)));
        }
        if self.eat(TOK_LPAREN) {
            let expr = self.parse_or(&[TOK_RPAREN])?;
            self.expect(TOK_RPAREN)?;
            return Some(expr);
        }
        self.parse_condition()
    }

    fn parse_condition(&mut self) -> Option<ExprSummary> {
        if self.eat(TOK_EXISTS) {
            return self.parse_exists_condition();
        }
        let variable = self.take_name()?;
        let property = if self.eat(TOK_DOT) {
            Some(self.take_name()?)
        } else {
            None
        };
        let op = self.parse_operator()?;
        let value = if matches!(op, "IS NULL" | "IS NOT NULL") {
            b"NULL".to_vec()
        } else {
            self.take_value()?
        };
        Some(ExprSummary::Condition(ConditionSummary {
            variable,
            property,
            op,
            value,
            exists_dir: None,
        }))
    }

    fn parse_exists_condition(&mut self) -> Option<ExprSummary> {
        self.expect(TOK_LBRACE)?;
        let anchor = self.parse_node()?;
        let rel = self.parse_relationship()?;
        let _target = self.parse_node()?;
        self.expect(TOK_RBRACE)?;
        Some(ExprSummary::Condition(ConditionSummary {
            variable: anchor.variable,
            property: None,
            op: "EXISTS",
            value: rel.types.first().cloned().unwrap_or_default(),
            exists_dir: Some(rel.direction),
        }))
    }

    fn parse_operator(&mut self) -> Option<&'static str> {
        if self.eat(TOK_EQ) {
            Some("=")
        } else if self.eat(TOK_NEQ) {
            Some("<>")
        } else if self.eat(TOK_EQTILDE) {
            Some("=~")
        } else if self.eat(TOK_GT) {
            Some(">")
        } else if self.eat(TOK_LT) {
            Some("<")
        } else if self.eat(TOK_GTE) {
            Some(">=")
        } else if self.eat(TOK_LTE) {
            Some("<=")
        } else if self.eat(TOK_CONTAINS) {
            Some("CONTAINS")
        } else if self.eat(TOK_STARTS) {
            self.expect(TOK_WITH)?;
            Some("STARTS WITH")
        } else if self.eat(TOK_ENDS) {
            self.expect(TOK_WITH)?;
            Some("ENDS WITH")
        } else if self.eat(TOK_IS) {
            if self.eat(TOK_NOT) {
                self.expect(TOK_NULL_KW)?;
                Some("IS NOT NULL")
            } else {
                self.expect(TOK_NULL_KW)?;
                Some("IS NULL")
            }
        } else {
            None
        }
    }

    fn parse_ref(&mut self) -> Option<Vec<u8>> {
        let mut out = self.take_name()?;
        if self.eat(TOK_DOT) {
            out.push(b'.');
            out.extend_from_slice(&self.take_name()?);
        }
        Some(out)
    }

    fn parse_pipe_joined_names(&mut self) -> Option<Vec<u8>> {
        let mut out = self.take_name()?;
        while self.eat(TOK_PIPE) {
            out.push(b'|');
            out.extend_from_slice(&self.take_name()?);
        }
        Some(out)
    }

    fn parse_name_list(&mut self) -> Option<Vec<Vec<u8>>> {
        let mut names = vec![self.take_name()?];
        while self.eat(TOK_PIPE) {
            names.push(self.take_name()?);
        }
        Some(names)
    }

    fn parse_hops(&mut self) -> (i32, i32) {
        if self.at(TOK_NUMBER) {
            let min = self.take_i32().unwrap_or(1);
            if self.eat(TOK_DOTDOT) {
                let max = if self.at(TOK_NUMBER) {
                    self.take_i32().unwrap_or(0)
                } else {
                    0
                };
                (min, max)
            } else {
                (1, min)
            }
        } else if self.eat(TOK_DOTDOT) {
            let max = if self.at(TOK_NUMBER) {
                self.take_i32().unwrap_or(0)
            } else {
                0
            };
            (1, max)
        } else {
            (1, 0)
        }
    }

    fn skip_balanced(&mut self, open: i32, close: i32) -> Option<()> {
        let mut depth = 1;
        while depth > 0 {
            if self.at(TOK_EOF) {
                return None;
            }
            if self.eat(open) {
                depth += 1;
            } else if self.eat(close) {
                depth -= 1;
            } else {
                self.pos += 1;
            }
        }
        Some(())
    }

    fn skip_return_tail(&mut self) {
        while !self.at(TOK_UNION) && !self.at(TOK_EOF) {
            self.pos += 1;
        }
    }

    fn take_name(&mut self) -> Option<Vec<u8>> {
        if !self.at_name() {
            return None;
        }
        let text = self.current().text.clone();
        self.pos += 1;
        Some(text)
    }

    fn take_value(&mut self) -> Option<Vec<u8>> {
        match self.kind() {
            TOK_STRING | TOK_NUMBER | TOK_IDENT | TOK_TRUE | TOK_FALSE | TOK_NULL_KW => {
                let text = self.current().text.clone();
                self.pos += 1;
                Some(text)
            }
            _ => None,
        }
    }

    fn take_i32(&mut self) -> Option<i32> {
        let text = self.current().text.clone();
        self.pos += 1;
        std::str::from_utf8(&text).ok()?.parse::<i32>().ok()
    }

    fn projection_func(&self) -> Option<&'static str> {
        match self.kind() {
            TOK_COUNT => Some("COUNT"),
            TOK_SUM => Some("SUM"),
            TOK_AVG => Some("AVG"),
            TOK_MIN_KW => Some("MIN"),
            TOK_MAX_KW => Some("MAX"),
            TOK_COLLECT => Some("COLLECT"),
            TOK_TOLOWER => Some("toLower"),
            TOK_TOUPPER => Some("toUpper"),
            TOK_TOSTRING => Some("toString"),
            _ => None,
        }
    }

    fn expect(&mut self, kind: i32) -> Option<()> {
        if self.eat(kind) {
            Some(())
        } else {
            None
        }
    }

    fn eat(&mut self, kind: i32) -> bool {
        if self.at(kind) {
            self.pos += 1;
            true
        } else {
            false
        }
    }

    fn at(&self, kind: i32) -> bool {
        self.kind() == kind
    }

    fn at_stop(&self, stops: &[i32]) -> bool {
        stops.iter().any(|kind| self.at(*kind))
    }

    fn starts_relationship(&self) -> bool {
        self.at(TOK_DASH) || self.at(TOK_LT)
    }

    fn at_name(&self) -> bool {
        matches!(
            self.kind(),
            TOK_IDENT
                | TOK_MATCH
                | TOK_WHERE
                | TOK_RETURN
                | TOK_ORDER
                | TOK_BY
                | TOK_LIMIT
                | TOK_AND
                | TOK_OR
                | TOK_AS
                | TOK_DISTINCT
                | TOK_OPTIONAL
                | TOK_ALL
        )
    }

    fn kind(&self) -> i32 {
        self.current().kind
    }

    fn peek_kind(&self) -> i32 {
        self.tokens
            .get(self.pos + 1)
            .map_or(TOK_EOF, |token| token.kind)
    }

    fn current(&self) -> &Token {
        &self.tokens[self.pos.min(self.tokens.len().saturating_sub(1))]
    }
}

fn render_query(query: &QuerySummary, out: &mut Vec<u8>) {
    if let Some(next) = query.union_next.as_deref() {
        out.extend_from_slice(b"union=");
        out.extend_from_slice(if query.union_all { b"all" } else { b"distinct" });
        out.extend_from_slice(b";left={");
        render_single_query(query, out);
        out.extend_from_slice(b"};right={");
        render_query(next, out);
        out.push(b'}');
        return;
    }
    render_single_query(query, out);
}

fn render_single_query(query: &QuerySummary, out: &mut Vec<u8>) {
    out.extend_from_slice(b"patterns=");
    out.extend_from_slice(query.patterns.len().to_string().as_bytes());
    for (idx, pattern) in query.patterns.iter().enumerate() {
        out.extend_from_slice(b";P");
        out.extend_from_slice(idx.to_string().as_bytes());
        out.extend_from_slice(if pattern.optional {
            b"=optional;nodes="
        } else {
            b"=req;nodes="
        });
        render_nodes(&pattern.nodes, out);
        out.extend_from_slice(b";rels=");
        render_rels(&pattern.rels, out);
    }
    out.extend_from_slice(b";where=");
    render_expr_opt(query.where_expr.as_ref(), out);
    out.extend_from_slice(b";with=");
    render_projection_items(&query.with_items, query.with_distinct, out);
    out.extend_from_slice(b";post_where=");
    render_expr_opt(query.post_with_where.as_ref(), out);
    out.extend_from_slice(b";return=");
    render_projection_items(&query.return_items, query.return_distinct, out);
}

fn render_nodes(nodes: &[NodeSummary], out: &mut Vec<u8>) {
    if nodes.is_empty() {
        out.push(b'-');
        return;
    }
    for (idx, node) in nodes.iter().enumerate() {
        if idx > 0 {
            out.push(b',');
        }
        push_escaped(out, &node.variable);
        out.push(b':');
        push_escaped(out, &node.label);
    }
}

fn render_rels(rels: &[RelSummary], out: &mut Vec<u8>) {
    if rels.is_empty() {
        out.push(b'-');
        return;
    }
    for (idx, rel) in rels.iter().enumerate() {
        if idx > 0 {
            out.push(b',');
        }
        push_escaped(out, &rel.variable);
        out.push(b':');
        render_type_list(&rel.types, out);
        out.push(b':');
        out.extend_from_slice(direction_name(rel.direction).as_bytes());
        out.push(b':');
        out.extend_from_slice(rel.min_hops.to_string().as_bytes());
        out.extend_from_slice(b"..");
        out.extend_from_slice(rel.max_hops.to_string().as_bytes());
    }
}

fn render_type_list(types: &[Vec<u8>], out: &mut Vec<u8>) {
    for (idx, type_name) in types.iter().enumerate() {
        if idx > 0 {
            out.push(b'|');
        }
        push_escaped(out, type_name);
    }
}

fn render_projection_items(items: &[ProjectionItem], distinct: bool, out: &mut Vec<u8>) {
    if items.is_empty() {
        out.push(b'-');
        return;
    }
    if distinct {
        out.extend_from_slice(b"DISTINCT ");
    }
    for (idx, item) in items.iter().enumerate() {
        if idx > 0 {
            out.push(b',');
        }
        if let Some(func) = item.func {
            out.extend_from_slice(func.as_bytes());
            out.push(b'(');
            if item.distinct {
                out.extend_from_slice(b"DISTINCT ");
            }
            push_escaped(out, &item.expr);
            out.push(b')');
        } else {
            push_escaped(out, &item.expr);
        }
        if let Some(alias) = item.alias.as_ref() {
            out.push(b':');
            push_escaped(out, alias);
        }
    }
}

fn render_expr_opt(expr: Option<&ExprSummary>, out: &mut Vec<u8>) {
    if let Some(expr) = expr {
        render_expr(expr, out);
    } else {
        out.push(b'-');
    }
}

fn render_expr(expr: &ExprSummary, out: &mut Vec<u8>) {
    match expr {
        ExprSummary::Condition(cond) => render_condition(cond, out),
        ExprSummary::And(left, right) => render_binary_expr(b"AND", left, right, out),
        ExprSummary::Or(left, right) => render_binary_expr(b"OR", left, right, out),
        ExprSummary::Not(child) => {
            out.extend_from_slice(b"NOT(");
            render_expr(child, out);
            out.push(b')');
        }
    }
}

fn render_binary_expr(name: &[u8], left: &ExprSummary, right: &ExprSummary, out: &mut Vec<u8>) {
    out.extend_from_slice(name);
    out.push(b'(');
    render_expr(left, out);
    out.push(b',');
    render_expr(right, out);
    out.push(b')');
}

fn render_condition(cond: &ConditionSummary, out: &mut Vec<u8>) {
    if cond.op == "EXISTS" {
        out.extend_from_slice(b"EXISTS(");
        push_escaped(out, &cond.variable);
        out.push(b',');
        out.extend_from_slice(
            direction_name(cond.exists_dir.unwrap_or(DirectionSummary::Any)).as_bytes(),
        );
        out.push(b',');
        push_escaped(out, &cond.value);
        out.push(b')');
        return;
    }
    out.extend_from_slice(b"COND(");
    push_escaped(out, &cond.variable);
    if let Some(property) = cond.property.as_ref() {
        out.push(b'.');
        push_escaped(out, property);
    }
    out.push(b',');
    out.extend_from_slice(cond.op.as_bytes());
    out.push(b',');
    push_escaped(out, &cond.value);
    out.push(b')');
}

fn direction_name(direction: DirectionSummary) -> &'static str {
    match direction {
        DirectionSummary::Outbound => "outbound",
        DirectionSummary::Inbound => "inbound",
        DirectionSummary::Any => "any",
    }
}

fn lex_string_literal(input: &[u8], pos: &mut usize, quote: u8) -> Token {
    let start = *pos;
    let mut text = Vec::new();
    while *pos < input.len() && input[*pos] != quote {
        if input[*pos] == b'\\' && *pos + 1 < input.len() {
            *pos += 1;
            if text.len() < STRING_BUF_MAX {
                match input[*pos] {
                    b'n' => text.push(b'\n'),
                    b't' => text.push(b'\t'),
                    b'\\' => text.push(b'\\'),
                    other => text.push(other),
                }
            }
        } else if text.len() < STRING_BUF_MAX {
            text.push(input[*pos]);
        }
        *pos += 1;
    }
    if *pos < input.len() {
        *pos += 1;
    }
    Token {
        kind: TOK_STRING,
        pos: start.saturating_sub(1) as i32,
        text,
    }
}

fn try_number(input: &[u8], pos: &mut usize) -> Option<Token> {
    let c = input[*pos];
    if !(c.is_ascii_digit()
        || c == b'.' && *pos + 1 < input.len() && input[*pos + 1].is_ascii_digit())
    {
        return None;
    }
    let start = *pos;
    while *pos < input.len()
        && (input[*pos].is_ascii_digit()
            || (input[*pos] == b'.' && *pos + 1 < input.len() && input[*pos + 1] != b'.'))
    {
        *pos += 1;
    }
    Some(Token {
        kind: TOK_NUMBER,
        pos: start as i32,
        text: input[start..*pos].to_vec(),
    })
}

fn try_ident(input: &[u8], pos: &mut usize) -> Option<Token> {
    let c = input[*pos];
    if !c.is_ascii_alphabetic() && c != b'_' {
        return None;
    }
    let start = *pos;
    while *pos < input.len() && (input[*pos].is_ascii_alphanumeric() || input[*pos] == b'_') {
        *pos += 1;
    }
    let text = input[start..*pos].to_vec();
    let lookup_len = text.len().min(255);
    let kind = keyword_lookup(&text[..lookup_len]).unwrap_or(TOK_IDENT);
    Some(Token {
        kind,
        pos: start as i32,
        text,
    })
}

fn try_two_char(input: &[u8], pos: &mut usize) -> Option<Token> {
    if *pos + 1 >= input.len() {
        return None;
    }
    let kind = two_char_kind(input[*pos], input[*pos + 1])?;
    let start = *pos;
    *pos += 2;
    Some(Token {
        kind,
        pos: start as i32,
        text: input[start..*pos].to_vec(),
    })
}

pub(crate) fn two_char_kind(first: u8, second: u8) -> Option<i32> {
    match (first, second) {
        (b'!', b'=') | (b'<', b'>') => Some(TOK_NEQ),
        (b'=', b'~') => Some(TOK_EQTILDE),
        (b'>', b'=') => Some(TOK_GTE),
        (b'<', b'=') => Some(TOK_LTE),
        (b'.', b'.') => Some(TOK_DOTDOT),
        _ => None,
    }
}

pub(crate) fn two_char_kind_or_eof(first: u8, second: u8) -> i32 {
    two_char_kind(first, second).unwrap_or(TOK_EOF)
}

fn skip_whitespace_comments(input: &[u8], pos: &mut usize) -> bool {
    if input[*pos].is_ascii_whitespace() {
        *pos += 1;
        return true;
    }
    if *pos + 1 < input.len() && input[*pos] == b'/' && input[*pos + 1] == b'/' {
        while *pos < input.len() && input[*pos] != b'\n' {
            *pos += 1;
        }
        return true;
    }
    if *pos + 1 < input.len() && input[*pos] == b'-' && input[*pos + 1] == b'-' {
        while *pos < input.len() && input[*pos] != b'\n' {
            *pos += 1;
        }
        return true;
    }
    if *pos + 1 < input.len() && input[*pos] == b'/' && input[*pos + 1] == b'*' {
        *pos += 2;
        while *pos + 1 < input.len() && !(input[*pos] == b'*' && input[*pos + 1] == b'/') {
            *pos += 1;
        }
        if *pos + 1 < input.len() {
            *pos += 2;
        }
        return true;
    }
    false
}

pub(crate) fn single_char_kind(c: u8) -> Option<i32> {
    match c {
        b'(' => Some(TOK_LPAREN),
        b')' => Some(TOK_RPAREN),
        b'[' => Some(TOK_LBRACKET),
        b']' => Some(TOK_RBRACKET),
        b'-' => Some(TOK_DASH),
        b'>' => Some(TOK_GT),
        b'<' => Some(TOK_LT),
        b':' => Some(TOK_COLON),
        b'.' => Some(TOK_DOT),
        b'{' => Some(TOK_LBRACE),
        b'}' => Some(TOK_RBRACE),
        b'*' => Some(TOK_STAR),
        b',' => Some(TOK_COMMA),
        b'=' => Some(TOK_EQ),
        b'|' => Some(TOK_PIPE),
        _ => None,
    }
}

pub(crate) fn single_char_kind_or_eof(c: u8) -> i32 {
    single_char_kind(c).unwrap_or(TOK_EOF)
}

fn keyword_lookup(word: &[u8]) -> Option<i32> {
    KEYWORDS
        .iter()
        .find(|(name, _)| ascii_eq_ignore_case(word, name.as_bytes()))
        .map(|(_, kind)| *kind)
}

fn ascii_eq_ignore_case(a: &[u8], b: &[u8]) -> bool {
    a.len() == b.len()
        && a.iter()
            .zip(b)
            .all(|(left, right)| left.eq_ignore_ascii_case(right))
}

fn token_name(kind: i32) -> &'static str {
    TOKEN_NAMES.get(kind as usize).copied().unwrap_or("UNKNOWN")
}

fn push_escaped(out: &mut Vec<u8>, text: &[u8]) {
    for &byte in text {
        match byte {
            b'\n' => out.extend_from_slice(b"\\n"),
            b'\t' => out.extend_from_slice(b"\\t"),
            b'\r' => out.extend_from_slice(b"\\r"),
            b'\\' => out.extend_from_slice(b"\\\\"),
            b'|' => out.extend_from_slice(b"\\|"),
            b':' => out.extend_from_slice(b"\\:"),
            0x20..=0x7e => out.push(byte),
            other => {
                out.extend_from_slice(b"\\x");
                out.push(hex_digit(other >> 4));
                out.push(hex_digit(other & 0x0f));
            }
        }
    }
}

fn hex_digit(nibble: u8) -> u8 {
    match nibble {
        0..=9 => b'0' + nibble,
        10..=15 => b'a' + (nibble - 10),
        _ => unreachable!("nibble is masked before hex conversion"),
    }
}

const KEYWORDS: &[(&str, i32)] = &[
    ("MATCH", TOK_MATCH),
    ("WHERE", TOK_WHERE),
    ("RETURN", TOK_RETURN),
    ("ORDER", TOK_ORDER),
    ("BY", TOK_BY),
    ("LIMIT", TOK_LIMIT),
    ("AND", TOK_AND),
    ("OR", TOK_OR),
    ("AS", TOK_AS),
    ("DISTINCT", TOK_DISTINCT),
    ("COUNT", TOK_COUNT),
    ("CONTAINS", TOK_CONTAINS),
    ("STARTS", TOK_STARTS),
    ("WITH", TOK_WITH),
    ("NOT", TOK_NOT),
    ("ASC", TOK_ASC),
    ("DESC", TOK_DESC),
    ("ENDS", TOK_ENDS),
    ("IN", TOK_IN),
    ("IS", TOK_IS),
    ("NULL", TOK_NULL_KW),
    ("XOR", TOK_XOR),
    ("SKIP", TOK_SKIP),
    ("UNION", TOK_UNION),
    ("UNWIND", TOK_UNWIND),
    ("SUM", TOK_SUM),
    ("AVG", TOK_AVG),
    ("MIN", TOK_MIN_KW),
    ("MAX", TOK_MAX_KW),
    ("COLLECT", TOK_COLLECT),
    ("toLower", TOK_TOLOWER),
    ("toUpper", TOK_TOUPPER),
    ("toString", TOK_TOSTRING),
    ("tolower", TOK_TOLOWER),
    ("toupper", TOK_TOUPPER),
    ("tostring", TOK_TOSTRING),
    ("CASE", TOK_CASE),
    ("WHEN", TOK_WHEN),
    ("THEN", TOK_THEN),
    ("ELSE", TOK_ELSE),
    ("END", TOK_END),
    ("CREATE", TOK_CREATE),
    ("DELETE", TOK_DELETE),
    ("DETACH", TOK_DETACH),
    ("SET", TOK_SET),
    ("REMOVE", TOK_REMOVE),
    ("MERGE", TOK_MERGE),
    ("YIELD", TOK_YIELD),
    ("CALL", TOK_CALL),
    ("ALL", TOK_ALL),
    ("TRUE", TOK_TRUE),
    ("FALSE", TOK_FALSE),
    ("EXISTS", TOK_EXISTS),
    ("MANDATORY", TOK_MANDATORY),
    ("FOREACH", TOK_FOREACH),
    ("ON", TOK_ON),
    ("ADD", TOK_ADD),
    ("CONSTRAINT", TOK_CONSTRAINT),
    ("DO", TOK_DO),
    ("DROP", TOK_DROP),
    ("FOR", TOK_FOR),
    ("FROM", TOK_FROM),
    ("GRAPH", TOK_GRAPH),
    ("OF", TOK_OF),
    ("REQUIRE", TOK_REQUIRE),
    ("SCALAR", TOK_SCALAR),
    ("UNIQUE", TOK_UNIQUE),
    ("OPTIONAL", TOK_OPTIONAL),
];

const TOKEN_NAMES: &[&str] = &[
    "MATCH",
    "WHERE",
    "RETURN",
    "ORDER",
    "BY",
    "LIMIT",
    "AND",
    "OR",
    "AS",
    "DISTINCT",
    "COUNT",
    "CONTAINS",
    "STARTS",
    "WITH",
    "NOT",
    "ASC",
    "DESC",
    "NEQ",
    "ENDS",
    "IN",
    "IS",
    "NULL",
    "XOR",
    "SKIP",
    "UNION",
    "UNWIND",
    "SUM",
    "AVG",
    "MIN",
    "MAX",
    "COLLECT",
    "TOLOWER",
    "TOUPPER",
    "TOSTRING",
    "CASE",
    "WHEN",
    "THEN",
    "ELSE",
    "END",
    "CREATE",
    "DELETE",
    "DETACH",
    "SET",
    "REMOVE",
    "MERGE",
    "OPTIONAL",
    "YIELD",
    "CALL",
    "ALL",
    "TRUE",
    "FALSE",
    "EXISTS",
    "MANDATORY",
    "FOREACH",
    "ON",
    "ADD",
    "CONSTRAINT",
    "DO",
    "DROP",
    "FOR",
    "FROM",
    "GRAPH",
    "OF",
    "REQUIRE",
    "SCALAR",
    "UNIQUE",
    "LPAREN",
    "RPAREN",
    "LBRACKET",
    "RBRACKET",
    "DASH",
    "GT",
    "LT",
    "COLON",
    "DOT",
    "LBRACE",
    "RBRACE",
    "STAR",
    "COMMA",
    "EQ",
    "EQTILDE",
    "GTE",
    "LTE",
    "PIPE",
    "DOTDOT",
    "IDENT",
    "STRING",
    "NUMBER",
    "EOF",
];

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn scalar_func_index_matches_c_contract() {
        assert_eq!(scalar_func_index(Some(b"labels")), Some(0));
        assert_eq!(scalar_func_index(Some(b"LABELS")), Some(0));
        assert_eq!(scalar_func_index(Some(b"Type")), Some(1));
        assert_eq!(scalar_func_index(Some(b"toInteger")), Some(5));
        assert_eq!(scalar_func_index(Some(b"TOINTEGER")), Some(5));
        assert_eq!(scalar_func_index(Some(b"toBoolean")), Some(7));
        assert_eq!(scalar_func_index(Some(b"reverse")), Some(13));
        assert_eq!(scalar_func_index(Some(b"REVERSE")), Some(13));
        // 不符合：未知名稱、長度不符、空、null、由別處處理的關鍵字
        assert_eq!(scalar_func_index(Some(b"unknown")), None);
        assert_eq!(scalar_func_index(Some(b"label")), None);
        assert_eq!(scalar_func_index(Some(b"labelsX")), None);
        assert_eq!(scalar_func_index(Some(b"")), None);
        assert_eq!(scalar_func_index(None), None);
        assert_eq!(scalar_func_index(Some(b"toLower")), None);
        assert_eq!(scalar_func_index(Some(b"toString")), None);
        // 非 ASCII byte 不應誤判為已知函式
        assert_eq!(scalar_func_index(Some(b"label\xc3\xa9")), None);
    }

    #[test]
    fn multiarg_func_index_matches_c_contract() {
        assert_eq!(multiarg_func_index(Some(b"coalesce")), Some(0));
        assert_eq!(multiarg_func_index(Some(b"COALESCE")), Some(0));
        assert_eq!(multiarg_func_index(Some(b"substring")), Some(1));
        assert_eq!(multiarg_func_index(Some(b"Replace")), Some(2));
        assert_eq!(multiarg_func_index(Some(b"LEFT")), Some(3));
        assert_eq!(multiarg_func_index(Some(b"right")), Some(4));
        assert_eq!(multiarg_func_index(None), None);
        assert_eq!(multiarg_func_index(Some(b"")), None);
        assert_eq!(multiarg_func_index(Some(b"coalesc")), None);
        assert_eq!(multiarg_func_index(Some(b"coalesceX")), None);
        assert_eq!(multiarg_func_index(Some(b"count")), None);
        assert_eq!(multiarg_func_index(Some(b"toLower")), None);
        assert_eq!(multiarg_func_index(Some(b"labels")), None);
        assert_eq!(multiarg_func_index(Some(b"split")), None);
        assert_eq!(multiarg_func_index(Some(b"right\xc3\xa9")), None);
    }

    #[test]
    fn aggregate_func_index_matches_c_contract() {
        assert_eq!(aggregate_func_index(TOK_COUNT), Some(0));
        assert_eq!(aggregate_func_index(TOK_SUM), Some(1));
        assert_eq!(aggregate_func_index(TOK_AVG), Some(2));
        assert_eq!(aggregate_func_index(TOK_MIN_KW), Some(3));
        assert_eq!(aggregate_func_index(TOK_MAX_KW), Some(4));
        assert_eq!(aggregate_func_index(TOK_COLLECT), Some(5));
        assert_eq!(aggregate_func_index(TOK_TOLOWER), None);
        assert_eq!(aggregate_func_index(TOK_IDENT), None);
        assert_eq!(aggregate_func_index(-1), None);
        assert_eq!(aggregate_func_index(TOK_EOF), None);
    }

    #[test]
    fn string_func_index_matches_c_contract() {
        assert_eq!(string_func_index(TOK_TOLOWER), Some(0));
        assert_eq!(string_func_index(TOK_TOUPPER), Some(1));
        assert_eq!(string_func_index(TOK_TOSTRING), Some(2));
        assert_eq!(string_func_index(TOK_COUNT), None);
        assert_eq!(string_func_index(TOK_IDENT), None);
        assert_eq!(string_func_index(-1), None);
        assert_eq!(string_func_index(TOK_EOF), None);
    }

    #[test]
    fn single_char_kind_matches_c_contract() {
        let expected = [
            (b'(', TOK_LPAREN),
            (b')', TOK_RPAREN),
            (b'[', TOK_LBRACKET),
            (b']', TOK_RBRACKET),
            (b'-', TOK_DASH),
            (b'>', TOK_GT),
            (b'<', TOK_LT),
            (b':', TOK_COLON),
            (b'.', TOK_DOT),
            (b'{', TOK_LBRACE),
            (b'}', TOK_RBRACE),
            (b'*', TOK_STAR),
            (b',', TOK_COMMA),
            (b'=', TOK_EQ),
            (b'|', TOK_PIPE),
        ];

        for (input, kind) in expected {
            assert_eq!(single_char_kind(input), Some(kind));
            assert_eq!(single_char_kind_or_eof(input), kind);
        }
        assert_eq!(single_char_kind(b'@'), None);
        assert_eq!(single_char_kind_or_eof(b'@'), TOK_EOF);
        assert_eq!(single_char_kind(0x80), None);
    }

    #[test]
    fn two_char_kind_matches_c_contract() {
        let expected = [
            ((b'!', b'='), TOK_NEQ),
            ((b'<', b'>'), TOK_NEQ),
            ((b'=', b'~'), TOK_EQTILDE),
            ((b'>', b'='), TOK_GTE),
            ((b'<', b'='), TOK_LTE),
            ((b'.', b'.'), TOK_DOTDOT),
        ];

        for ((first, second), kind) in expected {
            assert_eq!(two_char_kind(first, second), Some(kind));
            assert_eq!(two_char_kind_or_eof(first, second), kind);
        }
        assert_eq!(two_char_kind(b'@', b'!'), None);
        assert_eq!(two_char_kind_or_eof(b'@', b'!'), TOK_EOF);
        assert_eq!(two_char_kind(0x80, b'='), None);
    }

    #[test]
    fn lexes_core_contract_tokens() {
        let input = b"MATCH (f:Function)-[r:CALLS|DEFINES*1..2]->(g) \
                      WHERE f.name =~ \"Al\\npha\" RETURN COUNT(g)";
        let summary = String::from_utf8(summary(input)).unwrap();
        assert_eq!(
            summary,
            "MATCH@0:MATCH|LPAREN@6:(|IDENT@7:f|COLON@8:\\:|IDENT@9:Function|RPAREN@17:)|DASH@18:-|LBRACKET@19:[|IDENT@20:r|COLON@21:\\:|IDENT@22:CALLS|PIPE@27:\\||IDENT@28:DEFINES|STAR@35:*|NUMBER@36:1|DOTDOT@37:..|NUMBER@39:2|RBRACKET@40:]|DASH@41:-|GT@42:>|LPAREN@43:(|IDENT@44:g|RPAREN@45:)|WHERE@47:WHERE|IDENT@53:f|DOT@54:.|IDENT@55:name|EQTILDE@60:=~|STRING@63:Al\\npha|RETURN@73:RETURN|COUNT@80:COUNT|LPAREN@85:(|IDENT@86:g|RPAREN@87:)|EOF@88:"
        );
    }

    #[test]
    fn skips_comments_and_keeps_c_string_escape_contract() {
        let input = b"// one\nMATCH /* block */ (n) -- two\nRETURN 'a\\tb\\'c'";
        let summary = String::from_utf8(summary(input)).unwrap();
        assert_eq!(
            summary,
            "MATCH@7:MATCH|LPAREN@25:(|IDENT@26:n|RPAREN@27:)|RETURN@36:RETURN|STRING@43:a\\tb'c|EOF@52:"
        );
    }

    #[test]
    fn handles_numbers_dotdot_keywords_and_unknown_bytes() {
        let tokens = lex(b"CALL db.labels() YIELD label RETURN .5, 1..2, @");
        let kinds: Vec<i32> = tokens.iter().map(|token| token.kind).collect();
        assert_eq!(
            kinds,
            vec![
                TOK_CALL, TOK_IDENT, TOK_DOT, TOK_IDENT, TOK_LPAREN, TOK_RPAREN, TOK_YIELD,
                TOK_IDENT, TOK_RETURN, TOK_NUMBER, TOK_COMMA, TOK_NUMBER, TOK_DOTDOT, TOK_NUMBER,
                TOK_COMMA, TOK_EOF
            ]
        );
    }

    #[test]
    fn summarizes_core_parser_ast_shape() {
        let input = b"MATCH (n:Function|Module)-[r:CALLS|DEFINES*1..2]->(m:Function) \
                      WHERE n.name = \"main\" OR n.name = \"Alpha\" AND m.name = \"Beta\" \
                      RETURN n.name";
        let summary = String::from_utf8(parse_summary(input).unwrap()).unwrap();
        assert_eq!(
            summary,
            "patterns=1;P0=req;nodes=n:Function\\|Module,m:Function;\
             rels=r:CALLS|DEFINES:outbound:1..2;\
             where=OR(COND(n.name,=,main),AND(COND(n.name,=,Alpha),COND(m.name,=,Beta)));\
             with=-;post_where=-;return=n.name"
        );
    }

    #[test]
    fn summarizes_optional_union_exists_and_with_shape() {
        let optional = String::from_utf8(
            parse_summary(
                b"MATCH (f:Function) OPTIONAL MATCH (f)-[:CALLS]->(g:Function) \
                  RETURN f.name, g.name",
            )
            .unwrap(),
        )
        .unwrap();
        assert_eq!(
            optional,
            "patterns=2;P0=req;nodes=f:Function;rels=-;\
             P1=optional;nodes=f:,g:Function;rels=:CALLS:outbound:1..1;\
             where=-;with=-;post_where=-;return=f.name,g.name"
        );

        let union = String::from_utf8(
            parse_summary(
                b"MATCH (f:Function) RETURN f.name UNION ALL \
                  MATCH (g:Function) RETURN g.name",
            )
            .unwrap(),
        )
        .unwrap();
        assert_eq!(
            union,
            "union=all;left={patterns=1;P0=req;nodes=f:Function;rels=-;\
             where=-;with=-;post_where=-;return=f.name};\
             right={patterns=1;P0=req;nodes=g:Function;rels=-;\
             where=-;with=-;post_where=-;return=g.name}"
        );

        let exists = String::from_utf8(
            parse_summary(
                b"MATCH (f:Function) WHERE NOT EXISTS { (f)<-[:CALLS]-() } RETURN f.name",
            )
            .unwrap(),
        )
        .unwrap();
        assert_eq!(
            exists,
            "patterns=1;P0=req;nodes=f:Function;rels=-;\
             where=NOT(EXISTS(f,inbound,CALLS));with=-;post_where=-;return=f.name"
        );

        let with_post_where = String::from_utf8(
            parse_summary(
                b"MATCH (f:Function)-[:CALLS]->(g:Function) \
                  WITH f.name AS caller, COUNT(g) AS cnt WHERE cnt > \"1\" RETURN caller",
            )
            .unwrap(),
        )
        .unwrap();
        assert_eq!(
            with_post_where,
            "patterns=1;P0=req;nodes=f:Function,g:Function;rels=:CALLS:outbound:1..1;\
             where=-;with=f.name:caller,COUNT(g):cnt;\
             post_where=COND(cnt,>,1);return=caller"
        );
    }

    #[test]
    fn summarizes_union_distinct_shape() {
        let union = String::from_utf8(
            parse_summary(
                b"MATCH (f:Function) RETURN f.name UNION MATCH (g:Function) RETURN g.name",
            )
            .unwrap(),
        )
        .unwrap();
        assert_eq!(
            union,
            "union=distinct;left={patterns=1;P0=req;nodes=f:Function;rels=-;\
             where=-;with=-;post_where=-;return=f.name};\
             right={patterns=1;P0=req;nodes=g:Function;rels=-;\
             where=-;with=-;post_where=-;return=g.name}"
        );
    }

    #[test]
    fn summarizes_hop_range_shorthand_like_c_parser() {
        let single_number = String::from_utf8(
            parse_summary(b"MATCH (a:Function)-[:CALLS*2]->(b:Function) RETURN b.name").unwrap(),
        )
        .unwrap();
        assert_eq!(
            single_number,
            "patterns=1;P0=req;nodes=a:Function,b:Function;\
             rels=:CALLS:outbound:1..2;where=-;with=-;post_where=-;return=b.name"
        );

        let unbounded = String::from_utf8(
            parse_summary(b"MATCH (a:Function)-[:CALLS*]->(b:Function) RETURN b.name").unwrap(),
        )
        .unwrap();
        assert_eq!(
            unbounded,
            "patterns=1;P0=req;nodes=a:Function,b:Function;\
             rels=:CALLS:outbound:1..0;where=-;with=-;post_where=-;return=b.name"
        );

        let omitted_min = String::from_utf8(
            parse_summary(b"MATCH (a:Function)-[:CALLS*..3]->(b:Function) RETURN b.name").unwrap(),
        )
        .unwrap();
        assert_eq!(
            omitted_min,
            "patterns=1;P0=req;nodes=a:Function,b:Function;\
             rels=:CALLS:outbound:1..3;where=-;with=-;post_where=-;return=b.name"
        );
    }

    #[test]
    fn rejects_exists_identifier_syntax() {
        assert!(parse_summary(b"MATCH (f:Function) WHERE EXISTS (f) RETURN f.name").is_none());
    }

    #[test]
    fn summarizes_return_and_with_distinct_shape() {
        let with_distinct = String::from_utf8(
            parse_summary(
                b"MATCH (f:Function) WITH DISTINCT f.name AS caller WHERE caller = \"x\" RETURN caller",
            )
            .unwrap(),
        )
        .unwrap();
        assert_eq!(
            with_distinct,
            "patterns=1;P0=req;nodes=f:Function;rels=-;\
             where=-;with=DISTINCT f.name:caller;post_where=COND(caller,=,x);return=caller"
        );

        let return_distinct = String::from_utf8(
            parse_summary(
                b"MATCH (f:Function) RETURN DISTINCT f.name ORDER BY f.name LIMIT 1 SKIP 0",
            )
            .unwrap(),
        )
        .unwrap();
        assert_eq!(
            return_distinct,
            "patterns=1;P0=req;nodes=f:Function;rels=-;\
             where=-;with=-;post_where=-;return=DISTINCT f.name"
        );

        let count_distinct = String::from_utf8(
            parse_summary(
                b"MATCH (f:Function) RETURN DISTINCT COUNT(DISTINCT f.name) AS name_count",
            )
            .unwrap(),
        )
        .unwrap();
        assert_eq!(
            count_distinct,
            "patterns=1;P0=req;nodes=f:Function;rels=-;\
             where=-;with=-;post_where=-;return=DISTINCT COUNT(DISTINCT f.name):name_count"
        );
    }

    #[test]
    fn rejects_malformed_parser_summary_queries() {
        assert!(parse_summary(b"MATCH (f:Function WHERE f.name = \"x\"").is_none());
        assert!(parse_summary(b"WHERE f.name = \"x\"").is_none());
    }
}

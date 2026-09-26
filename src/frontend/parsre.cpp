#include "../../include/core/macros.hpp"
#include "../../include/lexer/token.hpp"
#include "../../include/parser/parser.hpp"

Token Parser::peek() const {
  auto res = lexer_.next_token(state_);
  if (res.is_err())
    return Token{TokenKind::TK_EoF, Span{}, {}};
  return res.value().token;
}

Token Parser::consume_token() {
  auto res = lexer_.next_token(state_);
  if (res.is_err())
    return Token{TokenKind::TK_EoF, Span{}, {}};
  state_ = res.value().next_state;
  return res.value().token;
}

bool Parser::match(TokenKind kind) {
  if (peek().kind == kind) {
    consume_token();
    return true;
  }
  return false;
}

void Parser::synchronize() {
  consume_token();
  while (peek().kind != TokenKind::TK_EoF) {
    if (peek().kind == TokenKind::TK_SEMICOLON) {
      consume_token();
      return;
    }
    switch (peek().kind) {
    case TokenKind::TK_LET:
    case TokenKind::TK_FN:
    case TokenKind::TK_IF:
    case TokenKind::TK_WHILE:
    case TokenKind::TK_RET:
      return;
    default:
      consume_token();
    }
  }
}

Prec Parser::token_prec(TokenKind kind) const {
  switch (kind) {
  case TokenKind::TK_ASSIGN:
    return Prec::ASSIGN;
  case TokenKind::TK_PLUS:
    return Prec::SUM;
  case TokenKind::TK_MINUS:
    return Prec::SUM;
  case TokenKind::TK_STAR:
    return Prec::PRODUCT;
  case TokenKind::TK_SLASH:
    return Prec::PRODUCT;
  case TokenKind::TK_LPRN:
    return Prec::CALL;
  default:
    return Prec::LOWEST;
  }
}

Node *Parser::parse_nud() {
  Token tok = consume_token();

  switch (tok.kind) {
  case TokenKind::TK_INT_LITERAL: {
    Node *node = PushStructZero(arena_, Node);
    node->kind = NodeKind::EXPR_INT_LITERAL;
    node->int_val = tok.int_val;
    node->span = tok.span;
    return node;
  }

  case TokenKind::TK_STR_LITERAL: {
    Node *node = PushStructZero(arena_, Node);
    node->kind = NodeKind::EXPR_STR_LITERAL;
    node->symbol_id = tok.symbol_id;
    node->span = tok.span;
    return node;
  }

  case TokenKind::TK_ID: {
    Node *node = PushStructZero(arena_, Node);
    node->kind = NodeKind::EXPR_IDENT;
    node->symbol_id = tok.symbol_id;
    node->span = tok.span;
    return node;
  }

  case TokenKind::TK_MINUS: {
    Node *node = PushStructZero(arena_, Node);
    node->kind = NodeKind::EXPR_UNARY;
    node->span = tok.span;
    node->unary.op = tok.kind;
    node->unary.operand = parse_expr(Prec::PREFIX);
    if (!node->unary.operand)
      return nullptr;

    return node;
  }

  case TokenKind::TK_LPRN: {
    Node *expr = parse_expr(Prec::LOWEST);
    if (!expr) {
      return nullptr;
    }
    EXPECT_NORETURN(TokenKind::TK_RPRN);
    return expr;
  }

  default:
    diag_.push_diag({.severity = SeverityKind::Err,
                     .span = tok.span,
                     .err_msg = "expected nud expr, got: " +
                                (string)token_kind_to_str(tok.kind)});
    return nullptr;
  }
}

Node *Parser::parse_led(Node *left, Token op) {

  switch (op.kind) {
  case TokenKind::TK_PLUS:
  case TokenKind::TK_MINUS:
  case TokenKind::TK_STAR:
  case TokenKind::TK_SLASH: {
    Node *node = PushStructZero(arena_, Node);
    node->kind = NodeKind::EXPR_BINARY;
    node->span = op.span;
    node->binary.lhs = left;
    node->binary.op = op.kind;
    Prec prec = token_prec(op.kind);
    node->binary.rhs = parse_expr(prec);
    if (!node->binary.rhs) {
      return nullptr;
    }
    return node;
  }
  case TokenKind::TK_LPRN: {
    Node *call = PushStructZero(arena_, Node);
    call->kind = NodeKind::EXPR_CALL;
    call->span = op.span;
    call->call.callee = left;
    NodeList *args_head = nullptr;
    NodeList **args_tail = &args_head;
    int size = 0;
    if (peek().kind != TokenKind::TK_RPRN) {
      do {
        Node *arg = parse_expr(Prec::LOWEST);
        if (!arg) {
          break;
        }

        NodeList *elem = PushStructZero(arena_, NodeList);
        elem->node = arg;
        *args_tail = elem;
        args_tail = &elem->next;
        size++;
      } while (match(TokenKind::TK_COMMA));
    }
    EXPECT_NORETURN(TokenKind::TK_RPRN);
    call->call.args = args_head;
    call->call.size = size;
    return call;
  }
  default:
    diag_.push_diag({.severity = SeverityKind::Err,
                     .span = op.span,
                     .err_msg = "expected led expr, got: " +
                                (string)token_kind_to_str(op.kind)});
    return nullptr;
  }
}

Node *Parser::parse_expr(Prec prec) {
  Node *left = parse_nud();
  if (!left) {
    return nullptr;
  }

  while (prec < token_prec(peek().kind)) {
    Token op = consume_token();
    left = parse_led(left, op);
    if (!left) {
      return nullptr;
    }
  }
  return left;
}

Node *Parser::parse_stmt() {
  switch (peek().kind) {
  case TokenKind::TK_LBRACE:
    return parse_block();
  case TokenKind::TK_LET:
    return parse_decl();
  case TokenKind::TK_IF:
    return parse_if();
  case TokenKind::TK_WHILE:
    return parse_while();
  case TokenKind::TK_RET:
    return parse_ret();
  default:

    Node *expr = parse_expr();
    if (!expr) {
      return nullptr;
    }
    EXPECT_NORETURN(TokenKind::TK_SEMICOLON);

    Node *stmt = PushStructZero(arena_, Node);
    stmt->kind = NodeKind::STMT_EXPR;
    stmt->expr_stmt.expr = expr;
    return stmt;
  }
}

Node *Parser::parse_params() {
  // Token param_tok = EXPECT_RETURN(TokenKind::TK_ID);
  // EXPECT_NORETURN(TokenKind::TK_COLON);
  if (peek().kind != TokenKind::TK_ID) {
    diag_.push_diag({.severity = SeverityKind::Err,
                     .span = peek().span,
                     .err_msg = "expected parameter name"});
    return nullptr;
  }
  Token param_tok = consume_token();

  if (!match(TokenKind::TK_COLON)) {
    diag_.push_diag({.severity = SeverityKind::Err,
                     .span = peek().span,
                     .err_msg = "expected ':' after parameter name"});
    return nullptr;
  }
  switch (peek().kind) {
  case TokenKind::TK_I32:
  case TokenKind::TK_STRING: {
    Node *param = PushStructZero(arena_, Node);
    param->kind = NodeKind::DECL_PARAM;
    param->span = param_tok.span;
    param->params.name_id = param_tok.symbol_id;
    param->params.type_id = consume_token().symbol_id;
    return param;
  }

  default:
    diag_.push_diag(
        {.severity = SeverityKind::Err,
         .span = peek().span,
         .err_msg = "Unknown Type: " + (string)token_kind_to_str(peek().kind)});
    return nullptr;
  }
}

Option<u32> Parser::parse_type() {
  switch (peek().kind) {
  case TokenKind::TK_I32:
  case TokenKind::TK_STRING:
  case TokenKind::TK_ID:
    return Option<u32>::some(consume_token().symbol_id);

  default:
    diag_.push_diag(
        {.severity = SeverityKind::Err,
         .span = peek().span,
         .err_msg = "Unknown Type: " + (string)token_kind_to_str(peek().kind)});
    return Option<u32>::none();
  }
}

Node *Parser::parse_decl() {

  switch (peek().kind) {
  case TokenKind::TK_LET: {
    EXPECT_NORETURN(TokenKind::TK_LET);
    Token id_tok = EXPECT_RETURN(TokenKind::TK_ID);

    if (!match(TokenKind::TK_COLON)) {
      diag_.push_diag({.severity = SeverityKind::Err,
                       .span = peek().span,
                       .err_msg = "expected: ':' after variable declaration " +
                                  (string)interner_.view(id_tok.symbol_id)});
      return nullptr;
    }

    Option<u32> type_id = parse_type();
    if (type_id.is_none()) {
      return nullptr;
    }

    Node *init_expr = nullptr;
    if (match(TokenKind::TK_ASSIGN)) {
      init_expr = parse_expr();
      if (!init_expr) {
        return nullptr;
      }
    }
    EXPECT_NORETURN(TokenKind::TK_SEMICOLON);

    Node *decl = PushStructZero(arena_, Node);
    decl->kind = NodeKind::DECL_VAR;
    decl->span = id_tok.span;
    decl->var_decl.init = init_expr;
    decl->var_decl.name_id = id_tok.symbol_id;
    decl->var_decl.type_id = type_id.value();
    return decl;
  }

  case TokenKind::TK_FN: {
    EXPECT_NORETURN(TokenKind::TK_FN);
    Token id_tok = EXPECT_RETURN(TokenKind::TK_ID);

    Node *fn = PushStructZero(arena_, Node);

    if (peek().kind != TokenKind::TK_LPRN) {
      diag_.push_diag({.severity = SeverityKind::Err,
                       .span = peek().span,
                       .err_msg = "expected: '(' after function declaration" +
                                  (string)interner_.view(id_tok.symbol_id)});
      return nullptr;
    }
    EXPECT_NORETURN(TokenKind::TK_LPRN);

    NodeList *head = nullptr;
    NodeList **tail = &head;

    if (peek().kind != TokenKind::TK_RPRN) {
      do {

        Node *param = parse_params();
        if (!param) {
          return nullptr;
        }
        NodeList *elem = PushStructZero(arena_, NodeList);
        elem->node = param;
        *tail = elem;
        tail = &elem->next;

      } while (match(TokenKind::TK_COMMA));
    }
    EXPECT_NORETURN(TokenKind::TK_RPRN);
    EXPECT_NORETURN(TokenKind::TK_COLON);
    Option<u32> type_id = parse_type();
    if (type_id.is_none()) {
      return nullptr;
    }

    Node *body = parse_block();
    if (!body) {
      diag_.push_diag({.severity = SeverityKind::Err,
                       .span = peek().span,
                       .err_msg = "invalid fn body: " +
                                  (string)interner_.view(id_tok.symbol_id)});
      return nullptr;
    }
    // PANICF("invalid fn body: (%.*s)", SV(interner_, id_tok.symbol_id));

    fn->kind = NodeKind::DECL_FN;
    fn->span = id_tok.span;
    fn->fn_decl.name_id = id_tok.symbol_id;
    fn->fn_decl.type_id = type_id.value();
    fn->fn_decl.params = head;
    fn->fn_decl.body = body;
    return fn;
  }

  default:
    diag_.push_diag({.severity = SeverityKind::Err,
                     .span = peek().span,
                     .err_msg = "invalid fn body: " +
                                (string)interner_.view(peek().symbol_id)});
    return nullptr;
    // PANICF("unknown top declaration (%s)", token_kind_to_str(peek().kind));
  }
}

Node *Parser::parse_block() {
  NodeList *head = NULL;
  NodeList **tail = &head;

  EXPECT_NORETURN(TokenKind::TK_LBRACE);
  while (peek().kind != TokenKind::TK_EoF &&
         peek().kind != TokenKind::TK_RBRACE) {
    Node *stmts = parse_stmt();
    if (!stmts) {
      synchronize();
    }

    NodeList *elems = PushStructZero(arena_, NodeList);
    elems->node = stmts;
    *tail = elems;
    tail = &elems->next;
  }
  EXPECT_NORETURN(TokenKind::TK_RBRACE);

  Node *block = PushStructZero(arena_, Node);
  block->kind = NodeKind::STMT_BLOCK;
  block->block_stmt.stmts = head;

  return block;
}

Node *Parser::parse_while() {
  Token while_tok = EXPECT_RETURN(TokenKind::TK_WHILE);

  Node *cond = parse_expr();
  if (!cond) {
    diag_.push_diag({.severity = SeverityKind::Err,
                     .span = peek().span,
                     .err_msg = "expeceted condition after while statement: "});
    return nullptr;
  }

  Node *body = parse_block();
  if (!body) {
    diag_.push_diag({.severity = SeverityKind::Err,
                     .span = peek().span,
                     .err_msg = "expected while body after cond"});
    return nullptr;
  }
  // PANICF("expected while body after cond");

  Node *while_node = PushStructZero(arena_, Node);

  while_node->kind = NodeKind::STMT_WHILE;
  while_node->span = while_tok.span;
  while_node->while_stmt.cond = cond;
  while_node->while_stmt.body = body;

  return while_node;
}

Node *Parser::parse_if() {
  Token if_tok = EXPECT_RETURN(TokenKind::TK_IF);

  Node *cond = parse_expr();
  if (!cond) {
    diag_.push_diag({.severity = SeverityKind::Err,
                     .span = peek().span,
                     .err_msg = "expected condition after if statement"});
    return nullptr;
  }
  // PANICF("expected condition after if statmenet");

  Node *then_body = parse_block();
  if (!then_body) {
    diag_.push_diag({.severity = SeverityKind::Err,
                     .span = peek().span,
                     .err_msg = "expected if body after cond"});
    return nullptr;
  }
  // PANICF("expected if body after cond");

  Node *if_node = PushStructZero(arena_, Node);

  if (peek().kind == TokenKind::TK_ELSE) {
    Token else_tok = consume_token();
    if (peek().kind == TokenKind::TK_IF) {
      if_node->if_stmt.else_body = parse_if();
      if (!if_node->if_stmt.else_body) {
        return nullptr;
      }
    } else {
      if_node->if_stmt.else_body = parse_block();
      if (!if_node->if_stmt.else_body) {
        return nullptr;
      }
    }
  }

  if_node->kind = NodeKind::STMT_IF;
  if_node->span = if_tok.span;
  if_node->if_stmt.cond = cond;
  if_node->if_stmt.then_body = then_body;

  return if_node;
}

Node *Parser::parse_ret() {
  Token ret_tok = EXPECT_RETURN(TokenKind::TK_RET);
  Node *expr = parse_expr();
  if (!expr) {
    return nullptr;
  }
  EXPECT_NORETURN(TokenKind::TK_SEMICOLON);

  Node *ret = PushStructZero(arena_, Node);
  ret->kind = NodeKind::STMT_RET;
  ret->span = ret_tok.span;
  ret->ret_stmt.expr = expr;
  return ret;
}

Node *Parser::parse_prog() {
  NodeList *head = nullptr;
  NodeList **tail = &head;

  while (peek().kind != TokenKind::TK_EoF) {
    Node *decls = parse_decl();
    if (!decls) {
      synchronize();
      return nullptr;
    }

    NodeList *elem = PushStructZero(arena_, NodeList);
    elem->node = decls;
    *tail = elem;
    tail = &elem->next;
  }

  Node *root = PushStructZero(arena_, Node);
  root->kind = NodeKind::DECL;
  root->declaration.decls = head;

  return root;
}

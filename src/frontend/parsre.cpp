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

// Token Parser::expect(TokenKind kind) {
// Token tok = consume_token();
// if (tok.kind != kind)
//   PANICF("Parser Error: expected token '%s' got '%s'\n",
//   token_kind_to_str(kind),
//          token_kind_to_str(tok.kind));
// return tok;
// }

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
    return node;
  }

  case TokenKind::TK_LPRN: {
    Node *expr = parse_expr(Prec::LOWEST);
    EXPECT_NORETURN(TokenKind::TK_RPRN);
    return expr;
  }

  default:
    PANICF("expected prefix expr, got: %s", token_kind_to_str(tok.kind));
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
    return node;
  }
  case TokenKind::TK_LPRN: {
    Node *call = PushStructZero(arena_, Node);
    call->kind = NodeKind::EXPR_CALL;
    call->span = op.span;
    call->call.callee = left;
    NodeList *args_head = nullptr;
    NodeList **args_tail = &args_head;
    if (peek().kind != TokenKind::TK_RPRN) {
      do {
        Node *arg = parse_expr(Prec::LOWEST);
        if (!arg)
          break;

        NodeList *elem = PushStructZero(arena_, NodeList);
        elem->node = arg;
        *args_tail = elem;
        args_tail = &elem->next;
      } while (match(TokenKind::TK_COMMA));
    }
    EXPECT_NORETURN(TokenKind::TK_RPRN);
    call->call.args = args_head;
    return call;
  }
  default:
    PANICF("expected an expression in led got: %s", token_kind_to_str(op.kind));
  }
}

Node *Parser::parse_expr(Prec prec) {
  Node *left = parse_nud();
  if (!left)
    return nullptr;

  while (prec < token_prec(peek().kind)) {
    Token op = consume_token();
    left = parse_led(left, op);
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
  }

  Node *expr = parse_expr();
  EXPECT_NORETURN(TokenKind::TK_SEMICOLON);

  Node *stmt = PushStructZero(arena_, Node);
  stmt->kind = NodeKind::STMT_EXPR;
  stmt->binary.lhs = expr; // repurposed for simplicity
  return stmt;
}

Node *Parser::parse_params() {
  Token param_tok = EXPECT_RETURN(TokenKind::TK_ID);
  EXPECT_NORETURN(TokenKind::TK_COLON);

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
    PANICF("Unknown Type: '%s'", token_kind_to_str(peek().kind));
  }
}

u32 Parser::parse_type() {
  switch (peek().kind) {
  case TokenKind::TK_I32:
  case TokenKind::TK_STRING:
  case TokenKind::TK_ID:
    return consume_token().symbol_id;

  default:
    PANICF("Unknown Type: '%s'", token_kind_to_str(peek().kind));
  }
}

Node *Parser::parse_decl() {

  switch (peek().kind) {
  case TokenKind::TK_LET: {
    EXPECT_NORETURN(TokenKind::TK_LET);
    Token id_tok = EXPECT_RETURN(TokenKind::TK_ID);

    if (!match(TokenKind::TK_COLON))
      PANICF("expected ':' after variable declaration");

    u32 type_id = parse_type();

    Node *init_expr = nullptr;
    if (match(TokenKind::TK_ASSIGN))
      init_expr = parse_expr();
    EXPECT_NORETURN(TokenKind::TK_SEMICOLON);

    Node *decl = PushStructZero(arena_, Node);
    decl->kind = NodeKind::DECL_VAR;
    decl->span = id_tok.span;
    decl->var_decl.init = init_expr;
    decl->var_decl.name_id = id_tok.symbol_id;
    decl->var_decl.type_id = type_id;
    return decl;
  }

  case TokenKind::TK_FN: {
    EXPECT_NORETURN(TokenKind::TK_FN);
    Token id_tok = EXPECT_RETURN(TokenKind::TK_ID);

    Node *fn = PushStructZero(arena_, Node);

    if (peek().kind != TokenKind::TK_LPRN)
      PANICF("expected '(' after (%.*s) fn declaration",
             SV(interner_, id_tok.symbol_id));
    EXPECT_NORETURN(TokenKind::TK_LPRN);

    NodeList *head = nullptr;
    NodeList **tail = &head;

    if (peek().kind != TokenKind::TK_RPRN) {
      do {

        Node *param = parse_params();
        NodeList *elem = PushStructZero(arena_, NodeList);
        elem->node = param;
        *tail = elem;
        tail = &elem->next;

      } while (match(TokenKind::TK_COMMA));
    }
    EXPECT_NORETURN(TokenKind::TK_RPRN);
    EXPECT_NORETURN(TokenKind::TK_COLON);
    u32 type_id = parse_type();

    Node *body = parse_block();
    if (!body)
      PANICF("invalid fn body: (%.*s)", SV(interner_, id_tok.symbol_id));

    fn->kind = NodeKind::DECL_FN;
    fn->span = id_tok.span;
    fn->fn_decl.name_id = id_tok.symbol_id;
    fn->fn_decl.type_id = type_id;
    fn->fn_decl.params = head;
    fn->fn_decl.body = body;
    return fn;
  }

  default:
    PANICF("unknown top declaration (%s)", token_kind_to_str(peek().kind));
  }
}

Node *Parser::parse_block() {
  NodeList *head = PushStructZero(arena_, NodeList);
  NodeList **tail = &head;

  EXPECT_NORETURN(TokenKind::TK_LBRACE);
  while (peek().kind != TokenKind::TK_EoF && peek().kind != TokenKind::TK_RBRACE) {
    Node *stmts = parse_stmt();
    if (!stmts)
      PANICF("invalid stmt");

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
  if (!cond)
    PANICF("expected condition after while statmenet");

  Node *body = parse_block();
  if (!body)
    PANICF("expected while body after cond");

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
  if (!cond)
    PANICF("expected condition after if statmenet");

  Node *then_body = parse_block();
  if (!then_body)
    PANICF("expected if body after cond");

  Node *if_node = PushStructZero(arena_, Node);

  if (peek().kind == TokenKind::TK_ELSE) {
    Token else_tok = consume_token();
    if (peek().kind == TokenKind::TK_IF) {
      if_node->if_stmt.else_body = parse_if();
    } else {
      if_node->if_stmt.else_body = parse_block();
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
    if (!decls)
      break;

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

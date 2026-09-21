#pragma once

#include "../core/arena.hpp"
#include "../lexer/span.hpp"
#include "../lexer/token.hpp"

#define _DECLS_                                                                \
  X(DECL)                                                                   \
  X(DECL_VAR)                                                                  \
  X(DECL_FN)                                                                   \
  X(DECL_PARAM)

#define _STMTS_                                                                \
  X(STMT_EXPR)                                                                 \
  X(STMT_RET)                                                                  \
  X(STMT_BLOCK)                                                                \
  X(STMT_IF)                                                                   \
  X(STMT_WHILE)

#define _EXPR_                                                                 \
  X(EXPR_INT_LITERAL)                                                          \
  X(EXPR_STR_LITERAL)                                                          \
  X(EXPR_IDENT)                                                                \
  X(EXPR_UNARY)                                                                \
  X(EXPR_BINARY)                                                               \
  X(EXPR_CALL)

enum class NodeKind {
#define X(node) node,
  _DECLS_ _STMTS_ _EXPR_
#undef X
};

static ccstr node_kind_to_str(NodeKind kind) {
  switch (kind) {
#define X(token)                                                               \
  case NodeKind::token:                                                        \
    return #token;
    _DECLS_ _STMTS_ _EXPR_
  }
#undef X
  return "unknown NodeKind";
}


struct Node;

struct NodeList {
  Node *node;
  NodeList *next;
};

enum class TypeKind {
  I32,
  Str,
  Invalid,
};

struct Node {
  NodeKind kind;
  Span span;

  union {
    u64 int_val;
    // id and str
    u32 symbol_id;

    struct {
      NodeList *decls;
    } declaration;

    struct {
      u32 name_id;
      u32 type_id;
      TypeKind type = TypeKind::Invalid;
      Node *init;
    } var_decl;

    struct {
      u32 name_id;
      u32 type_id;
      TypeKind type = TypeKind::Invalid;
      NodeList *params;
    } params;

    struct {
      u32 name_id;
      u32 type_id;
      TypeKind retType = TypeKind::Invalid;
      NodeList *params;
      Node *body;
    } fn_decl;

    struct {
      NodeList *stmts;
    } block_stmt;

    struct {
      Node *cond;
      Node *then_body;
      Node *else_body;
    } if_stmt;

    struct {
      Node *cond;
      Node *body;
    } while_stmt;

    struct {
      Node *expr;
    } ret_stmt;

    struct {
      Node* expr;
    } expr_stmt;
    
    struct {
      TokenKind op;
      Node *lhs;
      Node *rhs;
    } binary;

    struct {
      TokenKind op;
      Node *operand;
    } unary;

    struct {
      Node *callee;
      NodeList *args;
      int size;
    } call;
  };
};

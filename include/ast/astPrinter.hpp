#pragma once

#include "../lexer/interner.hpp"
#include "ast.hpp"
#include <cstdio>

struct ASTPrinter {
private:
  const Interner &interner_;

  void print_indent(int depth) const {
    for (int i = 0; i < depth; ++i) {
      std::printf(" ");
    }
  }

public:
  explicit ASTPrinter(const Interner &interner) : interner_(interner) {}

  void print(const Node *node, int depth = 0);
};

void ASTPrinter::print(const Node *node, int depth) {
  if (!node) {
    print_indent(depth);
    std::printf("<null>\n");
    return;
  }

  printf("[%d:%d:%zu]", node->span.loc.line, node->span.loc.col,
         node->span.len);
  print_indent(depth);

  switch (node->kind) {
  // declaration
  case NodeKind::DECL_VAR: {
    std::printf("VarDecl(name: '%.*s' | type: '%.*s')\n",
                SV(interner_, node->var_decl.name_id), SV(interner_, node->var_decl.type_id));
    if (node->var_decl.init) {
      print_indent(depth + 1);
      std::printf("init:\n");
      print(node->var_decl.init, depth + 2);
    }
    break;
  }
  case NodeKind::DECL_FN: {
    std::printf("FnDecl(name: '%.*s' | type: '%.*s')\n",
                SV(interner_, node->fn_decl.name_id),
                SV(interner_, node->fn_decl.type_id));
    if (node->fn_decl.params) {
      print_indent(depth + 1);
      std::printf("Args:\n");
      for (const NodeList *curr = node->fn_decl.params; curr; curr = curr->next) {
        print(curr->node, depth + 2);
      }
    }

    if (node->fn_decl.body) {
      print_indent(depth + 1);
      std::printf("body:\n");
      print(node->fn_decl.body, depth + 2);
    }
    break;
  }

  case NodeKind::DECL_PARAM: {
    std::printf("ParamDecl(name: '%.*s' | type: '%.*s')\n",
                SV(interner_, node->params.name_id), SV(interner_, node->params.type_id));
  
    break;
  }

  // statements
  case NodeKind::STMT_EXPR: {
    std::printf("ExprStmt\n");
    print(node->binary.lhs, depth + 1);
    break;
  }

  case NodeKind::DECL: {
    std::printf("Decl\n");
    for(const NodeList* curr = node->declaration.decls; curr; curr = curr->next)
      print(curr->node, depth + 1);  
    break;
  }
  
  case NodeKind::STMT_BLOCK: {

    std::printf("BlockStmt\n");
    for (const NodeList *curr = node->block_stmt.stmts; curr != nullptr;
         curr = curr->next)
      print(curr->node, depth + 1);
    break;
  }

  case NodeKind::STMT_IF: {
    std::printf("IfStmt\n");
    print_indent(depth + 1);
    std::printf("cond:\n");
    print(node->if_stmt.cond, depth + 2);
    print_indent(depth + 1);
    std::printf("body:\n");
    print(node->if_stmt.then_body, depth + 2);

    if (node->if_stmt.else_body) {
      print_indent(depth + 1);
      std::printf("ElseBody:\n");
      print(node->if_stmt.else_body, depth + 2);
    }
    break;
  }

  case NodeKind::STMT_WHILE: {
    std::printf("WhileStmt\n");
    print_indent(depth + 1);
    std::printf("cond:\n");
    print(node->while_stmt.cond, depth + 2);
    print_indent(depth + 1);
    std::printf("body:\n");
    print(node->while_stmt.body, depth + 2);
    break;
  }

  case NodeKind::STMT_RET: {
    std::printf("RetStmt:\n");
    print(node->ret_stmt.expr, depth + 1);
    break;
  }

    // Expressions
  case NodeKind::EXPR_INT_LITERAL: {
    std::printf("IntLit(%llu)\n",
                static_cast<unsigned long long>(node->int_val));
    break;
  }

  case NodeKind::EXPR_STR_LITERAL: {
    std::printf("StrLit(\"%.*s\")\n", SV(interner_, node->symbol_id));
    break;
  }

  case NodeKind::EXPR_IDENT: {
    std::printf("Ident('%.*s')\n", SV(interner_, node->symbol_id));
    break;
  }

  case NodeKind::EXPR_UNARY: {
    std::printf("UnaryExpr(op: '%s')\n", token_kind_to_str(node->unary.op));
    print(node->unary.operand, depth + 1);
    break;
  }

  case NodeKind::EXPR_BINARY: {
    std::printf("BinaryExpr(op: '%s')\n", token_kind_to_str(node->binary.op));
    print(node->binary.lhs, depth + 1);
    print(node->binary.rhs, depth + 1);
    break;
  }

  case NodeKind::EXPR_CALL: {
    std::printf("CallExpr\n");
    print_indent(depth + 1);
    std::printf("Callee:\n");
    print(node->call.callee, depth + 2);

    print_indent(depth + 1);
    std::printf("Args:\n");
    for (const NodeList *curr = node->call.args; curr != nullptr;
         curr = curr->next) {
      print(curr->node, depth + 2);
    }
    break;
  }
  }
}

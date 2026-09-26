#include "../../include/sema/sema.hpp"
#include "../../include/diagnostic/diag.hpp"
#include <string>

static Option<ccstr> type_kind_to_str(TypeKind kind) {
  switch (kind) {
  case TypeKind::I32:
    return Option<ccstr>::some("i32");
  case TypeKind::Str:
    return Option<ccstr>::some("str");
  case TypeKind::Invalid:
  default:
    return Option<ccstr>::none();
  }
}

auto Sema::resolve_type_name(u32 id) -> TypeKind {
  auto ident = interner_.view(id);

  SWITCH(ident) DO CASE("i32") DO return TypeKind::I32;
  END CASE("string") DO return TypeKind::Str;
  END DEFAULT DO return TypeKind::Invalid;
  END // no custom types for now
      END
}

bool Sema::collect_symbols() {
  NodeList *head = node_.declaration.decls;

  auto collect_var = [&](Node *node) -> auto {
    if (scopes.find_sym(node->var_decl.name_id).is_some()) {
      diag_.push_diag(
          {.severity = SeverityKind::Err,
           .span = node->span,
           .err_msg = "symbol '" +
                      (string)interner_.view(node->var_decl.name_id) +
                      "' already declared before"});
      return false;
    }
    Symbol sym;
    sym.kind = SymbolKind::SYM_VAR;
    sym.scopeKind = ScopeKind::Global;
    VarSymbol var_sym;
    var_sym.name = node->var_decl.name_id;
    var_sym.type = resolve_type_name(node->var_decl.type_id);
    node->var_decl.type = var_sym.type;
    if (var_sym.type == TypeKind::Invalid) {
      diag_.push_diag(
          {.severity = SeverityKind::Err,
           .span = node->span,
           .err_msg = "invalid variable '" +
                      (string)interner_.view(node->var_decl.name_id) +
                      "' type"});
      return false;
    }
    if (node->var_decl.init)
      var_sym.expr =
          node->var_decl.init; // if not init we check if it was init-ed later
    sym.sym = var_sym;
    scopes.get_scope().emplace(var_sym.name, sym);
    return true;
  };

  while (head) {
    if (head->node->kind == NodeKind::DECL_VAR) {
      collect_var(head->node);
    }

    if (head->node->kind == NodeKind::DECL_FN) {
      if (scopes.find_sym(head->node->fn_decl.name_id).is_some()) {
        diag_.push_diag(
            {.severity = SeverityKind::Err,
             .span = head->node->span,
             .err_msg = "symbol '" +
                        (string)interner_.view(head->node->fn_decl.name_id) +
                        "' already declared before"});
        return false;
      }
      Symbol sym;
      sym.kind = SymbolKind::SYM_FN;
      sym.scopeKind = ScopeKind::Global;
      FnSymbol fn_sym;
      fn_sym.name = head->node->fn_decl.name_id;
      fn_sym.ret_type = resolve_type_name(head->node->fn_decl.type_id);
      if (fn_sym.ret_type == TypeKind::Invalid) {
        diag_.push_diag(
            {.severity = SeverityKind::Err,
             .span = head->node->span,
             .err_msg = "invalid fn '" +
                        (string)interner_.view(head->node->fn_decl.name_id) +
                        "' return type"});
        return false;
      }

      NodeList *param = head->node->fn_decl.params;
      while (param) {
        TypeKind param_type = resolve_type_name(param->node->params.type_id);
        if (param_type == TypeKind::Invalid) {
          diag_.push_diag(
              {.severity = SeverityKind::Err,
               .span = param->node->span,
               .err_msg = "invalid parameter '" +
                          (string)interner_.view(param->node->params.name_id) +
                          "' type"});
          return false;
        }
        fn_sym.params.push_back(param_type);
        param->node->params.type = param_type;
        param = param->next;
      }

      fn_sym.fn = head->node;
      sym.sym = fn_sym;
      scopes.get_scope().emplace(fn_sym.name, sym);
    }

    head = head->next;
  }
  return true;
}

bool Sema::sema_prog() {
  NodeList *head = node_.declaration.decls;
  while (head) {

    auto cur_node = head->node;
    if (cur_node->kind == NodeKind::DECL_VAR)
      if (!sema_var(cur_node))
        return false;

    if (cur_node->kind == NodeKind::DECL_FN)
      if (!sema_fn(cur_node))
        return false;

    head = head->next;
  }
  return 1;
}

TypeKind Sema::sema_expr(Node *node) {
  switch (node->kind) {
  case NodeKind::EXPR_INT_LITERAL: {
    // TODO: bounds check
    return TypeKind::I32;
  }
  case NodeKind::EXPR_STR_LITERAL:
    return TypeKind::Str;

  case NodeKind::EXPR_IDENT: {
    auto var_sym = scopes.find_sym(node->symbol_id);

    if (var_sym.is_none()) {
      diag_.push_diag(
          {.severity = SeverityKind::Err,
           .span = node->span,
           .err_msg = "symbol isn't defined '" +
                      (string)interner_.view(node->var_decl.name_id) + "'"});
      return TypeKind::Invalid;
    }

    switch (var_sym.value().kind) {
    case SymbolKind::SYM_VAR:
      return std::get<VarSymbol>(var_sym.value().sym).type;
    case SymbolKind::SYM_FN:
      return std::get<FnSymbol>(var_sym.value().sym).ret_type;
    case SymbolKind::SYM_PARAM:
      return std::get<ParamSymbol>(var_sym.value().sym).type;
    default:
      diag_.push_diag({.severity = SeverityKind::Err,
                       .span = node->span,
                       .err_msg = "Unknown EXPR_IDENT"});
      return TypeKind::Invalid;
    }
  }

  case NodeKind::EXPR_UNARY: {
    auto type = sema_expr(node->unary.operand);
    if (type == TypeKind::Invalid)
      return TypeKind::Invalid;

    if (type != TypeKind::I32)
      return TypeKind::Invalid;

    if (node->unary.op == TokenKind::TK_MINUS) {
      return TypeKind::I32;
    }

    return TypeKind::Invalid;
  }
  case NodeKind::EXPR_BINARY: {
    auto lhs_type = sema_expr(node->binary.lhs);
    auto rhs_type = sema_expr(node->binary.rhs);
    if (lhs_type == TypeKind::Invalid || rhs_type == TypeKind::Invalid)
      return TypeKind::Invalid;

    if (lhs_type != rhs_type)
      return TypeKind::Invalid;

    return lhs_type;
  }

  case NodeKind::EXPR_CALL: {
    // Id ( params | variadic) -> returns;
    auto it = scopes.find_sym(node->call.callee->symbol_id);
    if (it.is_none()) {
      diag_.push_diag(
          {.severity = SeverityKind::Err,
           .span = node->span,
           .err_msg = "symbol isn't defined '" +
                      (string)interner_.view(node->call.callee->symbol_id) +
                      "'"});
      return TypeKind::Invalid;
    }

    auto fn = std::get<FnSymbol>(it.value().sym);
    if (fn.params.size() != node->call.size) {
      diag_.push_diag(
          {.severity = SeverityKind::Err,
           .span = node->span,
           .err_msg = "argument count of '" +
                      (string)interner_.view(node->call.callee->symbol_id) +
                      "' differs from parameter count"});
      return TypeKind::Invalid;
    }

    auto args = node->call.args;
    int i = 0;
    while (args && i < node->call.size) {
      TypeKind arg_type = sema_expr(args->node);
      if (arg_type != fn.params[i]) {
        diag_.push_diag(
            {.severity = SeverityKind::Err,
             .span = node->span,
             .err_msg = "mismatch type of argument (" + std::to_string(i) +
                        ") of '" +
                        (string)interner_.view(node->call.callee->symbol_id) +
                        "' expected '" +
                        (string)type_kind_to_str(fn.params[i])
                            .value_or("invalid type") +
                        "'"});
        return TypeKind::Invalid;
      }
      // PANICF("arguemnt('%d') type mismatch, expected '%s' got '%s'", i,
      //        type_kind_to_str(fn.params[i]), type_kind_to_str(arg_type));

      args = args->next;
    }

    return fn.ret_type;
  }

  default:
    diag_.push_diag({.severity = SeverityKind::Err,
                     .span = node->span,
                     .err_msg = "Unknown expression '" +
                                (string)node_kind_to_str(node->kind) + "'"});
    return TypeKind::Invalid;
    // PANICF("unknown expression %s", node_kind_to_str(node->kind));
  }
}

Result<Flow, SemaErr> Sema::sema_stmt(Node *node) {
  if (!node) {
    return Result<Flow, SemaErr>::ok(Flow::Continue);
  }

  switch (node->kind) {
  case NodeKind::STMT_BLOCK:
    return sema_block(node);
  case NodeKind::STMT_EXPR:
    if (sema_expr(node->expr_stmt.expr) == TypeKind::Invalid) {
      diag_.push_diag({.severity = SeverityKind::Err,
                       .span = node->span,
                       .err_msg = "invalid type"});
      return Result<Flow, SemaErr>::err(SemaErr{
          .kind = SemaErrKind::INVALID_TYPE, .err_msg = "invalid type"});
    }
    return Result<Flow, SemaErr>::ok(Flow::Continue);

  case NodeKind::DECL_VAR: {
    // auto sym = scopes.find_recent_sym(node->var_decl.name_id);
    // if (sym.is_some()) {
    //   diag_.push_diag(
    //       {.severity = SeverityKind::Err,
    //        .span = node->span,
    //        .err_msg = "symbol '" +
    //                   (string)interner_.view(node->var_decl.name_id) +
    //                   "' already defined"});
    //   return Result<Flow, SemaErr>::err(
    //       SemaErr{.kind = SemaErrKind::SYMBOL_ALREADY_DECLARED});
    // }
    // // PANICF("symbol already defined '%.*s'",
    // //        SV(interner_, node->var_decl.name_id));

    // node->var_decl.type = resolve_type_name(node->var_decl.type_id);

    // scopes.get_scope().emplace(
    //     node->var_decl.name_id,
    //     Symbol{.kind = SymbolKind::SYM_VAR,
    //            .sym = VarSymbol{.name = node->var_decl.name_id,
    //                             .type = node->var_decl.type,
    //                             .expr = node->var_decl.init}});
    // // we dont assign statement for now, so it must be inited upon
    // declaration if (!node->var_decl.init) {
    //   diag_.push_diag(
    //       {.severity = SeverityKind::Err,
    //        .span = node->span,
    //        .err_msg = "symbol '" +
    //                   (string)interner_.view(node->var_decl.name_id) +
    //                   "' must be inited upon declaration"});
    //   return Result<Flow, SemaErr>::err(
    //       SemaErr{.kind = SemaErrKind::SYMBOL_ALREADY_DECLARED});
    // }

    // // PANICF("symbol '%.*s' must be inited upon declaration",
    // //        SV(interner_, node->var_decl.name_id));
    // auto expr_type = sema_expr(node->var_decl.init);
    // if (expr_type != node->var_decl.type) {
    //   diag_.push_diag(
    //       {.severity = SeverityKind::Err,
    //        .span = node->span,
    //        .err_msg = "cannot assgin var of type '" +
    //                   (string)type_kind_to_str(node->var_decl.type)
    //                       .value_or("invalid type") +
    //                   " to expr of type " +
    //                   type_kind_to_str(expr_type).value_or("invalid type")});
    //   return Result<Flow, SemaErr>::err(
    //       SemaErr{.kind = SemaErrKind::SYMBOL_ALREADY_DECLARED});
    // }
    // // PANICF("cannot assign var of type %s to expr of type %s",
    // //        type_kind_to_str(node->var_decl.type),
    // //        type_kind_to_str(expr_type));
    if (sema_var(node))
      return Result<Flow, SemaErr>::ok(Flow::Continue);
    else
      return Result<Flow, SemaErr>::err(
          SemaErr{.kind = SemaErrKind::SYMBOL_ALREADY_DECLARED});
  }

  /*
   * if cond
   *   BODY
   */
  case NodeKind::STMT_IF: {
    TypeKind cond = sema_expr(node->if_stmt.cond);
    if (cond == TypeKind::Invalid) {
      diag_.push_diag(
          {.severity = SeverityKind::Err,
           .span = node->span,
           .err_msg = "invalid if statement condition type '",
           .note = "condition type can only be of type i32 for now"});
      return Result<Flow, SemaErr>::err(SemaErr{
          .kind = SemaErrKind::INVALID_TYPE, .err_msg = "invalid type"});
    }
    if (cond != TypeKind::I32) {
      diag_.push_diag(
          {.severity = SeverityKind::Err,
           .span = node->span,
           .err_msg = "type mismatch got '" +
                      (string)type_kind_to_str(cond).value_or("invalid type")});
      return Result<Flow, SemaErr>::err(SemaErr{
          .kind = SemaErrKind::TYPE_MISMATCH,
          .err_msg = "type mismatch got " +
                     (string)type_kind_to_str(cond).value_or("invalid type")});
    }

    auto then_block = sema_block(node->if_stmt.then_body);
    if (then_block.is_err())
      return then_block;

    if (!node->if_stmt.else_body && then_block.value() == Flow::Return)
      return Result<Flow, SemaErr>::ok(Flow::Return);
    else if (!node->if_stmt.else_body)
      return Result<Flow, SemaErr>::ok(Flow::Continue);

    auto else_body = sema_stmt(node->if_stmt.else_body);
    if (else_body.is_err())
      return else_body;

    if (then_block.value() == Flow::Return && else_body.value() == Flow::Return)
      return Result<Flow, SemaErr>::ok(Flow::Return);

    return Result<Flow, SemaErr>::ok(Flow::Continue);
  }

  case NodeKind::STMT_WHILE: {
    TypeKind cond = sema_expr(node->while_stmt.cond);
    if (cond == TypeKind::Invalid) {
      diag_.push_diag(
          {.severity = SeverityKind::Err,
           .span = node->span,
           .err_msg = "invalid type got '" +
                      (string)type_kind_to_str(cond).value_or("invalid type")});

      return Result<Flow, SemaErr>::err(SemaErr{
          .kind = SemaErrKind::INVALID_TYPE,
          .err_msg = "invalid type got " +
                     (string)type_kind_to_str(cond).value_or("invalid type")});
    }

    if (cond != TypeKind::I32) {
      diag_.push_diag(
          {.severity = SeverityKind::Err,
           .span = node->span,
           .err_msg = "type mismatch got '" +
                      (string)type_kind_to_str(cond).value_or("invalid type")});
      return Result<Flow, SemaErr>::err(SemaErr{
          .kind = SemaErrKind::INVALID_TYPE,
          .err_msg = "type mismatch got " +
                     (string)type_kind_to_str(cond).value_or("invalid type")});
    }
    auto block = sema_block(node->while_stmt.body);
    if (block.is_err())
      return block;

    if (block.value() == Flow::Return)
      return Result<Flow, SemaErr>::ok(Flow::Return);
    return Result<Flow, SemaErr>::ok(Flow::Continue);
  }

  // ret expr ;
  // check if expr is same type as ret type of cur fn
  case NodeKind::STMT_RET: {
    auto expr = sema_expr(node->ret_stmt.expr);
    if (expr == TypeKind::Invalid) {
      diag_.push_diag(
          {.severity = SeverityKind::Err,
           .span = node->span,
           .err_msg = "invalid type got '" +
                      (string)type_kind_to_str(expr).value_or("invalid type") +
                      "'"});
      return Result<Flow, SemaErr>::err(SemaErr{
          .kind = SemaErrKind::INVALID_TYPE, .err_msg = "invalid type"});
    }

    auto fn_sym = scopes.find_sym(cur_fn);

    if (expr != std::get<FnSymbol>(fn_sym.value().sym).ret_type) {
      diag_.push_diag(
          {.severity = SeverityKind::Err,
           .span = node->span,
           .err_msg = "ret type mismatch got '" +
                      (string)type_kind_to_str(expr).value_or("invalid type") +
                      "'"});
      return Result<Flow, SemaErr>::err(SemaErr{
          .kind = SemaErrKind::TYPE_MISMATCH,
          .err_msg = "ret type mismatch got " +
                     (string)type_kind_to_str(expr).value_or("invalid type")});
    }

    return Result<Flow, SemaErr>::ok(Flow::Return);
  }

  default:
    diag_.push_diag({.severity = SeverityKind::Err,
                     .span = node->span,
                     .err_msg = "Unknown statement '" +
                                (string)node_kind_to_str(node->kind) + "'"});
    return Result<Flow, SemaErr>::err(
        SemaErr{.kind = SemaErrKind::INVALID_TYPE});
  }
}

Result<Flow, SemaErr> Sema::sema_block(Node *node) {
  scopes.push_scope();

  NodeList *head = node->block_stmt.stmts;
  while (head) {

    auto flow = sema_stmt(head->node);
    if (flow.is_err()) {
      // diag_.push_diag({.severity = SeverityKind::Err, .err_msg = "Unknown
      // statement '" + (string)node_kind_to_str(node->kind) + "'"});
      return Result<Flow, SemaErr>::err(
          SemaErr{.kind = SemaErrKind::INVALID_TYPE});
    }
    // PANICF("%s", flow.error().err_msg.c_str());

    if (flow.value() == Flow::Return) {
      scopes.pop_scope();
      return flow;
    }

    head = head->next;
  }

  scopes.pop_scope();
  return Result<Flow, SemaErr>::ok(Flow::Continue);
}

/*
 * fn id(param | variadic) : RET_TYPE DO
 *  BODY ->
 *  -> RET
 * END
 *
 * we have to check if the ret type expr is compatiable
 *
 */
bool Sema::sema_fn(Node *node) {
  cur_fn = node->fn_decl.name_id;

  // frame scope
  scopes.push_scope();

  // check params
  auto params = node->fn_decl.params;
  while (params) {
    scopes.get_scope().emplace(
        params->node->params.name_id,
        Symbol{.kind = SymbolKind::SYM_PARAM,
               .scopeKind = ScopeKind::Param,
               .sym = ParamSymbol{.name_id = params->node->params.name_id,
                                  .type = params->node->params.type}});
    params = params->next;
  }

  NodeList *head = node->fn_decl.body->block_stmt.stmts;
  bool has_return = false;
  while (head) {
    auto flow = sema_stmt(head->node);
    if (flow.is_err())
      return false;
    // PANICF("%s", flow.error().err_msg.c_str());
    if (flow.value() == Flow::Return) {
      has_return = true;
      break;
    }
    head = head->next;
  }

  scopes.pop_scope();

  if (!has_return) {
    diag_.push_diag({.severity = SeverityKind::Err,
                     .span = node->span,
                     .err_msg = "fn '" +
                                (string)interner_.view(node->fn_decl.name_id) +
                                "' can reach its end without returning"});
    return false;
  }
  // PANICF("fn can reach its end without returning");

  return true;
}

// let var: type = expr;
// we have to check its type is compatiable with its expr
// we have to check its init
bool Sema::sema_var(Node *node) {

  u32 name_id = node->var_decl.name_id;

  auto global_sym = scopes.get_global_scope();
  auto global_it = global_sym.find(name_id);
  if (global_it != global_sym.end() && scopes.scopes_.size() == 1) {
    // we dont have assign statement for now, so it must be inited upon
    // declaration
    if (!node->var_decl.init) {
      diag_.push_diag(
          {.severity = SeverityKind::Err,
           .span = node->span,
           .err_msg = "symbol '" +
                      (string)interner_.view(node->var_decl.name_id) +
                      "' must be inited upon declaration"});
      return false;
    }
    // PANICF("symbol '%.*s' must be inited upon declaration",
    //        SV(interner_, node->var_decl.name_id));
    auto expr_type = sema_expr(node->var_decl.init);
    if (expr_type != node->var_decl.type) {
      diag_.push_diag(
          {.severity = SeverityKind::Err,
           .span = node->span,
           .err_msg = "cannot assign var of type '" +
                      (string)type_kind_to_str(node->var_decl.type)
                          .value_or("invalid type") +
                      "' to expr of type '" +
                      type_kind_to_str(expr_type).value_or("invalid type") +
                      "'"});
      return false;
    }
    return true;
  }

  auto local_sym = scopes.find_recent_sym(name_id);
  if (local_sym.is_some()) {
    diag_.push_diag({.severity = SeverityKind::Err,
                     .span = node->span,
                     .err_msg = "symbol already declared '" +
                                (string)interner_.view(name_id) +
                                "' in this scope"});
    return false;
  }

  node->var_decl.type = resolve_type_name(node->var_decl.type_id);
  if (node->var_decl.type == TypeKind::Invalid) {
    diag_.push_diag({.severity = SeverityKind::Err,
                     .span = node->span,
                     .err_msg = "invalid type for variable '" +
                                (string)interner_.view(name_id) + "'"});
    return false;
  }

  scopes.get_scope().emplace(
      node->var_decl.name_id,
      Symbol{.kind = SymbolKind::SYM_VAR,
             .scopeKind = ScopeKind::Local,
             .sym = VarSymbol{.name = node->var_decl.name_id,
                              .type = node->var_decl.type,
                              .expr = node->var_decl.init}});

  if (!node->var_decl.init) {
    diag_.push_diag({.severity = SeverityKind::Err,
                     .span = node->span,
                     .err_msg = "symbol '" + (string)interner_.view(name_id) +
                                "' must be inited upon declaration"});
    return false;
  }
  auto expr_type = sema_expr(node->var_decl.init);
  if (expr_type != node->var_decl.type) {
    diag_.push_diag(
        {.severity = SeverityKind::Err,
         .span = node->span,
         .err_msg = "cannot assign var of type '" +
                    (string)type_kind_to_str(node->var_decl.type)
                        .value_or("invalid type") +
                    "' to expr of type '" +
                    type_kind_to_str(expr_type).value_or("invalid type") +
                    "'"});
    return false;
  }
  return true;
}

bool Sema::analyze() {
  auto root = node_;

  scopes.scopes_.clear();
  scopes.push_scope();

  if (!collect_symbols()){
    scopes.pop_scope();
    return false;
  }

  u32 main_id = interner_.intern("main");
  auto main_sym = scopes.find_sym(main_id);
  if (main_sym.is_none()) {
    diag_.push_diag({.severity = SeverityKind::Err,
                     .span = root.span,
                     .err_msg = "symbol 'main' not found"});
    scopes.pop_scope();
    return false;
  }

  if (main_sym.value().kind != SymbolKind::SYM_FN) {
    diag_.push_diag({.severity = SeverityKind::Err,
                     .span = std::get<FnSymbol>(main_sym.value().sym).fn->span,
                     .err_msg = "symbol 'main' should be a function"});
    scopes.pop_scope();
    return false;
  }

  if (std::get<FnSymbol>(main_sym.value().sym).ret_type != TypeKind::I32) {
    diag_.push_diag({.severity = SeverityKind::Err,
                     .span = std::get<FnSymbol>(main_sym.value().sym).fn->span,
                     .err_msg = "symbol 'main' return type should be 'i32'"});
    scopes.pop_scope();
    return false;
  }

  if (!sema_prog()){
    scopes.pop_scope();
    return false;
  }

  scopes.pop_scope();

  return true;
}

#pragma once

#include "../ast/ast.hpp"
#include "../lexer/interner.hpp"
#include "../core/result.hpp"
#include "../core/option.hpp"
#include <cstdint>
#include <unordered_map>
#include <variant>
#include <vector>

enum class SymbolKind {
  SYM_VAR,
  SYM_FN,
  INVALID,
};

template<typename T>
struct List {
  List* next;
  T val;
};

/*
* fn id (params | variadic) scope_begin
*   body -> local symbol
* scope_end
*/
struct FnSymbol {
  TypeKind ret_type = TypeKind::Invalid;

  vec<TypeKind> params;
  u32 name = UINT32_MAX;
  Node* fn = nullptr;
    
  bool is_variadic = false;
};

/*
* let id: type = expr;
*/
struct VarSymbol {
  u32 name = UINT32_MAX;
  TypeKind type = TypeKind::Invalid;
  Node* expr = nullptr;
};

struct Symbol {
  SymbolKind kind = SymbolKind::INVALID;
  variant<std::monostate, VarSymbol, FnSymbol> sym;
};

enum class Flow {
  Continue,
  Return,
};

enum class SemaErrKind {
  /* type errors */
  INVALID_TYPE,
  TYPE_MISMATCH,
  
  UNDECL_MAIN,
  UNINIT_VAR,
  UNDECLARED_FN,
};

struct SemaErr {
  SemaErrKind kind;
  string err_msg;
};

struct Scope {
  private:
  std::vector<std::unordered_map<u32, Symbol>> scopes_;
  public:

  auto push_scope() -> void {
    scopes_.push_back({});
  }

  auto pop_scope() -> void {
    scopes_.pop_back();
  }

  auto get_scope() -> std::unordered_map<u32, Symbol> {
    return scopes_.back();
  }

  auto find_recent_sym(u32 id) -> Option<Symbol> {
    auto last_scope = get_scope();
    auto it = last_scope.find(id);
    if(it == last_scope.end())
      return Option<Symbol>::none();
    return Option<Symbol>::some(it->second);
  }

  auto find_sym(u32 id) -> Option<Symbol> {
    for(auto& s : scopes_){
      auto it = s.find(id);
      if(it != s.end())
        return Option<Symbol>::some(it->second);
    }
    return Option<Symbol>::none();
  }
  
};

struct Sema {
  private:
    Interner& interner_;
  public:
    explicit Sema(Interner* interner, Node* node) : interner_(*interner), node_(*node) {}

  Node& node_;
  Scope scopes;
  std::unordered_map<u32, Symbol> symbols;
  u32 cur_fn; // maybe we need just the name ?
  
  auto analyze() -> bool;

  auto collect_symbols() -> bool;

  auto sema_prog() -> bool; 
  auto resolve_type_name(u32 id) -> TypeKind;
  
  /* decl */
  auto sema_fn(Node* node) -> bool;
  auto sema_var(Node* node) -> bool;  

  /* stmt */  
  auto sema_block(Node* node) -> Result<Flow, SemaErr>;  
  auto sema_stmt(Node* node) -> Result<Flow, SemaErr>;

  /* expr */
  auto sema_expr(Node* node) -> TypeKind; // TODO: consider changing to Result<TypeKind, SemaErr>
};

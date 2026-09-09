#pragma once

#include "macros.hpp"
#include "types.hpp"
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <sys/types.h>

#ifndef ARENA_ALIGNMENT
#define ARENA_ALIGNMENT 8
#endif

#ifndef ARENA_CHUNK_CAP
#define ARENA_CHUNK_CAP (64 * 1024) // KiB(64)
#endif

#ifndef ARENA_INIT_CAP
#define ARENA_INIT_CAP 256
#endif

#define ARENA_ALIGNOF(ptr, align) (ptr + (align - 1)) & ~(uintptr_t)(align - 1)

#define PushBytes(arena, bytes) (arena)->alloc((bytes), ARENA_ALIGNMENT)

#define PushBytesZero(arena, bytes)                                            \
  (arena)->alloc_zero((bytes), ARENA_ALIGNMENT)

#define PushArray(arena, type, count)                                          \
  ((type *)(arena)->alloc(sizeof(type) * (count), alignof(type)))

#define PushArrayZero(arena, type, count)                                      \
  ((type *)(arena)->alloc_zero(sizeof(type) * (count), alignof(type)))

#define PushStruct(arena, type) PushArray((arena), type, 1)

#define PushStructZero(arena, type) PushArrayZero((arena), type, 1)

#define SCRATCH_SCOPE(arena_ref) ScratchArena CONCAT_IMPL(_scratch_, __LINE__)(arena_ref)
#define CONCAT_IMPL(x, y) CONCAT_INNER(x, y)
#define CONCAT_INNER(x, y) x ## y

inline void *xmalloc(usize size) {
  void *p = std::malloc(size);
  assert(p && "xmalloc failed: out of memory");
  return p;
}

struct ArenaChunk {
  ArenaChunk *next{nullptr};
  ssize cap{0};
  ssize used{0};
  // flexable array / raw buffer payload start directly after metadata
  char data[];
};

struct ArenaMarker {
  ArenaChunk* chunk{nullptr};
  ssize used{0};
};

struct Arena {
private:
  ArenaChunk *current_{nullptr};
  ssize default_chunk_cap_{ARENA_CHUNK_CAP};

  static uintptr_t align_address(uintptr_t address, ssize alignment) noexcept {
    ASSERT(alignment > 0);
    ASSERT((alignment & (alignment - 1)) == 0 &&
           "alignment must be a power of 2");
    return ARENA_ALIGNOF(address, alignment);
  }

public:
  explicit Arena(ssize default_cap = ARENA_CHUNK_CAP)
      : default_chunk_cap_(ARENA_CHUNK_CAP) {}

  ~Arena() { destory_all(); }

  Arena(const Arena &) = delete;
  Arena &operator=(const Arena &) = delete;

  Arena(Arena &&other) noexcept
      : current_(other.current_), default_chunk_cap_(other.default_chunk_cap_) {
    other.current_ = nullptr;
  }
  Arena &operator=(Arena &&other) noexcept {
    if (this != &other) {
      destory_all();
      current_ = other.current_;
      default_chunk_cap_ = other.default_chunk_cap_;
      other.current_ = nullptr;
    }
    return *this;
  }

  [[nodiscard]] ArenaMarker get_marker() const noexcept {
    return ArenaMarker{current_, current_ ? current_->used : 0};
  }

  void reset_to_marker(ArenaMarker marker) noexcept {
    if(!marker.chunk){
      for(ArenaChunk* chunk = current_; chunk; chunk = chunk->next)
        chunk->used = 0;
    return;
    }

    ArenaChunk* chunk = current_;
    while(chunk && chunk != marker.chunk){
      chunk->used = 0;
      chunk = chunk->next;
    }

    if(marker.chunk){
      ASSERT(marker.used <= marker.chunk->cap);
      marker.chunk->used = marker.used;
      current_ = marker.chunk;
    }
  }

  void *alloc(ssize size, ssize alignment = ARENA_ALIGNMENT) {
    ASSERT(alignment > 0);
    ASSERT((alignment & (alignment - 1)) == 0 && "alignment must be a power of 2");

    if(!current_){
      ssize cap = (size > default_chunk_cap_) ? size : default_chunk_cap_;
      auto* new_chunk = static_cast<ArenaChunk*>(xmalloc(sizeof(ArenaChunk) + cap));
      new_chunk->next = nullptr;
      new_chunk->cap = cap;
      new_chunk->used = 0;
      current_ = new_chunk;
    }
    ArenaChunk* chunk = current_;

    uintptr_t raw_addr = reinterpret_cast<uintptr_t>(chunk->data + chunk->used);
    uintptr_t aligned_addr = align_address(raw_addr, alignment);
    ssize padding = aligned_addr - raw_addr;
    ssize total_required = padding + size;
    if(chunk->used + total_required > chunk->cap){
      ssize next_cap = (size > default_chunk_cap_ ? size : default_chunk_cap_);
      auto* new_chunk = static_cast<ArenaChunk*>(xmalloc(sizeof(ArenaChunk) + next_cap));
      new_chunk->next = current_;
      new_chunk->cap = next_cap;

      uintptr_t fresh_addr = align_address(reinterpret_cast<uintptr_t>(new_chunk->data), aligned_addr);
      ssize fresh_padding = fresh_addr - reinterpret_cast<uintptr_t>(new_chunk->data);
      new_chunk->used = fresh_padding + size;
      current_ = new_chunk;
      return reinterpret_cast<void*>(fresh_addr);
    }

    chunk->used += total_required;
    return reinterpret_cast<void*>(aligned_addr);
  }

  void *alloc_zero(ssize size, ssize alignment = ARENA_ALIGNMENT){
    void* ptr = alloc(size, alignment);
    std::memset(ptr, 0, size);
    return ptr;
  }

  void reset() noexcept {
    for(ArenaChunk* chunk = current_; chunk; chunk = chunk->next)
      chunk->used = 0;
  }

  void destory_all() noexcept {
    ArenaChunk* chunk = current_;
    while(chunk){
      ArenaChunk* next = current_->next;
      free(chunk);
      chunk = next;
    }
    current_ = nullptr;
  }

  [[nodiscard]] ssize capacity() const noexcept { return default_chunk_cap_; }
  [[nodiscard]] ssize get_pos() const noexcept { return current_->used; };
};

struct ScratchArena {
  private:
    Arena* arena_{nullptr};
    ArenaMarker marker_{};
  public:
    explicit ScratchArena(Arena& arena) : arena_(&arena), marker_(arena.get_marker()) {}
    ~ScratchArena(){
      if(arena_)
        arena_->reset_to_marker(marker_);
    }

    ScratchArena(const ScratchArena&) = delete;
    ScratchArena& operator=(const ScratchArena&) = delete;

    // move-only (allows returning a scratch guard from a function if needed)
    ScratchArena(ScratchArena&& other) noexcept : arena_(other.arena_), marker_(other.marker_){
      other.arena_ = nullptr;
    }

    ScratchArena& operator=(ScratchArena&& other) noexcept {
      if(this != &other){
        if(arena_){
          arena_->reset_to_marker(marker_);
        }
        arena_ = other.arena_;
        marker_ = other.marker_;
        other.arena_ = nullptr;
      }
      return *this;
    }

    // yeah whatever fuck 
    [[nodiscard]] Arena* get() const noexcept { return arena_; }
    Arena* operator->() const noexcept { return arena_; }
};

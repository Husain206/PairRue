### syntax 
```rue
fn add(a: i32, b: i32) : i32 {
  ret a + b;
}

fn main(argc: i32, argv: string): i32 {

    let sum: i32 = add(34, 35);
    if(sum == 69)
      ret 0;
    else 
      ret x;
}
```

<br></br>

### those are the topics i come acorss while implmenting the compiler and some of the state of my compiler (! not done)

### monads 
- std::variant
- std::monostate / sentinel state
- semantic wrappers
- deleted constructors
- explicit
- move vs copy
- forwarding references
- std::forward
- std::move
- reference collapsing
- &&-qualified members
- std::invoke_result_t
- map
- and_then
- map_err
- in-place construction

### arena
- RC, copy techneques
  * https://en.wikipedia.org/wiki/Object_copying#Shallow_copy

- overcomitment
  * https://en.wikipedia.org/wiki/Memory_overcommitment

- arena chunk reference
  * https://nukethebees.com/arena-allocators-parser/

### hashmap
- golden ratio
- modular hashing
  * fn1v
- fibonacci hashing
  * https://probablydance.com/2018/06/16/fibonacci-hashing-the-optimization-that-the-world-forgot-or-a-better-alternative-to-integer-modulo/
- seperate chaining(linked lists)
- open addressing

### lexer
  - state splitting desgin
  - a start line index table
    * ! Add SIMD vectorization (AVX2/NEON) to speed up line index creation
  - spans

### parser
  - recursive decent parser `decl` -> `block` -> `stmt` ->
    * no assign stmt yet, no refs, ptrs or arrays yet, no product or sum types
  - pratt parser `expr`
  - context-free grammer

### sema
  - symbol table
    * !collect param symbols at collect_sym()
  - name resolution
  - scope resolution
  - type checking
  - program propagation flow


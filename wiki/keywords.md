# JOCKY Reserved Words (150)

> Every word below lexes as `Keyword` (`include/jocky/lexer/lexer.hpp`)
> and can never be a binding, field, function-segment, or evidence name —
> using one as a name fails at parse time (e.g. `expected binding name,
> found 'new'`). Only §1's 42 `.jky` words have grammar rules; §§2–4 are
> reserved future syntax space with no grammar behind them. Deliberately
> NOT reserved: `true`/`false` (lex as boolean literals via scan-word
> precedence), `source`/`count` (contextual identifiers), `null` (would
> need literal semantics — flagged future, not silently added).

## 1. `.jky` keywords (42) — these do something

| Word(s) | Role |
|---|---|
| `case`, `evidence`, `rule`, `investigate` | Block declarations (`wiki/grammar.md` §§2–5) |
| `let` | Bindings |
| `call` | Registry invocation |
| `source` | NOT a keyword — contextual identifier, listed here so nobody adds it: only special as a pipeline head followed by another identifier |
| `pcap`, `eventlog`, `directory` | Evidence adapters |
| `filter`, `where`, `having` | Predicate operators (identical grammar) |
| `select`, `as` | Projection + alias |
| `group_by`, `sort_by`, `limit`, `emit` | Pipeline operators |
| `correlate`, `within`, `on` | Join expression/operator |
| `and`, `or`, `not` | Logical operators |
| `in`, `contains`, `contains_any` | Comparison operators |
| `if`, `else`, `for`, `while` | Control flow (Phase 5.5) |
| `int` | `for`-header type; still valid as a type name |
| `report` | `write report(...)` / `emit report` (reserved elsewhere) |
| `when`, `score`, `tag`, `json`, `csv`, `markdown`, `html`, `readonly` | Reserved, unparsed — future syntax space |

## 2. C keywords (38; `case else for if while int` already in §1)

```
auto break char const continue default do double enum extern float goto
inline long register restrict return short signed sizeof static struct
switch typedef union unsigned void volatile
_Alignas _Alignof _Atomic _Bool _Complex _Generic _Imaginary _Noreturn
_Static_assert _Thread_local
```

## 3. C++ keywords (52; `and or not` already in §1)

```
asm bool catch char8_t char16_t char32_t class compl concept consteval
constexpr constinit const_cast co_await co_return co_yield decltype delete
dynamic_cast explicit export friend mutable namespace new noexcept nullptr
operator private protected public reinterpret_cast requires static_assert
static_cast template this thread_local throw try typeid typename using
virtual wchar_t and_eq bitand bitor not_eq or_eq xor xor_eq
```

## 4. Java keywords (18; remainder overlap C/C++ above)

```
abstract assert boolean byte extends final finally implements import
instanceof interface native package strictfp super synchronized throws
transient
```

## 5. Reservation record

- Added in one session (Phase 6): corpus scan found zero real collisions
  (4 matches, all inside `#` comments, never tokenized).
- `parse_type` accepts any keyword as a type name (type position is
  unambiguous), so `flag: bool` / `x: float` parse exactly as before —
  verified live, as was the rejection of `let new = …`.
- Trade-off, stated plainly: any future field/binding named exactly like
  one of the §§2–4 words is unusable bare — the price of C-familiar
  reserved space. Reversal is one lexer table.
- Full regression at reservation time: clean build (0 warnings), `check`
  AST byte-identical, 7/0/42 registry, phase3 `0,0,1,1,1,1`, phase4
  `1,1,1,1,0,1`, phase5 `0,0,0,1,0`, phase5.5 gate `0,0,0,1,0,0`.

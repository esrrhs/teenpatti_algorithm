# TeenPatti Algorithm (C++)

C++17 implementation of the Teen Patti lookup-table algorithm, functionally aligned with the [Java implementation](../java) and the [Go implementation](../go). All three share the same `teenpatti_data.txt` table, pass the same set of unit tests, and the C++ generation pipeline reproduces the table byte-for-byte.

[中文文档](../README_CN.md)

## Build

```bash
cmake -S cpp -B cpp/build -DCMAKE_BUILD_TYPE=Release
cmake --build cpp/build
ctest --output-on-failure   # run from cpp/build
```

Artifacts: `libteenpatti.a` (static library), `teenpatti_test`, `teenpatti_gen`.

## Usage

```cpp
#include <teenpatti/teenpatti.hpp>

#include <iostream>

int main() {
  // Load the embedded lookup table once at startup
  if (!teenpatti::load()) {
    return 1;
  }

  int type = teenpatti::get_win_type("黑A,方A,鬼");      // 6 = Three of a Kind
  int position = teenpatti::get_win_position("黑A,方A,鬼");  // global rank, higher = stronger
  int max_key = teenpatti::get_max("黑A,方A,鬼");        // best resolved hand (Joker expanded)
  std::string max_str = teenpatti::key_to_str(max_key);   // readable form, e.g. "方A黑A梅A"
  int result = teenpatti::compare("黑A,方A,鬼", "黑A,鬼,方3");  // > 0: first hand wins

  std::cout << type << " " << position << " " << max_key << " " << max_str << " " << result << "\n";
}
```

### Embedding

By default CMake embeds `teenpatti_data.txt` into the library through `.incbin` (GNU/Clang), so binaries are self-contained. Configure with `-DTEENPATTI_EMBED_DATA=OFF` to instead load the file at runtime from the working directory (mirroring the Java classpath-then-file fallback); `load_from_file(path)` and `load_from_stream(std::istream&)` are always available.

## API

| Function | Return | Description |
|----------|--------|-------------|
| `load()` / `load_from_file(path)` / `load_from_stream(in)` | `bool` | Load the lookup table (call `load()` once at startup) |
| `get_win_type(cards)` | `int` | Hand type constant |
| `get_win_position(cards)` | `int` | Global rank (higher = stronger) |
| `get_max(cards)` | `int` | Encoded key of the best resolved hand |
| `compare(a, b)` | `int` | Positive/zero/negative comparison result |
| `get_key_data(cards)` | `std::optional<KeyData>` | Full entry: `position`, `type`, `max` |
| `key_to_str(key)` / `key_to_bytes(key)` / `key_to_pokes(key)` | — | Decode an encoded key |
| `str_to_pokes(s)` / `str_to_poke(s)` | — | Parse card notation like `"黑A"` or `"鬼"` |
| `max_cards(cards)` / `compare_cards(a, b)` / `get_card_type_unordered(cards)` | — | Direct card utilities (Jokers resolved) |
| `normal_map()` | snapshot `shared_ptr` | The loaded `unordered_map<int, KeyData>` |

Overloads accept `std::string` (`"黑A,方A,鬼"`), `std::vector<std::uint8_t>` (packed card bytes) or `int` (encoded key), mirroring the Java API's parameter variants. Card types: `CardTypeGaoPai`(1) 高牌, `CardTypeDuiZi`(2) 对子, `CardTypeTongHua`(3) 同花, `CardTypeShunZi`(4) 顺子, `CardTypeTongHuaShun`(5) 同花顺, `CardTypeSanTiao`(6) 三条.

## Regenerating the lookup table

```bash
cmake --build cpp/build --target teenpatti_gen
cpp/build/teenpatti_gen   # writes teenpatti_data.txt into the working directory
```

The output is byte-identical to the Java-generated table.

## Tests

```bash
ctest --output-on-failure
```

The suite mirrors the Java unit tests one-to-one and additionally verifies every one of the 23479 table entries against a fresh recomputation through the algorithm.

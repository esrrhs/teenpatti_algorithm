# TeenPatti Algorithm (Go)

Go implementation of the Teen Patti lookup-table algorithm, functionally aligned with the [Java implementation](../java). Both share the same `teenpatti_data.txt` table, pass the same set of unit tests, and the Go generation pipeline reproduces the table byte-for-byte.

[中文文档](../README_CN.md)

## Install

```bash
go get github.com/esrrhs/teenpatti_algorithm/go
```

## Usage

```go
package main

import (
	"fmt"

	teenpatti "github.com/esrrhs/teenpatti_algorithm/go"
)

func main() {
	// Load the embedded lookup table once at startup
	if err := teenpatti.Load(); err != nil {
		panic(err)
	}

	typ := teenpatti.GetWinType("黑A,方A,鬼")                    // 6 = Three of a Kind
	position := teenpatti.GetWinPosition("黑A,方A,鬼")            // global rank, higher = stronger
	maxKey := teenpatti.GetMax("黑A,方A,鬼")                     // best resolved hand (Joker expanded)
	maxStr := teenpatti.KeyToStr(maxKey)                        // readable form, e.g. "方A方A梅A"
	result := teenpatti.Compare("黑A,方A,鬼", "黑A,鬼,方3")       // > 0: first hand wins

	fmt.Println(typ, position, maxKey, maxStr, result)
}
```

## API

| Function | Return | Description |
|----------|--------|-------------|
| `Load()` | `error` | Load the embedded `teenpatti_data.txt` (call once at startup) |
| `LoadFromFile(path)` | `error` | Load the lookup table from a custom file |
| `GetWinType(string)` / `GetWinTypeByCards([]byte)` | `int` | Hand type constant |
| `GetWinPosition(string)` / `GetWinPositionByCards([]byte)` | `int` | Global rank (higher = stronger) |
| `GetMax(string)` / `GetMaxByCards([]byte)` | `int` | Encoded key of the best resolved hand |
| `Compare(a, b string)` / `CompareByCards(a, b []byte)` | `int` | Positive/zero/negative comparison result |
| `CompareByKey(a, b int)` | `int` | Compare two encoded keys |
| `GetKeyData(string)` / `GetKeyDataByCards([]byte)` / `GetKeyDataByKey(int)` | `*KeyData` | Full entry: `Position`, `Type`, `Max` |
| `KeyToStr(int)` / `KeyToBytes(int)` / `KeyToPokes(int)` | — | Decode an encoded key |
| `StrToPokes(string)` / `StrToPoke(string)` | — | Parse card notation like `"黑A"` or `"鬼"` |
| `MaxCards([]Poke)` / `CompareCards(a, b []Poke)` / `GetCardTypeUnordered([]Poke)` | — | Direct card utilities (Jokers resolved) |

Card types: `CardTypeGaoPai`(1) 高牌, `CardTypeDuiZi`(2) 对子, `CardTypeTongHua`(3) 同花, `CardTypeShunZi`(4) 顺子, `CardTypeTongHuaShun`(5) 同花顺, `CardTypeSanTiao`(6) 三条.

## Regenerating the lookup table

```bash
go run ./cmd/teenpatti_gen
```

Writes `teenpatti_data.txt` into the working directory. The output is byte-identical to the Java-generated table.

## Tests

```bash
go test -race ./...
```

The suite mirrors the Java unit tests one-to-one and additionally verifies every one of the 23479 table entries against a fresh recomputation through the algorithm.

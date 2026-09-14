# TeenPatti Algorithm

[<img src="https://img.shields.io/github/license/esrrhs/teenpatti_algorithm">](https://github.com/esrrhs/teenpatti_algorithm)
[<img src="https://img.shields.io/github/languages/top/esrrhs/teenpatti_algorithm">](https://github.com/esrrhs/teenpatti_algorithm)
[<img src="https://img.shields.io/maven-central/v/com.github.esrrhs/teenpatti_algorithm">](https://central.sonatype.com/artifact/com.github.esrrhs/teenpatti_algorithm)
[<img src="https://img.shields.io/github/actions/workflow/status/esrrhs/teenpatti_algorithm/maven.yml?branch=master">](https://github.com/esrrhs/teenpatti_algorithm/actions)

[中文文档](README_CN.md)

A high-performance lookup-table algorithm for the Indian card game **Teen Patti**, with full support for **Joker (wild card)**. Given any 3-card hand, the library instantly returns the hand's rank, type, and best possible combination.

Derived from [texas_algorithm](https://github.com/esrrhs/texas_algorithm).

---

## Table of Contents

- [Game Rules Overview](#game-rules-overview)
- [Quick Start](#quick-start)
- [API Reference](#api-reference)
- [Card Notation](#card-notation)
- [Hand Rankings](#hand-rankings)
- [Algorithm Design](#algorithm-design)
- [Generating the Lookup Table](#generating-the-lookup-table)
- [Running the Demo](#running-the-demo)
- [Related Projects](#related-projects)

---

## Game Rules Overview

Teen Patti is played with a standard 52-card deck plus **3 Jokers** (55 cards total). Each player is dealt 3 cards. The goal is to have the best 3-card hand. Jokers are wild — they substitute for any card to form the best possible combination.

---

## Quick Start

### Maven dependency

```xml
<dependency>
    <groupId>com.github.esrrhs</groupId>
    <artifactId>teenpatti_algorithm</artifactId>
    <version>1.0.3</version>
</dependency>
```

### Basic usage

```java
// 1. Load the lookup table once at startup
TeenPattiAlgorithmUtil.load();

// 2. Get the hand type (returns an integer constant, see Hand Rankings)
int type = TeenPattiAlgorithmUtil.getWinType("黑A,方A,鬼");
// type == 6  → Three of a Kind (Joker acts as a third Ace)

// 3. Get the rank position (higher = stronger hand)
int position = TeenPattiAlgorithmUtil.getWinPosition("黑2,黑3,黑4");

// 4. Compare two hands  (positive = first hand wins, negative = second wins, 0 = tie)
int result = TeenPattiAlgorithmUtil.compare("黑A,方A,鬼", "黑A,鬼,方3");

// 5. Get the best resolved hand (Joker expanded to its optimal card)
int maxKey = TeenPattiAlgorithmUtil.getMax("黑A,方A,鬼");
String maxStr = TeenPattiAlgorithmUtil.keyToStr(maxKey);  // e.g. "黑A方A红A"
```

---

## API Reference

All public methods are on `TeenPattiAlgorithmUtil`.

| Method | Parameters | Return | Description |
|--------|-----------|--------|-------------|
| `load()` | — | `void` | Load lookup table from `teenpatti_data.txt` (call once at startup) |
| `loadNormal(InputStream)` | input stream | `void` | Load lookup table from a custom stream |
| `getWinType(String)` | comma-separated cards | `int` | Hand type constant (see Hand Rankings) |
| `getWinPosition(String)` | comma-separated cards | `int` | Global rank (higher = stronger) |
| `getMax(String)` | comma-separated cards | `int` | Encoded key of the best resolved hand |
| `compare(String, String)` | two hands | `int` | Positive/zero/negative comparison result |
| `keyToStr(int)` | encoded key | `String` | Human-readable card string |

### `KeyData` fields

`getKeyData()` returns a `KeyData` object with three fields:

| Field | Getter | Description |
|-------|--------|-------------|
| `position` | `getPosition()` / `getPostion()` | Global rank index among all possible hands |
| `type` | `getType()` | Hand type (1–6, see Hand Rankings) |
| `max` | `getMax()` | Encoded key of best resolved hand |

---

## Card Notation

Cards are written as `<suit><value>` in Chinese notation, separated by commas.

### Suits

| Notation | Suit | English |
|----------|------|---------|
| `方` | ♦ | Diamonds |
| `梅` | ♣ | Clubs |
| `红` | ♥ | Hearts |
| `黑` | ♠ | Spades |

### Values

`2` `3` `4` `5` `6` `7` `8` `9` `10` `J` `Q` `K` `A`

### Joker

Write `鬼` for a wild Joker card. There are 3 Jokers in the deck.

### Examples

| Input string | Meaning |
|--------------|---------|
| `"黑A,方A,鬼"` | Ace of Spades, Ace of Diamonds, Joker |
| `"黑2,黑3,黑4"` | 2♠ 3♠ 4♠ |
| `"红K,梅Q,方J"` | K♥ Q♣ J♦ |

---

## Hand Rankings

Ranked from lowest to highest:

| Rank | Type constant | Name | Description |
|------|--------------|------|-------------|
| 1 | `TEENPATTI_CARD_TYPE_GAOPAI = 1` | High Card | No combination; highest card wins |
| 2 | `TEENPATTI_CARD_TYPE_DUIZI = 2` | Pair | Two cards of the same value |
| 3 | `TEENPATTI_CARD_TYPE_TONGHUA = 3` | Flush | All three cards of the same suit |
| 4 | `TEENPATTI_CARD_TYPE_SHUNZI = 4` | Straight | Three consecutive values (A-2-3 also valid) |
| 5 | `TEENPATTI_CARD_TYPE_TONGHUASHUN = 5` | Straight Flush | Consecutive values, all same suit |
| 6 | `TEENPATTI_CARD_TYPE_SANTIAO = 6` | Three of a Kind | All three cards of the same value |

> **Note:** Three of a Kind ranks higher than Straight Flush in Teen Patti, unlike Texas Hold'em.

**Tie-breaking rules:**
- **Three of a Kind / Pair:** compare by the repeated card's value, then the kicker.
- **All others:** compare highest card first, then second, then third.

---

## Algorithm Design

### Overview

The library uses a **pre-computed lookup table**. At query time, a 3-card hand is encoded into a single integer key; the key is looked up in a `ConcurrentHashMap` to retrieve the hand's rank, type, and best expansion in O(1).

### Step 1 — Card encoding

Each card is packed into one byte: the upper 4 bits hold the suit (0–3), the lower 4 bits hold the value (2–14). A Joker uses a reserved `(color=5, value=8)` sentinel.

```
byte = (suit << 4) | value
```

A 3-card hand is encoded into a single `int` by concatenating the three byte values in decimal:

```
key = card1_byte * 10000 + card2_byte * 100 + card3_byte
```

Cards are sorted before encoding so the same set always produces the same key regardless of order.

### Step 2 — Enumerate all combinations

The deck contains **55 cards** (52 regular + 3 Jokers). All C(55, 3) = **26,235 unique combinations** are enumerated using a recursive combination generator. Duplicate-key hands (e.g. a hand where Joker resolves to an identical state) are deduplicated.

### Step 3 — Multi-threaded quicksort

All combination keys are sorted by hand strength using a **parallel quicksort** (`Sorter.java`). The thread pool size equals the number of available CPU cores. When the number of active threads exceeds `2 × CPU_CORES`, sub-partitions fall back to in-thread recursion to avoid thread explosion.

The comparison function (`GenUtil.compare`) resolves Jokers to their best possible substitution before comparing, so the sort order reflects the true game outcome.

### Step 4 — Output the lookup table

After sorting, each entry is written to `teenpatti_data.txt` with:

```
<key> <rank> <rank_index> <total> <best_hand_str> <hand_type> <best_hand_key> <best_hand_readable>
```

The rank index is incremented only when two adjacent hands are not equal in strength, producing a dense sequential ranking.

### Query path (runtime)

```
input string  →  parse cards  →  sort bytes  →  encode key  →  HashMap.get(key)  →  KeyData{position, type, max}
```

---

## Generating the Lookup Table

Run `TeenPattiAlgorithmUtil.main()` (or `GenUtil.genKey()` + `GenUtil.outputData()`) to regenerate `teenpatti_data.txt`. This is only needed if you modify the deck or ranking rules.

```bash
mvn exec:java -Dexec.mainClass="com.github.esrrhs.teenpatti_algorithm.TeenPattiAlgorithmUtil"
```

The generation process prints progress with estimated time remaining and throughput (entries/sec).

---

## Running the Demo

`TestUtil.main()` loads the table and prints results for two sample hands:

```java
TeenPattiAlgorithmUtil.load();

String cards  = "黑A,方A,鬼";   // A♠ A♦ Joker  → Three Aces
String cards1 = "黑A,鬼,方3";   // A♠ Joker 3♦  → Pair of Aces

System.out.println(TeenPattiAlgorithmUtil.getWinPosition(cards));   // rank
System.out.println(TeenPattiAlgorithmUtil.getWinType(cards));       // 6 = Three of a Kind
System.out.println(TeenPattiAlgorithmUtil.keyToStr(
        TeenPattiAlgorithmUtil.getMax(cards)));                      // best resolved hand

System.out.println(TeenPattiAlgorithmUtil.compare(cards, cards1));  // > 0: cards wins
```

---

## Related Projects

- [majiang_algorithm](https://github.com/esrrhs/majiang_algorithm) — Mahjong algorithm
- [texas_algorithm](https://github.com/esrrhs/texas_algorithm) — Texas Hold'em algorithm

---

## License

This project is licensed under the [MIT License](LICENSE).

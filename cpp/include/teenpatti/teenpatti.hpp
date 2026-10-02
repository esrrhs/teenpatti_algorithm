// TeenPatti lookup-table algorithm, C++17 port of the Java teenpatti_algorithm
// library (https://github.com/esrrhs/teenpatti_algorithm).
//
// Evaluates 3-card Teen Patti hands through a pre-computed lookup table, with
// full support for Joker (wild card) hands, and can regenerate the lookup
// table itself. Behavior is aligned with the Java and Go implementations:
// the same teenpatti_data.txt, the same ranking, byte-identical regeneration.
#pragma once

#include <cstdint>
#include <iosfwd>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace teenpatti {

// Card values, 2 through A.
constexpr std::uint8_t PokeValue2 = 2;
constexpr std::uint8_t PokeValue3 = 3;
constexpr std::uint8_t PokeValue4 = 4;
constexpr std::uint8_t PokeValue5 = 5;
constexpr std::uint8_t PokeValue6 = 6;
constexpr std::uint8_t PokeValue7 = 7;
constexpr std::uint8_t PokeValue8 = 8;
constexpr std::uint8_t PokeValue9 = 9;
constexpr std::uint8_t PokeValue10 = 10;
constexpr std::uint8_t PokeValueJ = 11;
constexpr std::uint8_t PokeValueQ = 12;
constexpr std::uint8_t PokeValueK = 13;
constexpr std::uint8_t PokeValueA = 14;

// Card colors (suits).
constexpr std::uint8_t PokeColorFang = 0;  // 方 diamonds
constexpr std::uint8_t PokeColorMei = 1;   // 梅 clubs
constexpr std::uint8_t PokeColorHong = 2;  // 红 hearts
constexpr std::uint8_t PokeColorHei = 3;   // 黑 spades

// Card type constants, ordered from lowest to highest ranking.
// Three of a Kind outranks Straight Flush in Teen Patti.
constexpr int CardTypeUnknown = 0;      // 未知
constexpr int CardTypeGaoPai = 1;       // 高牌 high card
constexpr int CardTypeDuiZi = 2;        // 对子 pair
constexpr int CardTypeTongHua = 3;      // 同花 flush
constexpr int CardTypeShunZi = 4;       // 顺子 straight
constexpr int CardTypeTongHuaShun = 5;  // 同花顺 straight flush
constexpr int CardTypeSanTiao = 6;      // 三条 three of a kind

// Deck layout used by the generation pipeline: 52 regular cards + 3 Jokers.
constexpr int kGuiNum = 3;
constexpr int kGenNum = 52 + kGuiNum;
constexpr int kGenTotal = (kGenNum * (kGenNum - 1) * (kGenNum - 2)) / (3 * 2);  // C(55, 3)

// A single card.
struct Poke {
  std::uint8_t color = 0;
  std::uint8_t value = 0;

  // Packs the card into one byte: (color << 4) | value.
  std::uint8_t to_byte() const;
  // Reports whether the card is the Joker.
  bool is_gui() const;
  // Returns the human readable form, e.g. "黑A" or "鬼".
  std::string to_string() const;

  friend bool operator==(const Poke& a, const Poke& b) {
    return a.color == b.color && a.value == b.value;
  }
  friend bool operator!=(const Poke& a, const Poke& b) { return !(a == b); }
};

// The Joker (wild card) sentinel poke.
inline constexpr Poke kGuiPoke{5, 8};

// Unpacks a card from its packed byte form.
Poke poke_from_byte(std::uint8_t b);
// Reports whether the packed byte is the Joker sentinel.
bool is_gui_byte(std::uint8_t b);

// Returns the type of a 3-card hand regardless of card order, or
// CardTypeUnknown when the hand does not hold exactly 3 cards.
int get_card_type_unordered(const std::vector<Poke>& cards);

// Converts cards to their packed byte forms.
std::vector<std::uint8_t> poke_to_bytes(const std::vector<Poke>& cards);
// Converts packed card bytes back to cards.
std::vector<Poke> bytes_to_pokes(const std::vector<std::uint8_t>& bytes);

// Resolves Jokers in the hand to their best possible substitution and returns
// the strongest hand. Hands without Jokers are returned as a copy.
std::vector<Poke> max_cards(const std::vector<Poke>& cards);

// Compares two hands, resolving Jokers to their best possible substitution
// first. Returns 1 win, 0 tie, -1 lose.
int compare_cards(const std::vector<Poke>& first, const std::vector<Poke>& second);
// Compares two hands that contain no Jokers. Returns 1 win, 0 tie, -1 lose.
int compare_cards_without_gui(const std::vector<Poke>& first, const std::vector<Poke>& second);

// Encodes an ordered card byte slice into its integer key: each card byte
// becomes two decimal digits, first card most significant.
int gen_card_bind(const std::vector<std::uint8_t>& cards);
// Encodes an ordered Poke slice into its integer key.
int gen_poke_card_bind(const std::vector<Poke>& cards);

// Unpacks an integer key into its card bytes, most significant digit first
// (i.e. the ascending card order the key was built from).
std::vector<std::uint8_t> key_to_bytes(std::int64_t key);
// Unpacks an integer key back into its cards.
std::vector<Poke> key_to_pokes(std::int64_t key);
// Renders an integer key as the concatenated readable cards.
std::string key_to_str(std::int64_t key);

// The lookup result for one hand: its global rank among all possible hands,
// the type of its best resolved hand, and the encoded key of that best
// resolved hand.
struct KeyData {
  int position;
  int type;
  int max;
};

// Loads the lookup table: from the data embedded at compile time when
// available, otherwise from "teenpatti_data.txt" in the working directory
// (mirroring the Java classpath-then-file fallback). Call once at startup.
bool load();
// Loads the lookup table from an explicit file.
bool load_from_file(const std::string& path);
// Loads the lookup table from a stream of teenpatti_data.txt lines.
bool load_from_stream(std::istream& in);

// Returns a snapshot of the loaded key table. The snapshot stays valid
// regardless of later loads; treat it as read-only.
std::shared_ptr<const std::unordered_map<int, KeyData>> normal_map();

// Parses a card value token ("A", "K", "Q", "J" or "2".."10").
// Returns 0 when the token is not a valid value.
std::uint8_t str_to_poke_value(const std::string& s);
// Parses a single card token like "黑A" or "鬼".
// Returns 0 when the token is not a valid card.
std::uint8_t str_to_poke(const std::string& s);
// Parses a comma separated hand like "黑A,方A,鬼".
std::vector<std::uint8_t> str_to_pokes(const std::string& s);

// Looks up a hand given as a comma separated string. For compatibility with
// the Java implementation this requires exactly seven cards and returns
// nullopt otherwise; use the vector overload for 3-card hands.
std::optional<KeyData> get_key_data(const std::string& cards);
// Sorts a copy of pokes ascending, encodes them into a key and looks the key
// up in the table. Returns nullopt when the hand is not in the table.
std::optional<KeyData> get_key_data(std::vector<std::uint8_t> pokes);
// Returns the table entry for an encoded key, or nullopt.
std::optional<KeyData> get_key_data(int key);

// Returns the global rank of a hand, higher is stronger. Returns 0 when the
// hand is not in the table.
int get_win_position(const std::string& cards);
int get_win_position(std::vector<std::uint8_t> pokes);

// Returns the type of a hand's best resolved hand, one of the CardType
// constants. Returns 0 when the hand is not in the table.
int get_win_type(const std::string& cards);
int get_win_type(std::vector<std::uint8_t> pokes);

// Returns the encoded key of a hand's best resolved hand. Returns 0 when the
// hand is not in the table.
int get_max(const std::string& cards);
int get_max(std::vector<std::uint8_t> pokes);

// Compares two hands given as comma separated card strings. Returns a
// positive value when the first hand wins, 0 on a tie and a negative value
// when the second hand wins.
int compare(const std::string& a, const std::string& b);
// Compares two hands given as parsed card bytes; both vectors are passed by
// value and sorted internally.
int compare(std::vector<std::uint8_t> a, std::vector<std::uint8_t> b);
// Compares two encoded keys. Returns a positive value when the first key
// ranks higher, 0 on a tie and a negative value when the second key ranks
// higher; missing keys sort lowest.
int compare(int k1, int k2);

// Generation pipeline: enumerates all C(55,3) hands and collects the
// distinct keys, unsorted, in enumeration order.
void gen_key();
// Sorts the keys collected by gen_key() by hand strength and writes
// teenpatti_data.txt into the working directory. Returns true on success.
bool output_data();
// The unsorted keys collected by the last gen_key() call.
const std::vector<int>& gen_keys();
// Sorts values in place by hand strength, weakest first, using a parallel
// quicksort: the same algorithm as the Java Sorter.
void quicksort(std::vector<int>& values);

}  // namespace teenpatti

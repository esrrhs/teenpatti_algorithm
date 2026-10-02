#include "teenpatti/teenpatti.hpp"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cmath>
#include <condition_variable>
#include <cstdio>
#include <fstream>
#include <memory>
#include <mutex>
#include <sstream>
#include <string>
#include <thread>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

// ---------------------------------------------------------------------------
// Compile-time embedded teenpatti_data.txt via .incbin (GNU/Clang only, wired
// up by CMake through TEENPATTI_EMBEDDED_DATA_PATH). Other compilers fall
// back to loading the file at runtime from the working directory.
// ---------------------------------------------------------------------------
#if defined(TEENPATTI_EMBEDDED_DATA_PATH) && defined(__GNUC__)
#define TEENPATTI_HAS_EMBEDDED_DATA 1
#define TP_STR2(x) #x
#define TP_STR(x) TP_STR2(x)
#if defined(__APPLE__)
#define TP_EMBED_SECTION "__TEXT,__const"
#define TP_EMBED_SYM(name) "_" #name
#else
#define TP_EMBED_SECTION ".rodata,\"a\",@progbits"
#define TP_EMBED_SYM(name) #name
#endif

__asm__(
    ".pushsection " TP_EMBED_SECTION "\n"
    ".globl " TP_EMBED_SYM(teenpatti_embedded_data_start) "\n"
    TP_EMBED_SYM(teenpatti_embedded_data_start) ":\n"
    // TEENPATTI_EMBEDDED_DATA_PATH is passed by CMake with the quotes already
    // part of the macro value, so no extra quoting is needed here.
    ".incbin " TP_STR(TEENPATTI_EMBEDDED_DATA_PATH) "\n"
    ".globl " TP_EMBED_SYM(teenpatti_embedded_data_end) "\n"
    TP_EMBED_SYM(teenpatti_embedded_data_end) ":\n"
    ".byte 0\n"
    ".popsection\n");

extern "C" const char teenpatti_embedded_data_start[];
extern "C" const char teenpatti_embedded_data_end[];
#endif

namespace teenpatti {

namespace {

// Data file name used by the file fallback of load().
constexpr const char* kDataFileName = "teenpatti_data.txt";

// Fallback multiplier of the parallel quicksort, mirroring the Java Sorter.
constexpr int kSorterFallback = 2;

const char* const kHuaSeName[] = {"方", "梅", "红", "黑"};
const char* const kValueName[] = {"",  "",  "2", "3", "4", "5", "6", "7",
                                  "8", "9", "10", "J", "Q", "K", "A"};

long long now_ms() {
  return std::chrono::duration_cast<std::chrono::milliseconds>(
             std::chrono::system_clock::now().time_since_epoch())
      .count();
}

bool starts_with(const std::string& s, const char* prefix) { return s.rfind(prefix, 0) == 0; }

std::vector<std::string> split(const std::string& s, char delim) {
  std::vector<std::string> out;
  std::string item;
  std::istringstream iss(s);
  while (std::getline(iss, item, delim)) {
    out.push_back(item);
  }
  return out;
}

// Generation pipeline state, mirroring the statics of the Java GenUtil.
std::atomic<long long> g_gen_total_key{0};
std::atomic<long long> g_gen_progress{0};
std::atomic<long long> g_gen_last_print{0};
std::atomic<long long> g_gen_begin{0};
std::vector<int> g_gen_keys;
std::unordered_set<int> g_gen_key_seen;

// The currently loaded key table; replaced atomically by the load functions.
std::shared_ptr<const std::unordered_map<int, KeyData>> g_normal_map =
    std::make_shared<const std::unordered_map<int, KeyData>>();

// Enumerates all size-`except` combinations of a in order.
template <typename Run>
void permutation(Run&& run, const std::vector<int>& a, int count, int count2, int except,
                 std::vector<int>& tmp) {
  if (count2 == except) {
    run(tmp);
    return;
  }
  for (int i = count; i < static_cast<int>(a.size()); ++i) {
    tmp[count2] = a[i];
    permutation(run, a, i + 1, count2 + 1, except, tmp);
  }
}

// The ascending deck: 52 regular cards then 3 Jokers, as in the Java GenUtil.
const std::vector<int>& gen_all_cards() {
  static const std::vector<int> list = [] {
    std::vector<int> l;
    l.reserve(kGenNum);
    for (int i = 0; i < 4; ++i) {
      for (int j = 0; j < kGenNum / 4; ++j) {
        l.push_back(Poke{static_cast<std::uint8_t>(i), static_cast<std::uint8_t>(j + 2)}.to_byte());
      }
    }
    for (int i = 0; i < kGuiNum; ++i) {
      l.push_back(kGuiPoke.to_byte());
    }
    std::sort(l.begin(), l.end());
    return l;
  }();
  return list;
}

int gen_card_bind_ints(const std::vector<int>& cards) {
  int ret = 0;
  for (int v : cards) {
    ret = ret * 100 + v;
  }
  return ret;
}

int compare_dui_zi(std::vector<Poke>& sort_cards1, std::vector<Poke>& sort_cards2) {
  if (sort_cards1[0].value != sort_cards1[1].value) {
    std::swap(sort_cards1[0], sort_cards1[2]);
  }
  if (sort_cards2[0].value != sort_cards2[1].value) {
    std::swap(sort_cards2[0], sort_cards2[2]);
  }
  if (sort_cards1[0].value != sort_cards2[0].value) {
    return sort_cards1[0].value > sort_cards2[0].value ? 1 : -1;
  }
  if (sort_cards1[2].value != sort_cards2[2].value) {
    return sort_cards1[2].value > sort_cards2[2].value ? 1 : -1;
  }
  return 0;
}

int compare_not_dui_zi(std::vector<Poke>& sort_cards1, std::vector<Poke>& sort_cards2) {
  for (int i = 2; i >= 0; --i) {
    if (sort_cards1[i].value != sort_cards2[i].value) {
      return sort_cards1[i].value > sort_cards2[i].value ? 1 : -1;
    }
  }
  return 0;
}

int compare_by_value(const std::vector<Poke>& first_cards, const std::vector<Poke>& second_cards) {
  const int card_type = get_card_type_unordered(first_cards);
  if (card_type == CardTypeUnknown) {
    return 0;
  }
  std::vector<Poke> sort_cards1(first_cards);
  std::vector<Poke> sort_cards2(second_cards);
  std::stable_sort(sort_cards1.begin(), sort_cards1.end(),
                   [](const Poke& a, const Poke& b) { return a.value < b.value; });
  std::stable_sort(sort_cards2.begin(), sort_cards2.end(),
                   [](const Poke& a, const Poke& b) { return a.value < b.value; });
  if (card_type == CardTypeDuiZi) {
    return compare_dui_zi(sort_cards1, sort_cards2);
  }
  return compare_not_dui_zi(sort_cards1, sort_cards2);
}

bool gen_compare(int k1, int k2) {
  return compare_cards(key_to_pokes(k1), key_to_pokes(k2)) < 0;
}

bool gen_equal(int k1, int k2) {
  return compare_cards(key_to_pokes(k1), key_to_pokes(k2)) == 0;
}

// The encoded key of the best resolved hand for k.
int gen_max(int k) { return gen_poke_card_bind(max_cards(key_to_pokes(k))); }

// The card type of the best resolved hand for k.
int gen_max_type(int k) { return get_card_type_unordered(max_cards(key_to_pokes(k))); }

// Parallel quicksort over the key list: same partition scheme and
// inline-vs-thread fallback as the Java Sorter.
struct QuickSorter {
  std::vector<int>& values;
  int total;
  int n_threads;
  std::atomic<int> tasks{0};
  std::mutex wait_mutex;
  std::condition_variable wait_cv;

  void spawn(int left, int right, int layer) {
    tasks.fetch_add(1);
    std::thread([this, left, right, layer] {
      quicksort(layer, left, right);
      if (tasks.fetch_sub(1) == 1) {
        std::lock_guard<std::mutex> lock(wait_mutex);
        wait_cv.notify_all();
      }
    }).detach();
  }

  void quicksort(int layer, int lower_index, int higher_index) {
    if (higher_index < lower_index) {
      return;
    }
    if (higher_index == lower_index) {
      print_leaf_progress();
      return;
    }

    int i = lower_index;
    int j = higher_index;
    // pivot value taken from the middle index
    const int pivot = values[lower_index + (higher_index - lower_index) / 2];

    long long total_step = j - i;
    if (total_step == 0) {
      total_step = 1;
    }
    int last_print = 0;
    const long long begin_print = now_ms();

    while (i <= j) {
      while (gen_compare(values[i], pivot)) {
        ++i;
      }
      while (gen_compare(pivot, values[j])) {
        --j;
      }
      if (i <= j) {
        std::swap(values[i], values[j]);
        ++i;
        --j;
      }

      if (i <= j && total_step > 100000) {
        long long step = total_step - (j - i);
        if (step < 0) {
          step = 0;
        }
        const int cur = static_cast<int>(step * 100 / total_step);
        if (cur != last_print) {
          last_print = cur;
          const long long now = now_ms();
          const double per = static_cast<double>(now - begin_print) / step;
          std::printf("%d/%d层 %d%% 需要%.1f分 用时%.1f分 速度%.1f条/秒\n", layer,
                      static_cast<int>(std::log(static_cast<double>(total))), cur,
                      per * (total_step - step) / 60.0 / 1000.0,
                      (now - begin_print) / 60.0 / 1000.0,
                      step / ((now - begin_print) / 1000.0));
        }
      }
    }

    if (tasks.load() >= kSorterFallback * n_threads) {
      if (i - j == 1) {
        quicksort(layer + 1, lower_index, j);
        quicksort(layer + 1, i, higher_index);
      } else {
        quicksort(layer + 1, lower_index, j + 1);
        quicksort(layer + 1, i, higher_index);
      }
    } else {
      if (i - j == 1) {
        spawn(lower_index, j, layer + 1);
        spawn(i, higher_index, layer + 1);
      } else {
        spawn(lower_index, j + 1, layer + 1);
        spawn(i, higher_index, layer + 1);
      }
    }
  }

  void print_leaf_progress() {
    const long long step = g_gen_progress.fetch_add(1) + 1;
    const long long cur = step * 10000 / total;
    if (cur != g_gen_last_print.load()) {
      g_gen_last_print.store(cur);
      const long long now = now_ms();
      const double per = static_cast<double>(now - g_gen_begin.load()) / step;
      std::printf("%lld%%%% 需要%.1f分 用时%.1f分 速度%.1f条/秒\n", cur,
                  per * (total - step) / 60.0 / 1000.0,
                  (now - g_gen_begin.load()) / 60.0 / 1000.0,
                  step / ((now - g_gen_begin.load()) / 1000.0));
    }
  }
};

void gen_card_save(const std::vector<int>& tmp) {
  const int c = gen_card_bind_ints(tmp);
  if (g_gen_key_seen.insert(c).second) {
    g_gen_keys.push_back(c);
  }
  const long long total_key = g_gen_total_key.fetch_add(1) + 1;

  const int cur = static_cast<int>(total_key * 100 / kGenTotal);
  if (cur != g_gen_last_print.load()) {
    g_gen_last_print.store(cur);
    const long long now = now_ms();
    const double per = static_cast<double>(now - g_gen_begin.load()) / total_key;
    std::printf("%d%% 需要%.1f分 用时%.1f分 速度%.1f条/秒\n", cur,
                per * (kGenTotal - total_key) / 60.0 / 1000.0,
                (now - g_gen_begin.load()) / 60.0 / 1000.0,
                total_key / ((now - g_gen_begin.load()) / 1000.0));
  }
}

}  // namespace

std::uint8_t Poke::to_byte() const { return static_cast<std::uint8_t>((color << 4) | value); }

bool Poke::is_gui() const { return value == kGuiPoke.value && color == kGuiPoke.color; }

std::string Poke::to_string() const {
  if (is_gui()) {
    return "鬼";
  }
  if (color >= 4 || value >= 15) {
    return "?";
  }
  return std::string(kHuaSeName[color]) + kValueName[value];
}

Poke poke_from_byte(std::uint8_t b) {
  return Poke{static_cast<std::uint8_t>(b >> 4), static_cast<std::uint8_t>(b % 16)};
}

bool is_gui_byte(std::uint8_t b) {
  return b % 16 == kGuiPoke.value && b >> 4 == kGuiPoke.color;
}

int get_card_type_unordered(const std::vector<Poke>& cards) {
  if (cards.size() != 3) {
    return CardTypeUnknown;
  }
  std::vector<Poke> tmp(cards);
  std::stable_sort(tmp.begin(), tmp.end(),
                   [](const Poke& a, const Poke& b) { return a.value < b.value; });
  const int value1 = tmp[0].value;
  const int value2 = tmp[1].value;
  const int value3 = tmp[2].value;

  const int color1 = tmp[0].color;
  const int color2 = tmp[1].color;
  const int color3 = tmp[2].color;

  if (value1 == value2 && value2 == value3) {
    return CardTypeSanTiao;
  }
  if (value1 == value2 || value2 == value3) {
    return CardTypeDuiZi;
  }

  const bool same_color = color1 == color2 && color1 == color3;

  bool shun_zi = value2 == value1 + 1 && value3 == value2 + 1;
  if (value1 == PokeValue2 && value2 == PokeValue3 && value3 == PokeValueA) {
    shun_zi = true;
  }

  if (same_color && shun_zi) {
    return CardTypeTongHuaShun;
  }
  if (same_color) {
    return CardTypeTongHua;
  }
  if (shun_zi) {
    return CardTypeShunZi;
  }
  return CardTypeGaoPai;
}

std::vector<std::uint8_t> poke_to_bytes(const std::vector<Poke>& cards) {
  std::vector<std::uint8_t> ret;
  ret.reserve(cards.size());
  for (const Poke& p : cards) {
    ret.push_back(p.to_byte());
  }
  return ret;
}

std::vector<Poke> bytes_to_pokes(const std::vector<std::uint8_t>& bytes) {
  std::vector<Poke> ret;
  ret.reserve(bytes.size());
  for (std::uint8_t b : bytes) {
    ret.push_back(poke_from_byte(b));
  }
  return ret;
}

std::vector<Poke> max_cards(const std::vector<Poke>& cards) {
  std::vector<Poke> ret(cards);

  int gui = 0;
  for (const Poke& p : cards) {
    if (p.is_gui()) {
      ++gui;
    }
  }
  if (gui == 0) {
    return ret;
  }

  std::vector<Poke> left;
  std::vector<int> left_map;
  for (const Poke& p : cards) {
    if (!p.is_gui()) {
      left.push_back(p);
      left_map.push_back(p.to_byte());
    }
  }

  std::vector<Poke> max;
  std::vector<int> tmp(gui);
  permutation(
      [&](const std::vector<int>& t) {
        for (int x : t) {
          if (std::find(left_map.begin(), left_map.end(), x) != left_map.end() ||
              x == kGuiPoke.to_byte()) {
            return;
          }
        }

        std::vector<Poke> last(left);
        for (int x : t) {
          last.push_back(poke_from_byte(static_cast<std::uint8_t>(x)));
        }
        if (max.empty() || compare_cards_without_gui(last, max) > 0) {
          max = last;
        }
      },
      gen_all_cards(), 0, 0, gui, tmp);
  return max;
}

int compare_cards(const std::vector<Poke>& first, const std::vector<Poke>& second) {
  return compare_cards_without_gui(max_cards(first), max_cards(second));
}

int compare_cards_without_gui(const std::vector<Poke>& first, const std::vector<Poke>& second) {
  const int first_card_type = get_card_type_unordered(first);
  const int second_card_type = get_card_type_unordered(second);
  if (first_card_type != second_card_type) {
    return first_card_type > second_card_type ? 1 : -1;
  }
  return compare_by_value(first, second);
}

int gen_card_bind(const std::vector<std::uint8_t>& cards) {
  int ret = 0;
  for (std::uint8_t b : cards) {
    ret = ret * 100 + b;
  }
  return ret;
}

int gen_poke_card_bind(const std::vector<Poke>& cards) {
  int ret = 0;
  for (const Poke& p : cards) {
    ret = ret * 100 + p.to_byte();
  }
  return ret;
}

std::vector<std::uint8_t> key_to_bytes(std::int64_t k) {
  std::vector<std::uint8_t> cs;
  if (k > 1000000000000LL) {
    cs.push_back(static_cast<std::uint8_t>(k % 100000000000000LL / 1000000000000LL));
  }
  if (k > 10000000000LL) {
    cs.push_back(static_cast<std::uint8_t>(k % 1000000000000LL / 10000000000LL));
  }
  if (k > 100000000LL) {
    cs.push_back(static_cast<std::uint8_t>(k % 10000000000LL / 100000000LL));
  }
  if (k > 1000000LL) {
    cs.push_back(static_cast<std::uint8_t>(k % 100000000LL / 1000000LL));
  }
  if (k > 10000LL) {
    cs.push_back(static_cast<std::uint8_t>(k % 1000000LL / 10000LL));
  }
  if (k > 100LL) {
    cs.push_back(static_cast<std::uint8_t>(k % 10000LL / 100LL));
  }
  if (k > 1LL) {
    cs.push_back(static_cast<std::uint8_t>(k % 100LL / 1LL));
  }
  return cs;
}

std::vector<Poke> key_to_pokes(std::int64_t key) { return bytes_to_pokes(key_to_bytes(key)); }

std::string key_to_str(std::int64_t key) {
  std::string ret;
  for (const Poke& p : key_to_pokes(key)) {
    ret += p.to_string();
  }
  return ret;
}

std::shared_ptr<const std::unordered_map<int, KeyData>> normal_map() {
  return std::atomic_load(&g_normal_map);
}

bool load_from_stream(std::istream& in) {
  auto m = std::make_shared<std::unordered_map<int, KeyData>>();
  std::string line;
  while (std::getline(in, line)) {
    if (!line.empty() && line.back() == '\r') {
      line.pop_back();
    }
    if (line.empty()) {
      continue;
    }
    const std::vector<std::string> fields = split(line, ' ');
    if (fields.size() < 7) {
      return false;
    }
    int key = 0;
    int pos = 0;
    int typ = 0;
    int max = 0;
    try {
      key = std::stoi(fields[0]);
      pos = std::stoi(fields[1]);
      typ = std::stoi(fields[5]);
      max = std::stoi(fields[6]);
    } catch (const std::exception&) {
      return false;
    }
    (*m)[key] = KeyData{pos, typ, max};
  }
  if (in.bad()) {
    return false;
  }
  std::atomic_store(&g_normal_map,
                    std::shared_ptr<const std::unordered_map<int, KeyData>>(m));
  return true;
}

bool load_from_file(const std::string& path) {
  std::ifstream f(path, std::ios::binary);
  if (!f) {
    return false;
  }
  return load_from_stream(f);
}

bool load() {
#if defined(TEENPATTI_HAS_EMBEDDED_DATA)
  {
    const std::string data(teenpatti_embedded_data_start,
                           teenpatti_embedded_data_end - teenpatti_embedded_data_start);
    std::istringstream iss(data);
    if (load_from_stream(iss)) {
      return true;
    }
  }
#endif
  std::ifstream f(kDataFileName, std::ios::binary);
  if (!f) {
    return false;
  }
  return load_from_stream(f);
}

std::uint8_t str_to_poke_value(const std::string& s) {
  if (s == "A") {
    return PokeValueA;
  }
  if (s == "K") {
    return PokeValueK;
  }
  if (s == "Q") {
    return PokeValueQ;
  }
  if (s == "J") {
    return PokeValueJ;
  }
  const bool digits = !s.empty() && std::all_of(s.begin(), s.end(), [](unsigned char c) {
                        return c >= '0' && c <= '9';
                      });
  if (!digits) {
    return 0;
  }
  const int v = std::stoi(s);
  if (v < 0 || v > 127) {
    return 0;
  }
  return static_cast<std::uint8_t>(v);
}

std::uint8_t str_to_poke(const std::string& s) {
  if (starts_with(s, "方")) {
    return Poke{PokeColorFang, str_to_poke_value(s.substr(sizeof("方") - 1))}.to_byte();
  }
  if (starts_with(s, "梅")) {
    return Poke{PokeColorMei, str_to_poke_value(s.substr(sizeof("梅") - 1))}.to_byte();
  }
  if (starts_with(s, "红")) {
    return Poke{PokeColorHong, str_to_poke_value(s.substr(sizeof("红") - 1))}.to_byte();
  }
  if (starts_with(s, "黑")) {
    return Poke{PokeColorHei, str_to_poke_value(s.substr(sizeof("黑") - 1))}.to_byte();
  }
  if (starts_with(s, "鬼")) {
    return kGuiPoke.to_byte();
  }
  return 0;
}

std::vector<std::uint8_t> str_to_pokes(const std::string& s) {
  std::vector<std::uint8_t> ret;
  if (s.empty()) {
    return ret;
  }
  const std::vector<std::string> strs = split(s, ',');
  ret.reserve(strs.size());
  for (const std::string& str : strs) {
    ret.push_back(str_to_poke(str));
  }
  return ret;
}

std::optional<KeyData> get_key_data(const std::string& cards) {
  std::vector<std::uint8_t> pokes = str_to_pokes(cards);
  if (pokes.size() != 7) {
    return std::nullopt;
  }
  return get_key_data(std::move(pokes));
}

std::optional<KeyData> get_key_data(std::vector<std::uint8_t> pokes) {
  std::sort(pokes.begin(), pokes.end());
  return get_key_data(gen_card_bind(pokes));
}

std::optional<KeyData> get_key_data(int key) {
  const std::shared_ptr<const std::unordered_map<int, KeyData>> snapshot = normal_map();
  const auto it = snapshot->find(key);
  if (it == snapshot->end()) {
    return std::nullopt;
  }
  return it->second;
}

int get_win_position(const std::string& cards) { return get_win_position(str_to_pokes(cards)); }

int get_win_position(std::vector<std::uint8_t> pokes) {
  const auto kd = get_key_data(std::move(pokes));
  if (!kd) {
    return 0;
  }
  return kd->position;
}

int get_win_type(const std::string& cards) { return get_win_type(str_to_pokes(cards)); }

int get_win_type(std::vector<std::uint8_t> pokes) {
  const auto kd = get_key_data(std::move(pokes));
  if (!kd) {
    return 0;
  }
  return kd->type;
}

int get_max(const std::string& cards) { return get_max(str_to_pokes(cards)); }

int get_max(std::vector<std::uint8_t> pokes) {
  const auto kd = get_key_data(std::move(pokes));
  if (!kd) {
    return 0;
  }
  return kd->max;
}

int compare(const std::string& a, const std::string& b) {
  return compare(str_to_pokes(a), str_to_pokes(b));
}

int compare(std::vector<std::uint8_t> a, std::vector<std::uint8_t> b) {
  std::sort(a.begin(), a.end());
  std::sort(b.begin(), b.end());
  return compare(gen_card_bind(a), gen_card_bind(b));
}

int compare(int k1, int k2) {
  const auto kd1 = get_key_data(k1);
  const auto kd2 = get_key_data(k2);
  if (!kd1 && !kd2) {
    return 0;
  }
  if (!kd1) {
    return -1;
  }
  if (!kd2) {
    return 1;
  }
  return kd1->position - kd2->position;
}

void gen_key() {
  g_gen_begin.store(now_ms());
  g_gen_keys.clear();
  g_gen_keys.reserve(kGenTotal);
  g_gen_key_seen.clear();
  g_gen_total_key.store(0);
  g_gen_last_print.store(0);
  g_gen_progress.store(0);

  std::vector<int> tmp(3);
  permutation([](const std::vector<int>& t) { gen_card_save(t); }, gen_all_cards(), 0, 0, 3, tmp);
  std::printf("genKey finish %d\n", kGenTotal);
}

const std::vector<int>& gen_keys() { return g_gen_keys; }

void quicksort(std::vector<int>& values) {
  QuickSorter sorter{values, static_cast<int>(values.size()),
                     static_cast<int>(std::thread::hardware_concurrency()), {}, {}, {}};
  sorter.spawn(0, static_cast<int>(values.size()) - 1, 0);
  std::unique_lock<std::mutex> lock(sorter.wait_mutex);
  sorter.wait_cv.wait(lock, [&sorter] { return sorter.tasks.load() == 0; });
}

bool output_data() {
  const long long begin = now_ms();

  std::ofstream f(kDataFileName, std::ios::binary | std::ios::trunc);
  if (!f) {
    return false;
  }

  g_gen_begin.store(now_ms());
  g_gen_last_print.store(0);

  quicksort(g_gen_keys);

  g_gen_total_key.store(0);
  g_gen_last_print.store(0);
  g_gen_begin.store(now_ms());

  int i = 0;
  int iindex = 0;
  int last_max = 0;
  const int total_keys = static_cast<int>(g_gen_keys.size());
  for (int index = 0; index < total_keys; ++index) {
    const int k = g_gen_keys[index];
    if (last_max == 0) {
      iindex = index;
    } else if (!gen_equal(last_max, k)) {
      ++i;
      iindex = index;
    }
    last_max = k;

    const int max_key = gen_max(g_gen_keys[index]);
    std::ostringstream oss;
    oss << k << ' ' << i << ' ' << iindex << ' ' << total_keys << ' '
        << key_to_str(g_gen_keys[index]) << ' ' << gen_max_type(g_gen_keys[index]) << ' '
        << max_key << ' ' << key_to_str(max_key) << '\n';
    f << oss.str();

    const int cur = (index + 1) * 100 / total_keys;
    if (cur != g_gen_last_print.load()) {
      g_gen_last_print.store(cur);
      const long long now = now_ms();
      const double per = static_cast<double>(now - g_gen_begin.load()) / (index + 1);
      std::printf("%d%% 需要%.1f分 用时%.1f分 速度%.1f条/秒\n", cur,
                  per * (total_keys - index - 1) / 60.0 / 1000.0,
                  (now - g_gen_begin.load()) / 60.0 / 1000.0,
                  (index + 1) / ((now - g_gen_begin.load()) / 1000.0));
    }
  }

  f.flush();
  if (!f.good()) {
    return false;
  }
  f.close();

  std::printf("outputData finish %d time:%lld分 %lld\n", total_keys,
              (now_ms() - begin) / 1000 / 60, g_gen_progress.load());
  return true;
}

}  // namespace teenpatti

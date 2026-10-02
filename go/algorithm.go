package teenpatti

import (
	"bufio"
	"bytes"
	"embed"
	"fmt"
	"io"
	"os"
	"sort"
	"strconv"
	"strings"
	"sync/atomic"
)

//go:embed teenpatti_data.txt
var teenPattiData embed.FS

// KeyData is the lookup result for one hand: its global rank among all
// possible hands, the type of its best resolved hand, and the encoded key of
// that best resolved hand.
type KeyData struct {
	Position int
	Type     int
	Max      int
}

var normalMap atomic.Pointer[map[int]KeyData]

func init() {
	m := make(map[int]KeyData)
	normalMap.Store(&m)
}

// NormalMap returns the currently loaded key table. The returned map is
// shared and must be treated as read-only; Load and LoadFromFile replace it
// atomically.
func NormalMap() map[int]KeyData {
	return *normalMap.Load()
}

// Load parses the embedded teenpatti_data.txt into the key table. Call it
// once at startup before using the query functions.
func Load() error {
	data, err := teenPattiData.ReadFile("teenpatti_data.txt")
	if err != nil {
		return err
	}
	return loadFrom(bytes.NewReader(data))
}

// LoadFromFile parses an external teenpatti_data.txt into the key table.
func LoadFromFile(path string) error {
	f, err := os.Open(path)
	if err != nil {
		return err
	}
	defer f.Close()
	return loadFrom(f)
}

func loadFrom(r io.Reader) error {
	m := make(map[int]KeyData)
	scanner := bufio.NewScanner(r)
	scanner.Buffer(make([]byte, 64*1024), 1024*1024)
	for scanner.Scan() {
		line := strings.TrimRight(scanner.Text(), "\r")
		if line == "" {
			continue
		}
		fields := strings.Split(line, " ")
		if len(fields) < 7 {
			return fmt.Errorf("malformed teenpatti_data line: %q", line)
		}
		key, err := strconv.Atoi(fields[0])
		if err != nil {
			return fmt.Errorf("bad key in teenpatti_data line %q: %w", line, err)
		}
		pos, err := strconv.Atoi(fields[1])
		if err != nil {
			return fmt.Errorf("bad position in teenpatti_data line %q: %w", line, err)
		}
		typ, err := strconv.Atoi(fields[5])
		if err != nil {
			return fmt.Errorf("bad type in teenpatti_data line %q: %w", line, err)
		}
		max, err := strconv.Atoi(fields[6])
		if err != nil {
			return fmt.Errorf("bad max in teenpatti_data line %q: %w", line, err)
		}
		m[key] = KeyData{Position: pos, Type: typ, Max: max}
	}
	if err := scanner.Err(); err != nil {
		return err
	}
	normalMap.Store(&m)
	return nil
}

// StrToPokeValue parses a card value token ("A", "K", "Q", "J" or "2".."10").
// Returns 0 when the token is not a valid value.
func StrToPokeValue(s string) byte {
	switch s {
	case "A":
		return PokeValueA
	case "K":
		return PokeValueK
	case "Q":
		return PokeValueQ
	case "J":
		return PokeValueJ
	}
	v, err := strconv.ParseInt(s, 10, 8)
	if err != nil {
		return 0
	}
	return byte(v)
}

// StrToPoke parses a single card token like "黑A" or "鬼".
// Returns 0 when the token is not a valid card.
func StrToPoke(s string) byte {
	switch {
	case strings.HasPrefix(s, "方"):
		return NewPoke(PokeColorFang, StrToPokeValue(s[len("方"):])).ToByte()
	case strings.HasPrefix(s, "梅"):
		return NewPoke(PokeColorMei, StrToPokeValue(s[len("梅"):])).ToByte()
	case strings.HasPrefix(s, "红"):
		return NewPoke(PokeColorHong, StrToPokeValue(s[len("红"):])).ToByte()
	case strings.HasPrefix(s, "黑"):
		return NewPoke(PokeColorHei, StrToPokeValue(s[len("黑"):])).ToByte()
	case strings.HasPrefix(s, "鬼"):
		return NewPoke(GUI.Color, GUI.Value).ToByte()
	}
	return 0
}

// StrToPokes parses a comma separated hand like "黑A,方A,鬼".
func StrToPokes(s string) []byte {
	if len(s) == 0 {
		return nil
	}
	strs := strings.Split(s, ",")
	ret := make([]byte, 0, len(strs))
	for _, str := range strs {
		ret = append(ret, StrToPoke(str))
	}
	return ret
}

// KeyToBytes unpacks an integer key into its card bytes, most significant
// digit first (i.e. the ascending card order the key was built from).
func KeyToBytes(k int) []byte {
	u := int64(k)
	var cs []byte
	if u > 1000000000000 {
		cs = append(cs, byte(u%100000000000000/1000000000000))
	}
	if u > 10000000000 {
		cs = append(cs, byte(u%1000000000000/10000000000))
	}
	if u > 100000000 {
		cs = append(cs, byte(u%10000000000/100000000))
	}
	if u > 1000000 {
		cs = append(cs, byte(u%100000000/1000000))
	}
	if u > 10000 {
		cs = append(cs, byte(u%1000000/10000))
	}
	if u > 100 {
		cs = append(cs, byte(u%10000/100))
	}
	if u > 1 {
		cs = append(cs, byte(u%100/1))
	}
	return cs
}

// GetKeyData looks up a hand given as a comma separated string.
// For compatibility with the Java implementation this requires exactly seven
// cards and returns nil otherwise; use GetKeyDataByCards for 3-card hands.
func GetKeyData(cards string) *KeyData {
	pokes := StrToPokes(cards)
	if len(pokes) != 7 {
		return nil
	}
	return GetKeyDataByCards(pokes)
}

// GetKeyDataByCards sorts pokes ascending in place (as the Java version
// does), encodes them into a key and looks the key up in the table.
// Returns nil when the hand is not in the table.
func GetKeyDataByCards(pokes []byte) *KeyData {
	sort.SliceStable(pokes, func(i, j int) bool { return pokes[i] < pokes[j] })
	key := GenCardBind(pokes)
	return GetKeyDataByKey(key)
}

// GetKeyDataByKey returns the table entry for an encoded key, or nil when
// the key is not in the table.
func GetKeyDataByKey(key int) *KeyData {
	m := *normalMap.Load()
	kd, ok := m[key]
	if !ok {
		return nil
	}
	return &kd
}

// GetWinPosition returns the global rank of a hand, higher is stronger.
// Returns 0 when the hand is not in the table.
func GetWinPosition(cards string) int {
	return GetWinPositionByCards(StrToPokes(cards))
}

// GetWinPositionByCards is GetWinPosition for an already parsed hand.
func GetWinPositionByCards(pokes []byte) int {
	kd := GetKeyDataByCards(pokes)
	if kd == nil {
		return 0
	}
	return kd.Position
}

// GetWinType returns the type of a hand's best resolved hand, one of the
// CardType constants. Returns 0 when the hand is not in the table.
func GetWinType(cards string) int {
	return GetWinTypeByCards(StrToPokes(cards))
}

// GetWinTypeByCards is GetWinType for an already parsed hand.
func GetWinTypeByCards(pokes []byte) int {
	kd := GetKeyDataByCards(pokes)
	if kd == nil {
		return 0
	}
	return kd.Type
}

// GetMax returns the encoded key of a hand's best resolved hand.
// Returns 0 when the hand is not in the table.
func GetMax(cards string) int {
	return GetMaxByCards(StrToPokes(cards))
}

// GetMaxByCards is GetMax for an already parsed hand.
func GetMaxByCards(pokes []byte) int {
	kd := GetKeyDataByCards(pokes)
	if kd == nil {
		return 0
	}
	return kd.Max
}

// Compare compares two hands given as comma separated card strings.
// Returns a positive value when the first hand wins, 0 on a tie and a
// negative value when the second hand wins.
func Compare(a, b string) int {
	return CompareByCards(StrToPokes(a), StrToPokes(b))
}

// CompareByCards is Compare for already parsed hands. Both slices are sorted
// ascending in place, as the Java version does.
func CompareByCards(a, b []byte) int {
	sort.SliceStable(a, func(i, j int) bool { return a[i] < a[j] })
	sort.SliceStable(b, func(i, j int) bool { return b[i] < b[j] })
	return CompareByKey(GenCardBind(a), GenCardBind(b))
}

// CompareByKey compares two encoded keys. Returns a positive value when the
// first key ranks higher, 0 on a tie and a negative value when the second
// key ranks higher; missing keys sort lowest.
func CompareByKey(k1, k2 int) int {
	kd1 := GetKeyDataByKey(k1)
	kd2 := GetKeyDataByKey(k2)
	if kd1 == nil && kd2 == nil {
		return 0
	}
	if kd1 == nil {
		return -1
	}
	if kd2 == nil {
		return 1
	}
	return kd1.Position - kd2.Position
}

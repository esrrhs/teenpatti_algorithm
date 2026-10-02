package teenpatti

import "sort"

// Card type constants, ordered from lowest to highest ranking.
// Three of a Kind outranks Straight Flush in Teen Patti.
const (
	CardTypeUnknown     = 0 // 未知
	CardTypeGaoPai      = 1 // 高牌 high card
	CardTypeDuiZi       = 2 // 对子 pair
	CardTypeTongHua     = 3 // 同花 flush
	CardTypeShunZi      = 4 // 顺子 straight
	CardTypeTongHuaShun = 5 // 同花顺 straight flush
	CardTypeSanTiao     = 6 // 三条 three of a kind
)

// GetCardTypeUnordered returns the type of a 3-card hand regardless of card
// order, or CardTypeUnknown when the hand does not hold exactly 3 cards.
func GetCardTypeUnordered(cards []Poke) int {
	if len(cards) != 3 {
		return CardTypeUnknown
	}
	tmp := make([]Poke, 3)
	copy(tmp, cards)
	sort.SliceStable(tmp, func(i, j int) bool { return tmp[i].Value < tmp[j].Value })
	value1 := tmp[0].Value
	value2 := tmp[1].Value
	value3 := tmp[2].Value

	color1 := tmp[0].Color
	color2 := tmp[1].Color
	color3 := tmp[2].Color

	if value1 == value2 && value2 == value3 {
		return CardTypeSanTiao
	}
	if value1 == value2 || value2 == value3 {
		return CardTypeDuiZi
	}

	sameColor := color1 == color2 && color1 == color3

	shunZi := value2 == value1+1 && value3 == value2+1
	if value1 == PokeValue2 && value2 == PokeValue3 && value3 == PokeValueA {
		shunZi = true
	}

	if sameColor && shunZi {
		return CardTypeTongHuaShun
	}
	if sameColor {
		return CardTypeTongHua
	}
	if shunZi {
		return CardTypeShunZi
	}
	return CardTypeGaoPai
}

// PokeToBytes converts cards to their packed byte forms.
func PokeToBytes(cards []Poke) []byte {
	ret := make([]byte, 0, len(cards))
	for _, p := range cards {
		ret = append(ret, p.ToByte())
	}
	return ret
}

// BytesToPokes converts packed card bytes back to cards.
func BytesToPokes(cards []byte) []Poke {
	ret := make([]Poke, 0, len(cards))
	for _, b := range cards {
		ret = append(ret, NewPokeFromByte(b))
	}
	return ret
}

// MaxCards resolves Jokers in the hand to their best possible substitution
// and returns the strongest hand. Hands without Jokers are returned as a copy.
func MaxCards(cards []Poke) []Poke {
	ret := make([]Poke, 0, len(cards))
	ret = append(ret, cards...)

	gui := 0
	for _, p := range cards {
		if p.IsGui() {
			gui++
		}
	}
	if gui == 0 {
		return ret
	}

	var left []Poke
	leftMap := make(map[byte]struct{})
	for _, p := range cards {
		if !p.IsGui() {
			left = append(left, p)
			leftMap[p.ToByte()] = struct{}{}
		}
	}

	var max []Poke
	tmp := make([]int, gui)
	permutation(func(t []int) {
		for _, x := range t {
			if _, ok := leftMap[byte(x)]; ok || byte(x) == GUI.ToByte() {
				return
			}
		}

		last := make([]Poke, 0, len(left)+len(t))
		last = append(last, left...)
		for _, x := range t {
			last = append(last, NewPokeFromByte(byte(x)))
		}
		if len(max) == 0 || CompareCardsWithoutGui(last, max) > 0 {
			max = last
		}
	}, genAllCardsList, 0, 0, gui, tmp)
	return max
}

// CompareCards compares two hands, resolving Jokers to their best possible
// substitution first. Returns 1 win, 0 tie, -1 lose.
func CompareCards(firstCards, secondCards []Poke) int {
	f := MaxCards(firstCards)
	s := MaxCards(secondCards)
	return CompareCardsWithoutGui(f, s)
}

// CompareCardsWithoutGui compares two hands that contain no Jokers.
// Returns 1 win, 0 tie, -1 lose.
func CompareCardsWithoutGui(firstCards, secondCards []Poke) int {
	firstCardType := GetCardTypeUnordered(firstCards)
	secondCardType := GetCardTypeUnordered(secondCards)
	if firstCardType != secondCardType {
		if firstCardType > secondCardType {
			return 1
		}
		return -1
	}
	return compareByValue(firstCards, secondCards)
}

func compareByValue(firstCards, secondCards []Poke) int {
	cardType := GetCardTypeUnordered(firstCards)
	if cardType == CardTypeUnknown {
		return 0
	}
	sortCards1 := make([]Poke, len(firstCards))
	copy(sortCards1, firstCards)
	sortCards2 := make([]Poke, len(secondCards))
	copy(sortCards2, secondCards)
	sort.SliceStable(sortCards1, func(i, j int) bool { return sortCards1[i].Value < sortCards1[j].Value })
	sort.SliceStable(sortCards2, func(i, j int) bool { return sortCards2[i].Value < sortCards2[j].Value })
	if cardType == CardTypeDuiZi {
		return compareDuiZi(sortCards1, sortCards2)
	}
	return compareNotDuiZi(sortCards1, sortCards2)
}

// compareDuiZi compares two pair hands, both sorted ascending by value.
// The pair is moved to the front, then pair value and kicker are compared.
func compareDuiZi(sortCards1, sortCards2 []Poke) int {
	if sortCards1[0].Value != sortCards1[1].Value {
		sortCards1[0], sortCards1[2] = sortCards1[2], sortCards1[0]
	}
	if sortCards2[0].Value != sortCards2[1].Value {
		sortCards2[0], sortCards2[2] = sortCards2[2], sortCards2[0]
	}
	value1 := sortCards1[0].Value
	value3 := sortCards1[2].Value

	value11 := sortCards2[0].Value
	value31 := sortCards2[2].Value

	if value1 != value11 {
		if value1 > value11 {
			return 1
		}
		return -1
	}
	if value3 != value31 {
		if value3 > value31 {
			return 1
		}
		return -1
	}
	return 0
}

// compareNotDuiZi compares two non-pair hands, both sorted ascending by
// value: highest card first, then middle, then lowest.
func compareNotDuiZi(sortCards1, sortCards2 []Poke) int {
	for i := 2; i >= 0; i-- {
		if sortCards1[i].Value != sortCards2[i].Value {
			if sortCards1[i].Value > sortCards2[i].Value {
				return 1
			}
			return -1
		}
	}
	return 0
}

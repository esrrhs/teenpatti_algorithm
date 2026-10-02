// Package teenpatti is a Go port of the Java teenpatti_algorithm library
// (https://github.com/esrrhs/teenpatti_algorithm). It evaluates 3-card
// Teen Patti hands through a pre-computed lookup table, with full support
// for Joker (wild card) hands, and can regenerate the lookup table itself.
package teenpatti

// Card values, 2 through A.
const (
	PokeValue2  byte = 2
	PokeValue3  byte = 3
	PokeValue4  byte = 4
	PokeValue5  byte = 5
	PokeValue6  byte = 6
	PokeValue7  byte = 7
	PokeValue8  byte = 8
	PokeValue9  byte = 9
	PokeValue10 byte = 10
	PokeValueJ  byte = 11
	PokeValueQ  byte = 12
	PokeValueK  byte = 13
	PokeValueA  byte = 14
)

// PokeValues lists all regular card values in ascending order.
var PokeValues = []byte{
	PokeValue2, PokeValue3, PokeValue4, PokeValue5, PokeValue6, PokeValue7,
	PokeValue8, PokeValue9, PokeValue10, PokeValueJ, PokeValueQ, PokeValueK,
	PokeValueA,
}

// Card colors (suits).
const (
	PokeColorFang byte = 0 // 方 diamonds
	PokeColorMei  byte = 1 // 梅 clubs
	PokeColorHong byte = 2 // 红 hearts
	PokeColorHei  byte = 3 // 黑 spades
)

// GUI is the Joker (wild card) sentinel poke.
var GUI = Poke{Color: 5, Value: 8}

var huaseName = [...]string{"方", "梅", "红", "黑"}
var valueName = [...]string{"", "", "2", "3", "4", "5", "6", "7", "8", "9", "10", "J", "Q", "K", "A"}

// Poke is a single card.
type Poke struct {
	Color byte
	Value byte
}

// NewPoke creates a card from a color and a value.
func NewPoke(color, value byte) Poke {
	return Poke{Color: color, Value: value}
}

// NewPokeFromByte unpacks a card from its packed byte form.
func NewPokeFromByte(b byte) Poke {
	return Poke{Color: b >> 4, Value: b % 16}
}

// IsGuiByte reports whether the packed byte is the Joker sentinel.
func IsGuiByte(i byte) bool {
	return i%16 == GUI.Value && i>>4 == GUI.Color
}

// IsGui reports whether the card is the Joker.
func (p Poke) IsGui() bool {
	return p.Value == GUI.Value && p.Color == GUI.Color
}

// ToByte packs the card into one byte: (color << 4) | value.
func (p Poke) ToByte() byte {
	return p.Color<<4 | p.Value
}

// String returns the human readable form, e.g. "黑A" or "鬼".
func (p Poke) String() string {
	if p.IsGui() {
		return "鬼"
	}
	return huaseName[p.Color] + valueName[p.Value]
}

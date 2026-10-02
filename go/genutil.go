package teenpatti

import (
	"bufio"
	"fmt"
	"os"
	"sort"
	"strings"
	"sync/atomic"
	"time"
)

// Deck layout used by the generation pipeline: 52 regular cards + 3 Jokers.
const (
	guiNum   = 3
	genNum   = 52 + guiNum
	genTotal = (genNum * (genNum - 1) * (genNum - 2)) / (3 * 2) // C(55, 3)
)

// GenCardBind encodes an ordered card byte slice into its integer key: each
// card byte becomes two decimal digits, first card most significant.
func GenCardBind(cards []byte) int {
	ret := 0
	for _, b := range cards {
		ret = ret*100 + int(b)
	}
	return ret
}

// GenPokeCardBind encodes an ordered Poke slice into its integer key.
func GenPokeCardBind(cards []Poke) int {
	ret := 0
	for _, p := range cards {
		ret = ret*100 + int(p.ToByte())
	}
	return ret
}

func genCardBindInts(cards []int) int {
	ret := 0
	for _, v := range cards {
		ret = ret*100 + v
	}
	return ret
}

// KeyToPokes unpacks an integer key back into its cards, most significant
// digit first (i.e. the ascending card order the key was built from).
func KeyToPokes(k int) []Poke {
	u := int64(k)
	var cs []Poke
	if u > 1000000000000 {
		cs = append(cs, NewPokeFromByte(byte(u%100000000000000/1000000000000)))
	}
	if u > 10000000000 {
		cs = append(cs, NewPokeFromByte(byte(u%1000000000000/10000000000)))
	}
	if u > 100000000 {
		cs = append(cs, NewPokeFromByte(byte(u%10000000000/100000000)))
	}
	if u > 1000000 {
		cs = append(cs, NewPokeFromByte(byte(u%100000000/1000000)))
	}
	if u > 10000 {
		cs = append(cs, NewPokeFromByte(byte(u%1000000/10000)))
	}
	if u > 100 {
		cs = append(cs, NewPokeFromByte(byte(u%10000/100)))
	}
	if u > 1 {
		cs = append(cs, NewPokeFromByte(byte(u%100/1)))
	}
	return cs
}

// KeyToStr renders an integer key as the concatenated readable cards.
func KeyToStr(k int) string {
	var sb strings.Builder
	for _, p := range KeyToPokes(k) {
		sb.WriteString(p.String())
	}
	return sb.String()
}

var genAllCardsList = genAllCards()

func genAllCards() []int {
	list := make([]int, 0, genNum)
	for i := byte(0); i < 4; i++ {
		for j := byte(0); j < genNum/4; j++ {
			list = append(list, int(NewPoke(i, j+2).ToByte()))
		}
	}
	for i := 0; i < guiNum; i++ {
		list = append(list, int(GUI.ToByte()))
	}
	sort.Ints(list)
	return list
}

type permutationRun func(tmp []int)

// permutation enumerates all size-`except` combinations of a in order.
func permutation(run permutationRun, a []int, count, count2, except int, tmp []int) {
	if count2 == except {
		run(tmp)
		return
	}
	for i := count; i < len(a); i++ {
		tmp[count2] = a[i]
		permutation(run, a, i+1, count2+1, except, tmp)
	}
}

// Generation pipeline state, mirroring the statics of the Java GenUtil.
var (
	genTotalKey  atomic.Int64
	genProgress  atomic.Int64
	genLastPrint atomic.Int64
	genBegin     atomic.Int64

	genKeys    []int
	genKeySeen map[int]struct{}
)

// GenKey enumerates all C(55,3) hands and collects the distinct keys,
// unsorted, in enumeration order.
func GenKey() {
	genBegin.Store(time.Now().UnixMilli())
	genKeys = make([]int, 0, genTotal)
	genKeySeen = make(map[int]struct{}, genTotal)
	genTotalKey.Store(0)
	genLastPrint.Store(0)

	genCard()

	fmt.Println("genKey finish", genTotal)
}

func genCard() {
	list := genAllCards()
	run := func(tmp []int) {
		genCardSave(tmp)
	}
	tmp := make([]int, 3)
	permutation(run, list, 0, 0, 3, tmp)
}

func genCardSave(tmp []int) {
	c := genCardBindInts(tmp)
	if _, ok := genKeySeen[c]; !ok {
		genKeySeen[c] = struct{}{}
		genKeys = append(genKeys, c)
	}
	totalKey := genTotalKey.Add(1)

	cur := int(totalKey * 100 / genTotal)
	if cur != int(genLastPrint.Load()) {
		genLastPrint.Store(int64(cur))

		now := time.Now().UnixMilli()
		per := float64(now-genBegin.Load()) / float64(totalKey)
		fmt.Printf("%d%% 需要%.1f分 用时%.1f分 速度%.1f条/秒\n",
			cur,
			per*float64(genTotal-totalKey)/60/1000,
			float64(now-genBegin.Load())/60/1000,
			float64(totalKey)/(float64(now-genBegin.Load())/1000))
	}
}

// genMax returns the encoded key of the best resolved hand for k.
func genMax(k int) int {
	cs := KeyToPokes(k)
	m := MaxCards(cs)
	return GenPokeCardBind(m)
}

// genMaxType returns the card type of the best resolved hand for k.
func genMaxType(k int) int {
	cs := KeyToPokes(k)
	m := MaxCards(cs)
	return GetCardTypeUnordered(m)
}

func genCompare(k1, k2 int) bool {
	cs1 := KeyToPokes(k1)
	cs2 := KeyToPokes(k2)
	return CompareCards(cs1, cs2) < 0
}

func genEqual(k1, k2 int) bool {
	cs1 := KeyToPokes(k1)
	cs2 := KeyToPokes(k2)
	return CompareCards(cs1, cs2) == 0
}

// OutputData sorts the keys collected by GenKey by hand strength and writes
// teenpatti_data.txt into the working directory.
func OutputData() error {
	begin := time.Now().UnixMilli()

	f, err := os.Create("teenpatti_data.txt")
	if err != nil {
		return err
	}
	w := bufio.NewWriter(f)

	genBegin.Store(time.Now().UnixMilli())
	genLastPrint.Store(0)

	Quicksort(genKeys)

	genTotalKey.Store(0)
	genLastPrint.Store(0)
	genBegin.Store(time.Now().UnixMilli())

	i := 0
	iindex := 0
	lastMax := 0
	for index, k := range genKeys {
		if lastMax == 0 {
			iindex = index
		} else if !genEqual(lastMax, k) {
			i++
			iindex = index
		}
		lastMax = k

		str := fmt.Sprintf("%d %d %d %d %s %d %d %s\n",
			k, i, iindex, len(genKeys),
			KeyToStr(genKeys[index]),
			genMaxType(genKeys[index]),
			genMax(genKeys[index]),
			KeyToStr(genMax(genKeys[index])))
		if _, err := w.WriteString(str); err != nil {
			f.Close()
			return err
		}

		cur := (index + 1) * 100 / len(genKeys)
		if cur != int(genLastPrint.Load()) {
			genLastPrint.Store(int64(cur))

			now := time.Now().UnixMilli()
			per := float64(now-genBegin.Load()) / float64(index+1)
			fmt.Printf("%d%% 需要%.1f分 用时%.1f分 速度%.1f条/秒\n",
				cur,
				per*float64(len(genKeys)-index-1)/60/1000,
				float64(now-genBegin.Load())/60/1000,
				float64(index+1)/(float64(now-genBegin.Load())/1000))
		}
	}

	if err := w.Flush(); err != nil {
		f.Close()
		return err
	}
	if err := f.Close(); err != nil {
		return err
	}

	fmt.Printf("outputData finish %d time:%d分 %d\n",
		len(genKeys), (time.Now().UnixMilli()-begin)/1000/60, genProgress.Load())
	return nil
}

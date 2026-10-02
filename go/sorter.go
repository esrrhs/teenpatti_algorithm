package teenpatti

import (
	"fmt"
	"math"
	"runtime"
	"sync"
	"sync/atomic"
	"time"
)

const sorterFallback = 2

// Quicksort sorts input in place by hand strength, weakest first, using a
// parallel quicksort: the same algorithm as the Java Sorter. Partitions are
// sorted on separate goroutines while the number of active tasks is below
// fallback * NumCPU, and fall back to inline recursion above that.
func Quicksort(input []int) {
	s := &sorter{values: input, total: len(input), nThreads: runtime.NumCPU()}
	s.quicksortTask(0, len(input)-1, 0)
	s.wg.Wait()
}

type sorter struct {
	values   []int
	total    int
	nThreads int
	tasks    atomic.Int32
	wg       sync.WaitGroup
}

func (s *sorter) quicksortTask(left, right, layer int) {
	s.tasks.Add(1)
	s.wg.Add(1)
	go func() {
		defer s.wg.Done()
		defer s.tasks.Add(-1)
		s.quicksort(layer, left, right)
	}()
}

func (s *sorter) quicksort(layer, lowerIndex, higherIndex int) {
	if higherIndex < lowerIndex {
		return
	}
	if higherIndex == lowerIndex {
		s.printLeafProgress()
		return
	}

	i := lowerIndex
	j := higherIndex
	// pivot value taken from the middle index
	pivot := s.values[lowerIndex+(higherIndex-lowerIndex)/2]

	totalStep := j - i
	if totalStep == 0 {
		totalStep = 1
	}
	lastPrint := 0
	beginPrint := time.Now().UnixMilli()

	for i <= j {
		for genCompare(s.values[i], pivot) {
			i++
		}
		for genCompare(pivot, s.values[j]) {
			j--
		}
		if i <= j {
			s.values[i], s.values[j] = s.values[j], s.values[i]
			i++
			j--
		}

		if i <= j && totalStep > 100000 {
			step := totalStep - (j - i)
			if step < 0 {
				step = 0
			}
			cur := step * 100 / totalStep
			if cur != lastPrint {
				lastPrint = cur

				now := time.Now().UnixMilli()
				per := float64(now-beginPrint) / float64(step)
				fmt.Printf("%d/%d层 %d%% 需要%.1f分 用时%.1f分 速度%.1f条/秒\n",
					layer, int(math.Log(float64(s.total))), cur,
					per*float64(totalStep-step)/60/1000,
					float64(now-beginPrint)/60/1000,
					float64(step)/(float64(now-beginPrint)/1000))
			}
		}
	}

	if s.tasks.Load() >= int32(sorterFallback*s.nThreads) {
		if i-j == 1 {
			s.quicksort(layer+1, lowerIndex, j)
			s.quicksort(layer+1, i, higherIndex)
		} else {
			s.quicksort(layer+1, lowerIndex, j+1)
			s.quicksort(layer+1, i, higherIndex)
		}
	} else {
		if i-j == 1 {
			s.quicksortTask(lowerIndex, j, layer+1)
			s.quicksortTask(i, higherIndex, layer+1)
		} else {
			s.quicksortTask(lowerIndex, j+1, layer+1)
			s.quicksortTask(i, higherIndex, layer+1)
		}
	}
}

func (s *sorter) printLeafProgress() {
	step := genProgress.Add(1)
	cur := int(step * 10000 / int64(s.total))
	if cur != int(genLastPrint.Load()) {
		genLastPrint.Store(int64(cur))

		now := time.Now().UnixMilli()
		per := float64(now-genBegin.Load()) / float64(step)
		fmt.Printf("%d%%%% 需要%.1f分 用时%.1f分 速度%.1f条/秒\n",
			cur,
			per*float64(int64(s.total)-step)/60/1000,
			float64(now-genBegin.Load())/60/1000,
			float64(step)/(float64(now-genBegin.Load())/1000))
	}
}

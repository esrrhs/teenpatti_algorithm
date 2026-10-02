package teenpatti

import (
	"os"
	"strings"
	"testing"
)

// TestMain mirrors the @BeforeAll setup of the Java test: load the lookup
// table and assert it is not empty.
func TestMain(m *testing.M) {
	if err := Load(); err != nil {
		panic(err)
	}
	if len(NormalMap()) == 0 {
		panic("normalMap should not be empty after load")
	}
	os.Exit(m.Run())
}

// TestHandEvaluation mirrors Java TeenPattiAlgorithmUtilTest.testHandEvaluation.
func TestHandEvaluation(t *testing.T) {
	cards := "黑A,方A,鬼"
	cards1 := "黑A,鬼,方3"

	winTypeCards := GetWinType(cards)
	winTypeCards1 := GetWinType(cards1)
	_ = winTypeCards1

	// "黑A,方A,鬼" can form AAA (Trail / Three of a kind, type 6)
	if winTypeCards != 6 {
		t.Errorf("AAA should be trail (type 6), got %d", winTypeCards)
	}

	posCards := GetWinPosition(cards)
	posCards1 := GetWinPosition(cards1)

	if posCards <= 0 {
		t.Errorf("Position should be positive, got %d", posCards)
	}
	if posCards1 <= 0 {
		t.Errorf("Position should be positive, got %d", posCards1)
	}
	if posCards <= posCards1 {
		t.Errorf("AAA should rank higher than AK3: %d vs %d", posCards, posCards1)
	}

	compareResult := Compare(cards, cards1)
	if compareResult <= 0 {
		t.Errorf("cards should win against cards1, got %d", compareResult)
	}
}

// TestCompareEqual mirrors Java TeenPattiAlgorithmUtilTest.testCompareEqual.
func TestCompareEqual(t *testing.T) {
	cardsA := "黑A,方K,梅Q"
	cardsB := "红A,梅K,方Q"

	result := Compare(cardsA, cardsB)
	if result != 0 {
		t.Errorf("Hands of same rank should tie, got %d", result)
	}
}

// TestGetMaxAndKeyToStr mirrors Java TeenPattiAlgorithmUtilTest.testGetMaxAndKeyToStr.
func TestGetMaxAndKeyToStr(t *testing.T) {
	cards := "黑A,方A,鬼"
	maxKey := GetMax(cards)
	if maxKey <= 0 {
		t.Fatalf("maxKey should be positive, got %d", maxKey)
	}

	maxStr := KeyToStr(maxKey)
	if maxStr == "" {
		t.Fatal("maxStr should not be empty")
	}
	if !strings.Contains(maxStr, "A") {
		t.Errorf("Resolved max card string should contain A, got %q", maxStr)
	}
}

// TestKeyDataGetters mirrors Java TeenPattiAlgorithmUtilTest.testKeyDataGetters.
func TestKeyDataGetters(t *testing.T) {
	pokes := StrToPokes("黑A,方A,鬼")
	kd := GetKeyDataByCards(pokes)

	if kd == nil {
		t.Fatal("KeyData should not be nil")
	}
	if kd.Type != 6 {
		t.Errorf("Type should be 6, got %d", kd.Type)
	}
	if kd.Max <= 0 {
		t.Errorf("Max should be positive, got %d", kd.Max)
	}
	if kd.Position != GetWinPositionByCards(StrToPokes("黑A,方A,鬼")) {
		t.Errorf("Position should match GetWinPositionByCards")
	}
}

// TestInvalidHand mirrors Java TeenPattiAlgorithmUtilTest.testInvalidHand.
func TestInvalidHand(t *testing.T) {
	if got := GetWinPosition("黑A,方A"); got != 0 {
		t.Errorf("GetWinPosition should be 0 for invalid hand, got %d", got)
	}
	if got := GetWinType("黑A,方A"); got != 0 {
		t.Errorf("GetWinType should be 0 for invalid hand, got %d", got)
	}
	if got := GetMax("黑A,方A"); got != 0 {
		t.Errorf("GetMax should be 0 for invalid hand, got %d", got)
	}
}

// TestGetKeyDataRequiresSevenCards documents the Java-compatible behavior of
// the string overload of getKeyData, which only accepts 7-card hands.
func TestGetKeyDataRequiresSevenCards(t *testing.T) {
	if kd := GetKeyData("黑A,方A,鬼"); kd != nil {
		t.Errorf("GetKeyData with 3 cards should return nil (Java parity), got %v", kd)
	}
}

// TestGenKeyEnumeratesAllHands checks that the generation pipeline enumerates
// every distinct 3-card hand of the 55-card deck.
func TestGenKeyEnumeratesAllHands(t *testing.T) {
	GenKey()
	if len(genKeys) != 23479 {
		t.Errorf("expected 23479 distinct keys, got %d", len(genKeys))
	}
}

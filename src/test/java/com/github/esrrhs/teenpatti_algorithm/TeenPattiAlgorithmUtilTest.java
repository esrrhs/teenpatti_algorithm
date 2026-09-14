package com.github.esrrhs.teenpatti_algorithm;

import org.junit.jupiter.api.BeforeAll;
import org.junit.jupiter.api.DisplayName;
import org.junit.jupiter.api.Test;

import java.util.List;

import static org.junit.jupiter.api.Assertions.*;

public class TeenPattiAlgorithmUtilTest
{
	@BeforeAll
	static void setUp()
	{
		TeenPattiAlgorithmUtil.load();
		assertFalse(TeenPattiAlgorithmUtil.normalMap.isEmpty(), "normalMap should not be empty after load");
	}

	@Test
	@DisplayName("Test basic hand ranking and type evaluation")
	void testHandEvaluation()
	{
		String cards = "黑A,方A,鬼";
		String cards1 = "黑A,鬼,方3";

		int winTypeCards = TeenPattiAlgorithmUtil.getWinType(cards);
		int winTypeCards1 = TeenPattiAlgorithmUtil.getWinType(cards1);

		// "黑A,方A,鬼" can form AAA (Trail / Three of a kind, type 6)
		assertEquals(6, winTypeCards, "AAA should be trail (type 6)");

		int posCards = TeenPattiAlgorithmUtil.getWinPosition(cards);
		int posCards1 = TeenPattiAlgorithmUtil.getWinPosition(cards1);

		assertTrue(posCards > 0, "Position should be positive");
		assertTrue(posCards1 > 0, "Position should be positive");
		assertTrue(posCards > posCards1, "AAA should rank higher than AK3");

		int compareResult = TeenPattiAlgorithmUtil.compare(cards, cards1);
		assertTrue(compareResult > 0, "cards should win against cards1");
	}

	@Test
	@DisplayName("Test compare equality")
	void testCompareEqual()
	{
		String cardsA = "黑A,方K,梅Q";
		String cardsB = "红A,梅K,方Q";

		int result = TeenPattiAlgorithmUtil.compare(cardsA, cardsB);
		assertEquals(0, result, "Hands of same rank should tie");
	}

	@Test
	@DisplayName("Test getMax and keyToStr conversion")
	void testGetMaxAndKeyToStr()
	{
		String cards = "黑A,方A,鬼";
		int maxKey = TeenPattiAlgorithmUtil.getMax(cards);
		assertTrue(maxKey > 0);

		String maxStr = TeenPattiAlgorithmUtil.keyToStr(maxKey);
		assertNotNull(maxStr);
		assertTrue(maxStr.contains("A"), "Resolved max card string should contain A");
	}

	@Test
	@DisplayName("Test KeyData getters including getPosition and backward-compatible getPostion")
	void testKeyDataGetters()
	{
		List<Byte> pokes = TeenPattiAlgorithmUtil.strToPokes("黑A,方A,鬼");
		TeenPattiAlgorithmUtil.KeyData keyData = TeenPattiAlgorithmUtil.getKeyData(pokes);

		assertNotNull(keyData, "KeyData should not be null");
		assertEquals(keyData.getPosition(), keyData.getPostion(), "getPosition() and getPostion() should match");
		assertEquals(6, keyData.getType());
		assertTrue(keyData.getMax() > 0);
	}

	@Test
	@DisplayName("Test invalid or empty hand handles gracefully")
	void testInvalidHand()
	{
		assertEquals(0, TeenPattiAlgorithmUtil.getWinPosition("黑A,方A"));
		assertEquals(0, TeenPattiAlgorithmUtil.getWinType("黑A,方A"));
		assertEquals(0, TeenPattiAlgorithmUtil.getMax("黑A,方A"));
	}
}

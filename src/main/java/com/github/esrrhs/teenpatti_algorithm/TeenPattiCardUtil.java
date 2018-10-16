package com.github.esrrhs.teenpatti_algorithm;

import java.util.ArrayList;
import java.util.Collections;
import java.util.Comparator;
import java.util.List;

public class TeenPattiCardUtil
{
	public static final int TEENPATTI_CARD_TYPE_UNKNOWN = 0; //未知
	public static final int TEENPATTI_CARD_TYPE_GAOPAI = 1; //高牌
	public static final int TEENPATTI_CARD_TYPE_DUIZI = 2; //对子
	public static final int TEENPATTI_CARD_TYPE_TONGHUA = 3; //同花
	public static final int TEENPATTI_CARD_TYPE_SHUNZI = 4; //顺子
	public static final int TEENPATTI_CARD_TYPE_TONGHUASHUN = 5; //同花顺
	public static final int TEENPATTI_CARD_TYPE_SANTIAO = 6; //三条

	public static int getCardTypeUnordered(ArrayList<Poke> privateCards)
	{
		if (privateCards == null || privateCards.size() != 3)
		{
			return TEENPATTI_CARD_TYPE_UNKNOWN;
		}
		ArrayList<Poke> tmp = new ArrayList<>(privateCards);
		Collections.sort(tmp, (Poke o1, Poke o2) -> {
			return o1.value - o2.value;
		});
		int value_1 = tmp.get(0).value;
		int value_2 = tmp.get(1).value;
		int value_3 = tmp.get(2).value;

		int color_1 = tmp.get(0).color;
		int color_2 = tmp.get(1).color;
		int color_3 = tmp.get(2).color;
		if (value_1 == value_2 && value_2 == value_3)
		{
			return TEENPATTI_CARD_TYPE_SANTIAO;
		}

		if (value_1 == value_2 || value_2 == value_3)
		{
			return TEENPATTI_CARD_TYPE_DUIZI;
		}

		boolean sameColor = false;
		boolean shunZi = false;
		if (color_1 == color_2 && color_1 == color_3)
		{
			sameColor = true;
		}

		if (value_2 == value_1 + 1 && value_3 == value_2 + 1)
		{
			shunZi = true;
		}
		if (value_1 == Poke.PokeValue_2 && value_2 == Poke.PokeValue_3 && value_3 == Poke.PokeValue_A)
		{
			shunZi = true;
		}

		if (sameColor && shunZi)
		{
			return TEENPATTI_CARD_TYPE_TONGHUASHUN;
		}
		if (sameColor)
		{
			return TEENPATTI_CARD_TYPE_TONGHUA;
		}
		if (shunZi)
		{
			return TEENPATTI_CARD_TYPE_SHUNZI;
		}

		return TEENPATTI_CARD_TYPE_GAOPAI;
	}

	public static List<Byte> pokeToByte(List<Poke> cards)
	{
		List<Byte> ret = new ArrayList<>();
		for (Poke p : cards)
		{
			ret.add(p.toByte());
		}
		return ret;
	}

	public static List<Poke> byteToPoke(List<Byte> cards)
	{
		List<Poke> ret = new ArrayList<>();
		for (Byte p : cards)
		{
			ret.add(new Poke(p));
		}
		return ret;
	}

	/**
	 * 比较两副牌大小
	 * @param firstCards
	 * @param secondCards
	 * @return 1:赢 0:平 -1:输
	 */
	public static int compareCards(ArrayList<Poke> firstCards, ArrayList<Poke> secondCards)
	{
		int firstCardType = getCardTypeUnordered(firstCards);
		int secondCardType = getCardTypeUnordered(secondCards);
		if (firstCardType != secondCardType)
		{
			return firstCardType > secondCardType ? 1 : -1;
		}

		return compareByValue(firstCards, secondCards);
	}

	private static int compareByValue(ArrayList<Poke> firstCards, ArrayList<Poke> secondCards)
	{
		int cardType = getCardTypeUnordered(firstCards);
		if (cardType == TEENPATTI_CARD_TYPE_UNKNOWN)
		{
			return 0;
		}
		ArrayList<Poke> sortCards1 = new ArrayList<>(firstCards);
		ArrayList<Poke> sortCards2 = new ArrayList<>(secondCards);
		Comparator<Poke> comparator = (Poke o1, Poke o2) -> {
			return o1.value - o2.value;
		};
		Collections.sort(sortCards1, comparator);
		Collections.sort(sortCards2, comparator);
		switch (cardType)
		{
			case TEENPATTI_CARD_TYPE_DUIZI:
				return compareDuiZi(sortCards1, sortCards2);
			default:
				return compareNotDuiZi(sortCards1, sortCards2);
		}

	}

	/**
	 * 比较对子牌大小
	 * @param sortCards1 有序
	 * @param sortCards2 有序
	 * @return
	 */
	private static int compareDuiZi(ArrayList<Poke> sortCards1, ArrayList<Poke> sortCards2)
	{
		if (sortCards1.get(0).value != sortCards1.get(1).value)
		{
			Collections.swap(sortCards1, 0, 2);
		}
		if (sortCards2.get(0).value != sortCards2.get(1).value)
		{
			Collections.swap(sortCards2, 0, 2);
		}
		int value_1 = sortCards1.get(0).value;
		int value_3 = sortCards1.get(2).value;

		int value_1_1 = sortCards2.get(0).value;
		int value_3_1 = sortCards2.get(2).value;
		if (value_1 != value_1_1)
		{
			return value_1 > value_1_1 ? 1 : -1;
		}
		if (value_3 != value_3_1)
		{
			return value_3 > value_3_1 ? 1 : -1;
		}

		return 0;
	}

	/**
	 * 比较非对子牌大小
	 * @param sortCards1 有序
	 * @param sortCards2 有序
	 * @return
	 */
	private static int compareNotDuiZi(ArrayList<Poke> sortCards1, ArrayList<Poke> sortCards2)
	{
		int value_1 = sortCards1.get(0).value;
		int value_2 = sortCards1.get(1).value;
		int value_3 = sortCards1.get(2).value;

		int value_1_1 = sortCards2.get(0).value;
		int value_2_1 = sortCards2.get(1).value;
		int value_3_1 = sortCards2.get(2).value;

		if (value_3 != value_3_1)
		{
			return value_3 > value_3_1 ? 1 : -1;
		}
		if (value_2 != value_2_1)
		{
			return value_2 > value_2_1 ? 1 : -1;
		}
		if (value_1 != value_1_1)
		{
			return value_1 > value_1_1 ? 1 : -1;
		}

		return 0;
	}

	public static void main(String[] args)
	{
		Poke poke1 = new Poke((byte) 2, (byte) 2);
		Poke poke2 = new Poke((byte) 3, (byte) 2);
		Poke poke3 = new Poke((byte) 2, (byte) 14);
		ArrayList<Poke> cards1 = new ArrayList<>();
		cards1.add(poke1);
		cards1.add(poke2);
		cards1.add(poke3);
		ArrayList<Poke> cards2 = new ArrayList<>();
		poke1 = new Poke((byte) 2, (byte) 13);
		poke2 = new Poke((byte) 3, (byte) 13);
		poke3 = new Poke((byte) 2, (byte) 2);
		cards2.add(poke1);
		cards2.add(poke2);
		cards2.add(poke3);
		System.out.println(getCardTypeUnordered(cards1));
		System.out.println(getCardTypeUnordered(cards2));
		System.out.println(compareCards(cards1, cards2));
	}
}

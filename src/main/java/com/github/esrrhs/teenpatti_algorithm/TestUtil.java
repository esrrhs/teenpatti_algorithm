package com.github.esrrhs.teenpatti_algorithm;

public class TestUtil
{
	public static void main(String[] args)
	{
		TeenPattiAlgorithmUtil.load();
		String cards = "方4,方A,黑2";
		String cards1 = "红8,方A,方2";
		System.out.println(TeenPattiAlgorithmUtil.getWinPosition(cards));
		System.out.println(TeenPattiAlgorithmUtil.getWinType(cards));

		System.out.println(TeenPattiAlgorithmUtil.getWinPosition(cards1));
		System.out.println(TeenPattiAlgorithmUtil.getWinType(cards1));

		System.out.println(TeenPattiAlgorithmUtil.compare(cards, cards1));
	}

}

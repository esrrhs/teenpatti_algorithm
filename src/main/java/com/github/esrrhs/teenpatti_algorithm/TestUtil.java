package com.github.esrrhs.teenpatti_algorithm;

public class TestUtil
{
	public static void main(String[] args)
	{
		TeenPattiAlgorithmUtil.load();
		String cards = "黑A,方A,黑4";
		String cards1 = "黑A,鬼,方3";
		System.out.println(TeenPattiAlgorithmUtil.getWinPosition(cards));
		System.out.println(TeenPattiAlgorithmUtil.getWinType(cards));

		System.out.println(TeenPattiAlgorithmUtil.getWinPosition(cards1));
		System.out.println(TeenPattiAlgorithmUtil.getWinType(cards1));

		System.out.println(TeenPattiAlgorithmUtil.compare(cards, cards1));
	}

}

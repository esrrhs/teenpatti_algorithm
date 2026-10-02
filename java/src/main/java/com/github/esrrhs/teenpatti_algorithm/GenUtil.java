package com.github.esrrhs.teenpatti_algorithm;

import java.io.File;
import java.io.FileOutputStream;
import java.util.ArrayList;
import java.util.Collections;
import java.util.List;
import java.util.concurrent.atomic.AtomicInteger;

public class GenUtil
{
	public static long totalKey = 0;
	public static FileOutputStream out;
	public static int lastPrint = 0;
	public static long beginPrint;
	public static final long guiNum = 3;
	public static final long genNum = 52 + guiNum;
	public static final long total = (genNum * (genNum - 1) * (genNum - 2)) / (3 * 2);
	public static ArrayList<Integer> keys = new ArrayList<>();
	public static AtomicInteger progress = new AtomicInteger();
	public static final ArrayList<Integer> allCards = genAllCards();

	public static void genKey()
	{
		try
		{
			beginPrint = System.currentTimeMillis();

			genCard();

			System.out.println("genKey finish " + total);
		}
		catch (Exception e)
		{
			e.printStackTrace();
		}
	}

	public static ArrayList<Integer> genAllCards()
	{
		ArrayList<Integer> list = new ArrayList<>();
		for (byte i = 0; i < 4; ++i)
		{
			for (byte j = 0; j < genNum / 4; ++j)
			{
				list.add((Integer) (int) (new Poke(i, (byte) (j + 2))).toByte());
			}
		}
		for (int i = 0; i < guiNum; i++)
		{
			list.add((Integer) (int) Poke.GUI.toByte());
		}
		Collections.sort(list);
		return list;
	}

	private static void genCard() throws Exception
	{
		ArrayList<Integer> list = genAllCards();

		PermutationRun permutationRun = new PermutationRun() {
			@Override
			public void run(int[] tmp, PermutationParam permutationParam) throws Exception
			{
				genCardSave(tmp);
			}
		};
		int[] tmp = new int[3];
		permutation(permutationRun, list, 0, 0, 3, tmp, null);
	}

	public interface PermutationRun
	{
		void run(int[] tmp, PermutationParam permutationParam) throws Exception;
	}

	public static class PermutationParam
	{
		Object o1;
		Object o2;
		Object o3;
		Object o4;
		Object o5;
		Object o6;
	}

	public static void permutation(PermutationRun permutationRun, ArrayList<Integer> a, int count, int count2,
			int except, int[] tmp, PermutationParam permutationParam) throws Exception
	{
		if (count2 == except)
		{
			permutationRun.run(tmp, permutationParam);
		}
		else
		{
			for (int i = count; i < a.size(); i++)
			{
				tmp[count2] = a.get(i);
				permutation(permutationRun, a, i + 1, count2 + 1, except, tmp, permutationParam);
			}
		}
	}

	private static void genCardSave(int[] tmp) throws Exception
	{
		int c = genCardBind(tmp);

		if (!keys.contains(c))
		{
			keys.add(c);
		}
		totalKey++;

		int cur = (int) (totalKey * 100 / total);
		if (cur != lastPrint)
		{
			lastPrint = cur;

			long now = System.currentTimeMillis();
			float per = (float) (now - beginPrint) / totalKey;
			System.out.println(
					cur + "% 需要" + per * (total - totalKey) / 60 / 1000 + "分" + " 用时" + (now - beginPrint) / 60 / 1000
							+ "分" + " 速度" + totalKey / ((float) (now - beginPrint) / 1000) + "条/秒");
		}
	}

	public static int genPokeCardBind(List<Poke> tmp)
	{
		int ret = 0;
		for (Poke i : tmp)
		{
			ret = ret * 100 + i.toByte();
		}
		return ret;
	}

	public static int genCardBind(int[] tmp)
	{
		int ret = 0;
		for (Integer i : tmp)
		{
			ret = ret * 100 + i;
		}
		return ret;
	}

	public static int genCardBind(List<Byte> tmp)
	{
		int ret = 0;
		for (Byte i : tmp)
		{
			ret = ret * 100 + i;
		}
		return ret;
	}

	public static void outputData()
	{
		long begin = System.currentTimeMillis();
		try
		{
			File file = new File("teenpatti_data.txt");
			if (file.exists())
			{
				file.delete();
			}
			file.createNewFile();
			out = new FileOutputStream(file, true);

			beginPrint = System.currentTimeMillis();
			lastPrint = 0;

			Sorter.quicksort(keys);

			totalKey = 0;
			lastPrint = 0;
			beginPrint = System.currentTimeMillis();
			int i = 0;
			int iindex = 0;
			int index = 0;
			int lastMax = 0;
			for (int k : keys)
			{
				int curMax = k;
				String str;
				if (lastMax == 0)
				{
					str = k + " " + i + " " + iindex + " " + keys.size() + " " + toString(keys.get((int) index)) + " "
							+ maxType(keys.get((int) index)) + " " + max(keys.get((int) index)) + " "
							+ toString(max(keys.get((int) index))) + "\n";
					lastMax = curMax;
					iindex = index;
				}
				else
				{
					if (equal(lastMax, curMax))
					{
						str = k + " " + i + " " + iindex + " " + keys.size() + " " + toString(keys.get((int) index))
								+ " " + maxType(keys.get((int) index)) + " " + max(keys.get((int) index)) + " "
								+ toString(max(keys.get((int) index))) + "\n";
						lastMax = curMax;
					}
					else
					{
						i++;
						iindex = index;
						str = k + " " + i + " " + iindex + " " + keys.size() + " " + toString(keys.get((int) index))
								+ " " + maxType(keys.get((int) index)) + " " + max(keys.get((int) index)) + " "
								+ toString(max(keys.get((int) index))) + "\n";
						lastMax = curMax;
					}
				}

				out.write(str.getBytes("utf-8"));
				index++;

				int cur = (int) (index * 100 / keys.size());
				if (cur != lastPrint)
				{
					lastPrint = cur;

					long now = System.currentTimeMillis();
					float per = (float) (now - beginPrint) / index;
					System.out.println(cur + "% 需要" + per * (keys.size() - index) / 60 / 1000 + "分" + " 用时"
							+ (now - beginPrint) / 60 / 1000 + "分" + " 速度" + index / ((float) (now - beginPrint) / 1000)
							+ "条/秒");
				}
			}

			out.close();

			System.out.println("outputData finish " + keys.size() + " time:"
					+ (System.currentTimeMillis() - begin) / 1000 / 60 + "分 " + GenUtil.progress);

			keys.clear();
		}
		catch (Exception e)
		{
			e.printStackTrace();
		}
	}

	public static int max(int k)
	{
		ArrayList<Poke> cs = toArray(k);
		ArrayList<Poke> max = TeenPattiCardUtil.maxCards(cs);
		return genPokeCardBind(max);
	}

	public static int maxType(int k)
	{
		ArrayList<Poke> cs = toArray(k);
		ArrayList<Poke> max = TeenPattiCardUtil.maxCards(cs);
		return TeenPattiCardUtil.getCardTypeUnordered(max);
	}

	public static ArrayList<Poke> toArray(int k)
	{
		ArrayList<Poke> cs = new ArrayList<>();
		if (k > 1000000000000L)
		{
			cs.add(new Poke((byte) (k % 100000000000000L / 1000000000000L)));
		}
		if (k > 10000000000L)
		{
			cs.add(new Poke((byte) (k % 1000000000000L / 10000000000L)));
		}
		if (k > 100000000L)
		{
			cs.add(new Poke((byte) (k % 10000000000L / 100000000L)));
		}
		if (k > 1000000L)
		{
			cs.add(new Poke((byte) (k % 100000000L / 1000000L)));
		}
		if (k > 10000L)
		{
			cs.add(new Poke((byte) (k % 1000000L / 10000L)));
		}
		if (k > 100L)
		{
			cs.add(new Poke((byte) (k % 10000L / 100L)));
		}
		if (k > 1L)
		{
			cs.add(new Poke((byte) (k % 100L / 1L)));
		}
		return cs;
	}

	public static String toString(int k)
	{
		ArrayList<Poke> cs = toArray(k);
		String ret = "";
		for (Poke poke : cs)
		{
			ret += poke;
		}
		return ret;
	}

	public static boolean compare(int k1, int k2)
	{
		ArrayList<Poke> cs1 = toArray(k1);
		ArrayList<Poke> cs2 = toArray(k2);
		return TeenPattiCardUtil.compareCards(cs1, cs2) < 0;
	}

	public static boolean equal(int k1, int k2)
	{
		ArrayList<Poke> cs1 = toArray(k1);
		ArrayList<Poke> cs2 = toArray(k2);
		return TeenPattiCardUtil.compareCards(cs1, cs2) == 0;
	}

	public static void main(String[] args)
	{
		System.out.println(compare(020304, 020305));
	}
}

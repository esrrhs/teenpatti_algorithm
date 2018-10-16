package com.github.esrrhs.teenpatti_algorithm;

import java.io.*;
import java.util.ArrayList;
import java.util.Collections;
import java.util.List;
import java.util.concurrent.ConcurrentHashMap;

public class TeenPattiAlgorithmUtil
{
	public static class KeyData
	{
		public KeyData(int postion, int type)
		{
			this.postion = postion;
			this.type = type;
		}

		private int postion;
		private int type;

		public int getPostion()
		{
			return postion;
		}

		public int getType()
		{
			return type;
		}
	}

	public static ConcurrentHashMap<Integer, KeyData> normalMap = new ConcurrentHashMap<>();

	public static void main(String[] args)
	{
		gen();
	}

	private static void gen()
	{
		File file = new File("texas_data.txt");
		if (!file.exists())
		{
			GenUtil.genKey();
			GenUtil.outputData();
		}
	}

	public static void load()
	{
		try
		{
			FileInputStream inputStream = new FileInputStream("teenpatti_algorithm_data.txt");
			loadNormal(inputStream);
			inputStream.close();
		}
		catch (Exception e)
		{
			e.printStackTrace();
		}
	}

	public static void loadNormal(InputStream inputStream) throws Exception
	{
		BufferedReader bufferedReader = new BufferedReader(new InputStreamReader(inputStream));

		String str = null;
		while ((str = bufferedReader.readLine()) != null)
		{
			String[] params = str.split(" ");
			int key = Integer.parseInt(params[0]);
			int i = Integer.parseInt(params[1]);
			int type = Integer.parseInt(params[2]);

			KeyData keyData = new KeyData(i, type);
			normalMap.put(key, keyData);
		}
		bufferedReader.close();
	}

	public static byte strToPokeValue(String str)
	{
		if (str.equals("A"))
		{
			return Poke.PokeValue_A;
		}
		else if (str.equals("K"))
		{
			return Poke.PokeValue_K;
		}
		else if (str.equals("Q"))
		{
			return Poke.PokeValue_Q;
		}
		else if (str.equals("J"))
		{
			return Poke.PokeValue_J;
		}
		else
		{
			return Byte.parseByte(str);
		}
	}

	public static byte strToPoke(String str)
	{
		if (str.startsWith("方"))
		{
			return (new Poke(Poke.PokeColor_FANG, strToPokeValue(str.substring(1)))).toByte();
		}
		else if (str.startsWith("梅"))
		{
			return (new Poke(Poke.PokeColor_MEI, strToPokeValue(str.substring(1)))).toByte();
		}
		else if (str.startsWith("红"))
		{
			return (new Poke(Poke.PokeColor_HONG, strToPokeValue(str.substring(1)))).toByte();
		}
		else if (str.startsWith("黑"))
		{
			return (new Poke(Poke.PokeColor_HEI, strToPokeValue(str.substring(1)))).toByte();
		}
		else
		{
			return 0;
		}
	}

	public static String keyToStr(int key)
	{
		return GenUtil.toString(key);
	}

	public static List<Byte> keyToByte(int k)
	{
		ArrayList<Byte> cs = new ArrayList<>();
		if (k > 1000000000000L)
		{
			cs.add(((byte) (k % 100000000000000L / 1000000000000L)));
		}
		if (k > 10000000000L)
		{
			cs.add(((byte) (k % 1000000000000L / 10000000000L)));
		}
		if (k > 100000000L)
		{
			cs.add(((byte) (k % 10000000000L / 100000000L)));
		}
		if (k > 1000000L)
		{
			cs.add(((byte) (k % 100000000L / 1000000L)));
		}
		if (k > 10000L)
		{
			cs.add(((byte) (k % 1000000L / 10000L)));
		}
		if (k > 100L)
		{
			cs.add(((byte) (k % 10000L / 100L)));
		}
		if (k > 1L)
		{
			cs.add(((byte) (k % 100L / 1L)));
		}
		return cs;
	}

	public static List<Byte> strToPokes(String str)
	{
		List<Byte> ret = new ArrayList<>();
		if (str.length() == 0)
		{
			return ret;
		}
		String[] strs = str.split(",");
		for (String s : strs)
		{
			ret.add(strToPoke(s));
		}
		return ret;
	}

	public static KeyData getKeyData(String str)
	{
		List<Byte> pokes = strToPokes(str);
		if (pokes.size() != 7)
		{
			return null;
		}

		return getKeyData(pokes);
	}

	public static KeyData getKeyData(int key)
	{
		return normalMap.get(key);
	}

	public static KeyData getKeyData(List<Byte> pokes)
	{
		int key = GenUtil.genCardBind(pokes);

		return getKeyData(key);
	}

	public static int getWinPosition(String str)
	{
		return getWinPosition(strToPokes(str));
	}

	public static int getWinPosition(List<Byte> pokes)
	{
		KeyData keyData = getKeyData(pokes);
		if (keyData == null)
		{
			return 0;
		}
		return keyData.getPostion();
	}

	public static int getWinType(String str)
	{
		return getWinType(strToPokes(str));
	}

	public static int getWinType(List<Byte> pokes)
	{
		KeyData keyData = getKeyData(pokes);
		if (keyData == null)
		{
			return 0;
		}
		return keyData.getType();
	}

	public static int compare(String str1, String str2)
	{
		return compare(strToPokes(str1), strToPokes(str2));
	}

	public static int compare(List<Byte> bytes1, List<Byte> bytes2)
	{
		return compare(GenUtil.genCardBind(bytes1), GenUtil.genCardBind(bytes2));
	}

	public static int compare(int k1, int k2)
	{
		KeyData keyData1 = getKeyData(k1);
		KeyData keyData2 = getKeyData(k2);
		if (keyData1 == null && keyData2 == null)
		{
			return 0;
		}
		if (keyData1 == null)
		{
			return -1;
		}
		if (keyData2 == null)
		{
			return 1;
		}
		return keyData1.getPostion() - keyData2.getPostion();
	}
}

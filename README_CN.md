# 印度炸金花 TeenPatti 算法

基于**查表法**的高性能印度炸金花算法，完整支持**鬼牌（万能牌）**。给定任意3张手牌，库可立即返回牌型、强度排名以及鬼牌展开后的最优组合。

算法由 [texas_algorithm](https://github.com/esrrhs/texas_algorithm) 修改而来。

---

## 目录

- [游戏规则简介](#游戏规则简介)
- [快速开始](#快速开始)
- [API 说明](#api-说明)
- [牌面记法](#牌面记法)
- [牌型大小](#牌型大小)
- [算法原理](#算法原理)
- [生成查表数据](#生成查表数据)
- [运行示例](#运行示例)
- [相关项目](#相关项目)

---

## 游戏规则简介

TeenPatti 使用标准52张牌加上**3张鬼牌**（共55张）。每位玩家发3张牌，目标是凑出最强的3张牌组合。鬼牌为万能牌，可代替任意一张牌以形成最优组合。

---

## 快速开始

### Maven 依赖

```xml
<dependency>
    <groupId>com.github.esrrhs</groupId>
    <artifactId>teenpatti_algorithm</artifactId>
    <version>1.0.2</version>
</dependency>
```

### 基本用法

```java
// 1. 启动时加载一次查表数据
TeenPattiAlgorithmUtil.load();

// 2. 获取牌型（返回整数常量，见"牌型大小"章节）
int type = TeenPattiAlgorithmUtil.getWinType("黑A,方A,鬼");
// type == 6  → 三条（鬼牌充当第三张A）

// 3. 获取排名（数值越大越强）
int position = TeenPattiAlgorithmUtil.getWinPosition("黑2,黑3,黑4");

// 4. 比较两副牌（正数=第一副赢，负数=第二副赢，0=平局）
int result = TeenPattiAlgorithmUtil.compare("黑A,方A,鬼", "黑A,鬼,方3");

// 5. 获取鬼牌展开后的最优组合
int maxKey = TeenPattiAlgorithmUtil.getMax("黑A,方A,鬼");
String maxStr = TeenPattiAlgorithmUtil.keyToStr(maxKey);  // 例如 "黑A方A红A"
```

---

## API 说明

所有公开方法均在 `TeenPattiAlgorithmUtil` 类中。

| 方法 | 参数 | 返回值 | 说明 |
|------|------|--------|------|
| `load()` | 无 | `void` | 从 `teenpatti_data.txt` 加载查表数据（启动时调用一次） |
| `loadNormal(InputStream)` | 输入流 | `void` | 从自定义流加载查表数据 |
| `getWinType(String)` | 逗号分隔的牌面字符串 | `int` | 牌型常量（见牌型大小） |
| `getWinPosition(String)` | 逗号分隔的牌面字符串 | `int` | 全局排名（数值越大越强） |
| `getMax(String)` | 逗号分隔的牌面字符串 | `int` | 最优组合的编码 key |
| `compare(String, String)` | 两副牌 | `int` | 正/零/负比较结果 |
| `keyToStr(int)` | 编码 key | `String` | 可读牌面字符串 |

### KeyData 字段

`getKeyData()` 返回包含三个字段的 `KeyData` 对象：

| 字段 | Getter | 说明 |
|------|--------|------|
| `position` | `getPosition()` / `getPostion()` | 在所有可能手牌中的全局排名 |
| `type` | `getType()` | 牌型（1–6，见牌型大小） |
| `max` | `getMax()` | 鬼牌展开后最优组合的编码 key |

---

## 牌面记法

牌面写法为 `<花色><点数>`，多张牌用逗号分隔。

### 花色

| 记法 | 花色 | 符号 |
|------|------|------|
| `方` | 方块 | ♦ |
| `梅` | 梅花 | ♣ |
| `红` | 红心 | ♥ |
| `黑` | 黑桃 | ♠ |

### 点数

`2` `3` `4` `5` `6` `7` `8` `9` `10` `J` `Q` `K` `A`

### 鬼牌

用 `鬼` 表示万能牌，牌组中共有3张鬼牌。

### 示例

| 输入字符串 | 含义 |
|------------|------|
| `"黑A,方A,鬼"` | 黑桃A、方块A、鬼牌 |
| `"黑2,黑3,黑4"` | 2♠ 3♠ 4♠ |
| `"红K,梅Q,方J"` | K♥ Q♣ J♦ |

---

## 牌型大小

从小到大排列：

| 排名 | 类型常量 | 牌型名称 | 说明 |
|------|---------|---------|------|
| 1 | `TEENPATTI_CARD_TYPE_GAOPAI = 1` | 高牌 | 无任何组合，按最大牌比较 |
| 2 | `TEENPATTI_CARD_TYPE_DUIZI = 2` | 对子 | 两张相同点数 |
| 3 | `TEENPATTI_CARD_TYPE_TONGHUA = 3` | 同花 | 三张相同花色 |
| 4 | `TEENPATTI_CARD_TYPE_SHUNZI = 4` | 顺子 | 三张连续点数（A-2-3 也算） |
| 5 | `TEENPATTI_CARD_TYPE_TONGHUASHUN = 5` | 同花顺 | 连续点数且相同花色 |
| 6 | `TEENPATTI_CARD_TYPE_SANTIAO = 6` | 三条 | 三张相同点数 |

> **注意：** TeenPatti 中三条比同花顺大，这与德州扑克不同。

**同型比大小规则：**
- **三条 / 对子：** 先比对子点数，再比踢脚牌。
- **其他牌型：** 依次比较最大、次大、最小牌的点数。

---

## 算法原理

### 整体思路

库使用**预计算查表**方式。查询时，将3张牌编码为一个整数 key，然后在 `ConcurrentHashMap` 中 O(1) 查找对应的排名、牌型和最优展开结果。

### 第一步 — 牌面编码

每张牌用1字节表示：高4位存花色（0–3），低4位存点数（2–14）。鬼牌使用保留值 `(color=5, value=8)`。

```
byte = (花色 << 4) | 点数
```

3张牌的组合编码为一个 `int`，将三个字节值按十进制拼接：

```
key = 牌1字节 * 10000 + 牌2字节 * 100 + 牌3字节
```

编码前先对牌排序，保证同一组合无论顺序如何都产生相同的 key。

### 第二步 — 穷举所有组合

牌组共 **55 张**（52 张普通牌 + 3 张鬼牌），穷举 C(55,3) = **26,235 种唯一组合**，通过递归组合生成器实现，并去除编码重复的情况。

### 第三步 — 多线程快速排序

所有组合 key 按牌力大小进行**并行快速排序**（`Sorter.java`），线程池大小等于 CPU 核心数。当活跃线程数超过 `2 × CPU核心数` 时，子分区退回单线程递归，防止线程爆炸。

比较函数（`GenUtil.compare`）在比较前先将鬼牌展开为最优替代牌，确保排序结果反映真实游戏结果。

### 第四步 — 输出查表文件

排序完成后，每条记录按以下格式写入 `teenpatti_data.txt`：

```
<key> <排名> <排名索引> <总条数> <最优牌面字符串> <牌型> <最优牌面key> <最优牌面可读字符串>
```

排名索引仅在相邻两手牌不等强时递增，形成连续的密集排名。

### 查询流程（运行时）

```
输入字符串 → 解析牌面 → 字节排序 → 编码 key → HashMap.get(key) → KeyData{position, type, max}
```

---

## 生成查表数据

运行 `TeenPattiAlgorithmUtil.main()`（或依次调用 `GenUtil.genKey()` 和 `GenUtil.outputData()`）可重新生成 `teenpatti_data.txt`。仅在修改牌组或排名规则时需要执行。

```bash
mvn exec:java -Dexec.mainClass="com.github.esrrhs.teenpatti_algorithm.TeenPattiAlgorithmUtil"
```

生成过程会输出进度、预计剩余时间和处理速度（条/秒）。

---

## 运行示例

`TestUtil.main()` 加载查表数据后，对两副示例手牌输出结果：

```java
TeenPattiAlgorithmUtil.load();

String cards  = "黑A,方A,鬼";   // A♠ A♦ 鬼  → 三条（三张A）
String cards1 = "黑A,鬼,方3";   // A♠ 鬼 3♦  → 对子（一对A）

System.out.println(TeenPattiAlgorithmUtil.getWinPosition(cards));   // 排名
System.out.println(TeenPattiAlgorithmUtil.getWinType(cards));       // 6 = 三条
System.out.println(TeenPattiAlgorithmUtil.keyToStr(
        TeenPattiAlgorithmUtil.getMax(cards)));                      // 最优展开结果

System.out.println(TeenPattiAlgorithmUtil.compare(cards, cards1));  // > 0: cards 赢
```

---

## 相关项目

- [majiang_algorithm](https://github.com/esrrhs/majiang_algorithm) — 麻将算法
- [texas_algorithm](https://github.com/esrrhs/texas_algorithm) — 德州扑克算法

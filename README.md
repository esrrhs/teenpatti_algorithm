# 印度炸金花TeenPatti算法
用于印度炸金花TeenPatti的带鬼牌的算法，通过牌查表获得牌型和大小。算法由texas_algorithm修改得来。

## 使用
``` xml
<dependency>
    <groupId>com.github.esrrhs</groupId>
    <artifactId>teenpatti_algorithm</artifactId>
    <version>1.0.0</version>
</dependency>
```
``` java
// 获取3张手牌的牌型
TeenPattiAlgorithmUtil.getWinType("黑2,黑3,鬼");
// 获取3张牌的大小，用于比牌
int win = TeenPattiAlgorithmUtil.getWinPosition("黑2,黑3,黑4");
```

## 测试玩玩
* 运行TestUtil.Main

## 生成表玩玩
* 运行 TeenPattiAlgorithmUtil.Main

# 查表算法
查表算法，给定任意3张牌，查表给出3张最大牌的大小、类型。查表方法很简单，下面讲一下生成表的算法。

### 算法实现

#### 穷举C(52, 3)的组合
52张牌里面选3张，对3张牌进行编码变成int类型，得到一个数组。

#### 多线程快速排序
对这数组进行从小到大排序，排序依据就是牌的大小。使用多线程快速排序。

#### 结果输出
数组已经排好序，现在按照顺序输出到一个文件，内容有key、大小顺序、可阅读的牌面信息。

## 其他
<a href="https://github.com/esrrhs/majiang_algorithm">麻将算法</a>
<a href="https://github.com/esrrhs/texas_algorithm">德州算法</a>

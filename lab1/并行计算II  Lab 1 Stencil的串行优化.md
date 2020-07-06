## 并行计算II  Lab 1 Stencil的串行优化

#### 林挺 1700010644 

1. **循环变量的交换**： 将`i`,`j`,`k` 的顺序交换，以提升访存的效率，降低cache miss的概率。【有效】

2. **活用input和output**: 原代码中每次都要先算到`output`中然后在写会`input`，这样无疑造成巨大的浪费，因此我们并不写回，而是直接交替更新，在偶数步的时候用`output`的值去更新`input` . 最后判断从`input`输出还是`output`输出即可。【有效】

   P.S. 试图用直接换指针的方式，大失败。

3. **数组对齐带来的cachemiss**: 将第一维加3 第二维加6. 【一点点效果】

4. **减少计算的重复**： 注意到每一层的计算中有一些重复的可以省去，利用一个`tmp`数组来储存`tmp[j][k] = input[j+1][k] + input[j][k+1] ` 再计算`output[i][j]`的时候有一定的方便之处。

5. **加了一些循环展开**`#pragma gcc unroll(8)`

6. `icc -O3 -unroll -ip  -pc32 stencil.c -ax=CORE-AVX2 stencil` 尽可能利用icc完成向量化

------------------------

一些失败的尝试

1. 把边界的处理单独拿出来，结果没什么卵用。 （好像是有一丢丢作用的 ）
2. 把`index`改成利用位运算， 结果大幅度变慢了。



-----------

实验结果

![image-20200405113449514](C:\Users\linti\AppData\Roaming\Typora\typora-user-images\image-20200405113449514.png)

![image-20200405113517197](C:\Users\linti\AppData\Roaming\Typora\typora-user-images\image-20200405113517197.png)

![image-20200405113531611](C:\Users\linti\AppData\Roaming\Typora\typora-user-images\image-20200405113531611.png)

![image-20200405113544184](C:\Users\linti\AppData\Roaming\Typora\typora-user-images\image-20200405113544184.png)




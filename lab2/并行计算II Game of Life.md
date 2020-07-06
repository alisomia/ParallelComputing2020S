### Game of Life 的MPI加速

---

在本次lab中，我们利用MPI在8个核上实现了对game of life的加速，具体的代码见`gol.c`

#### 编译上的优化

gcc 采用版本 10.1 在服务器上使用

```shell
module add gcc
```

MPI 采用 openmi  在服务器上使用

```
module add openmpi
```

编译指令如下

```makefile
MPICC = mpicc
FLAGS = -std=c99 -Wall -Ofast -march=native
gol:
	$(MPICC) $(FLAGS) gol.c -lm -o gol
```

#### MPI的框架

1. **笛卡尔拓扑 **(Cartisitan Topology)

   为了传递参数的方便，我们采用了MPI自带的笛卡尔拓扑，利用

   ```c
   MPI_Comm old_comm = MPI_COMM_WORLD, new_comm;
   int ndims = 2, dim_size[2], periods[2], reorder = 1;//2维
   dim_size[0] = 4, dim_size[1] = 2; // 4行2列
   periods[0] = periods[1] = 1; // 开启周期边界
   MPI_Cart_create(old_comm, ndims, dim_size, periods, reorder, &new_comm);
   ```

   

   生成笛卡尔拓扑，这样获得相邻的网格的`rank`会比较简单：

    ```c
    MPI_Cart_coords(new_comm, rank, ndims, coords);
    int up;
    int up_coords[2];
    up_coords[0] = coords[0]-1, up_coords[1] = coords[1];
    MPI_Cart_rank(new_comm, up_coords, &up);
    ```

2. **机制**
   假设`xn`和`yn`是本地要存下的矩阵大小（在代码中是500和1000），我们考虑在每个节点上建立大小为`xn+2`和`yn+2`的数组`world[xn+2][yn+2]`，其中`world[1:xn][1:yn]`用来存本地的世界状态，而其余的部分通过与周围8个邻居交换数据获得。
   
3. **利用`MPI_Sendrecv`  进行收发，防止死锁**

   （本来是想分奇偶讨论然后决定收发的，后来写着写着就死锁了，然后换用sendrecv了）

   - 行的交换是简单的，我们让每个节点向`up`发送，并接受`down`的数据。然后再向`down`发送数据并接受`up`的数据。这样相当于4个一组交换了他们的数据

   ```c
   MPI_Sendrecv(&world[1][1], yn, MPI_CHAR, up, 0, &world[xn+1][1], yn, MPI_CHAR, down, 0, new_comm, MPI_STATUS_IGNORE);
   MPI_Sendrecv(&world[xn][1], yn, MPI_CHAR, down, 0, &world[0][1], yn, MPI_CHAR, up, 0, new_comm, MPI_STATUS_IGNORE);
   ```

   - 列的交换本质上是一样的，只不过这时候我们会遇到想发送/接收的数据位置不是连续的，需要自定义`MPI_Datatype`


   ```c
   MPI_Datatype vertical_type;
    MPI_Type_vector(xn, 1, yn+2, MPI_CHAR, &vertical_type); // 创建一个总长度是xn,每个block size是1, stride是yn+2的列向量
    MPI_Type_commit(&vertical_type);
   ```

    ```c
   MPI_Sendrecv(&world[1][1], 1, vertical_type, left, 0, &world[1][yn+1], 1, vertical_type, right, 0, new_comm, MPI_STATUS_IGNORE); // send to left and recv from right
   MPI_Sendrecv(&world[1][yn], 1, vertical_type, right, 0, &world[1][0], 1, vertical_type, left, 0, new_comm, MPI_STATUS_IGNORE); //  send to right and recv from left
    ```
   - 四个角的交换

   ```c
   MPI_Sendrecv(&world[1][1], 1, MPI_CHAR, up_left, 0, &world[xn+1][yn+1], 1, MPI_CHAR, down_right, 0, new_comm, MPI_STATUS_IGNORE); // send to up_left and recv from down_right
   MPI_Sendrecv(&world[1][yn], 1, MPI_CHAR, up_right, 0, &world[xn+1][0], 1, MPI_CHAR, down_left, 0, new_comm, MPI_STATUS_IGNORE); // send to up_right and recv from down_left
   MPI_Sendrecv(&world[xn][1], 1, MPI_CHAR, down_left, 0, &world[0][yn+1], 1, MPI_CHAR, up_right, 0, new_comm, MPI_STATUS_IGNORE); // send to down_left and recv from down_right
   MPI_Sendrecv(&world[xn][yn], 1, MPI_CHAR, down_right, 0, &world[0][0], 1, MPI_CHAR, up_left, 0, new_comm, MPI_STATUS_IGNORE); // send to down_right and recv from up_left
   ```

   #### 其他的一些优化

   直接利用上面的MPI框架测得的速度大概在0.03s左右，接下来我们继续对算法层面进行一些优化。

   1. 利用两个数组`world`和`result`直接来回计算，减少了将`result`的结果写回`world`造成的不必要的开销。

   2. 展开了计算邻居的公式，并且对死活的判定做了一些优化

      ```c
      int nn = result[x-1][y-1] + result[x-1][y] + result[x-1][y+1] + result[x][y-1] + result[x][y+1] + result[x+1][y-1] + result[x+1][y] + result[x+1][y+1];
      
      world[x][y] = !(((nn & 7) | result[x][y]) ^ 3);
      ```

      这里的判断应该还是可以继续优化的，因为位运算似乎没有想象中的那么高效，但是我尝试了一下好像还没有写出比上面这个更快的算法orz.
      
   3. 改变数组大小防止cache miss. 我们把第二维开成`yn+4`. 注意到这里对应的列交换时候的`MPI_Datatype`也要做出相应的修改

   4. 把`world`和`result`的数据用`bool`来存

#### 结果

​      

![image-20200520175720545](C:\Users\linti\AppData\Roaming\Typora\typora-user-images\image-20200520175720545.png)

![image-20200520175756474](C:\Users\linti\AppData\Roaming\Typora\typora-user-images\image-20200520175756474.png)
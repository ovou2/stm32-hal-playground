# 参考 · printf 重定向到串口

> Keil MDK + HAL 库。核心就一个函数：让 `printf` 把字符交给串口，而不是默认的"半主机"调试通道。

## 代码

**1. 加头文件** —— `main.c` 的 `USER CODE BEGIN Includes`

```c
#include <stdio.h>
```

**2. 重定向** —— `main.c` 的 `USER CODE BEGIN 0`

```c
/* 把 huart2 换成你自己的串口句柄（huart1 / huart3 ...） */
int fputc(int ch, FILE *f)
{
  HAL_UART_Transmit(&huart2, (uint8_t *)&ch, 1, HAL_MAX_DELAY);
  return ch;
}
```

**3. 正常用** —— 任何地方都能直接调

```c
printf("Hello, %d\r\n", 123);
```

## 必须勾选 Use MicroLIB

Keil 魔术棒（`Alt+F7`）→ `Target` 标签 → 勾上 **`Use MicroLIB`**。

不勾的后果：C 标准库会去找"半主机"（semihosting）通道，程序可能卡在启动阶段，串口一个字符都出不来 —— 不报错，最难查。

## 坑

1. **★ 没勾 `Use MicroLIB`** —— 无输出或程序卡死，且不报错
2. **★ 换行要写 `\r\n`** —— 多数串口助手只认 `\r\n`，只写 `\n` 会一直输出在同一行
3. **`printf` 是阻塞发送** —— 115200 下每字符约 87μs，**别在中断里调用**（会拖慢整个中断响应）
4. **换 GCC 工具链（CubeIDE）时函数名不同** —— 要写 `int __io_putchar(int ch)`；Keil 用 `fputc`

## 顺带：串口最常用的三对函数

| 发送 | 接收 |
|------|------|
| `HAL_UART_Transmit(&huart2, buf, len, timeout)` | `HAL_UART_Receive(&huart2, buf, len, timeout)` |
| `HAL_UART_Transmit_IT(&huart2, buf, len)` | `HAL_UART_Receive_IT(&huart2, buf, len)` |
| `HAL_UART_Transmit_DMA(&huart2, buf, len)` | `HAL_UART_Receive_DMA(&huart2, buf, len)` |

- 带 `_IT` 的要在中断回调里收尾：`HAL_UART_TxCpltCallback` / `HAL_UART_RxCpltCallback`
- `buf` 参数是 **`uint8_t *`（指针）**，别写成 `(uint8_t)RxBuff`
- 句柄是 `huart2`，不是 `USART2`（`USART2` 是外设寄存器基址，传给 HAL 函数会编译报错）

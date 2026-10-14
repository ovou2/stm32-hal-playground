# 05-UART · 笔记

## 原理

- 串口是**异步**通信：没有时钟线，靠双方约定的**波特率**对齐（这里 115200）
- 一帧格式 **8N1** = 1 起始位 + 8 数据位 + 无校验 + 1 停止位
- 接线必须**交叉**：板子 TX 接模块 RX、板子 RX 接模块 TX
- 寄存器只需认三个：`DR`（收发数据）、`SR`（状态，含 RXNE/TXE）、`BRR`（波特率）
- `printf` 默认走"半主机"调试通道，要重定向到串口才能用 → 见 `参考-printf重定向.md`

## 接线

```
PA2 (USART2_TX) ──→ USB-TTL 模块 RX
PA3 (USART2_RX) ←── USB-TTL 模块 TX
GND ───────────── GND
```

> 本工程用的是 **USART2**，句柄是 `huart2`（不是 USART1 / huart1）。

## 定长接收的调用链

```
PC 发 5 字节
  → USART2_IRQHandler()              stm32f1xx_it.c:204（CubeMX 生成）
    → HAL_UART_IRQHandler(&huart2)   HAL：读 DR 存进 RxBuff、计数 +1
      → HAL_UART_RxCpltCallback()    main.c ← 你写的（收满 5 个才调用）
```

发送链路：

```
printf("...")
  → fputc(ch, f)                     main.c ← 你写的（重定向）
    → HAL_UART_Transmit()            HAL：阻塞逐字节发
```

## HAL 关键函数

| 函数 | 作用 |
|------|------|
| `HAL_UART_Receive_IT(&huart2, buf, len)` | 挂上接收，收满 len 个触发回调 |
| `HAL_UART_Transmit_IT(&huart2, buf, len)` | 异步发送，发完触发 `TxCpltCallback` |
| `HAL_UART_Transmit(&huart2, buf, len, timeout)` | 阻塞发送，发完才返回 |
| `HAL_UART_RxCpltCallback(huart)` | **用户回调**，收满一帧时被调用 |

**第一个参数永远是 `&huartN` 句柄，缓冲区永远是 `uint8_t *` 指针。**

## 踩过的坑

1. **★ 回调里忘了重新挂接收** —— 接收中断是"一次性"的，HAL 收满一帧就自动关掉。不在回调里再调 `HAL_UART_Receive_IT`，就永远只能收到第一帧
2. **★ `fputc` 写在了 `USER CODE BEGIN 4` 内部，还自己加了一对 `USER CODE BEGIN 0`** —— 同名标记出现两次，CubeMX 重新生成时位置会错乱。`fputc` 应放在函数外那对 `USER CODE BEGIN 0` 里
3. **★ 没勾 Keil 的 `Use MicroLIB`** —— `printf` 会去找"半主机"通道，串口无输出且不报错
4. **RX 和 TX 共用同一个缓冲区有风险** —— `Transmit_IT` 是异步的，发完之前若收到新数据，`RxBuff` 会被覆盖，发出去的就是花屏。手动测试间隔大时看不出来；要严谨就开两个缓冲区
5. **句柄别写成 `USART2`** —— `USART2` 是寄存器基址，HAL 函数要的是 `&huart2`；但 `huart->Instance == USART2` 这种比较反而是对的

## 待办

- [ ] 烧录后用串口助手发 5 个字符，看是否回显
- [ ] 故意只发 3 个字符，验证"收不满就不回调"
- [ ] 把 `length` 改成 10，验证改动一处即可生效

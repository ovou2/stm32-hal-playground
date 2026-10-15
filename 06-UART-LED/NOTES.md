# 06-UART-LED · 笔记

## 原理

- 在 05「回显」的基础上换成「**解析命令**」：PC 发 `ON` 点灯、发 `OFF` 灭灯
- **ON 是 2 字节、OFF 是 3 字节**，长度不固定 → 定长接收没法兼顾，改用**单字节模式**：
  - 挂接收 `HAL_UART_Receive_IT(&huart2, &Rxbuff[count], 1)`，每次只收 1 个
  - 每收 1 个进一次回调，`count++` 自己数「这是第几个字节」
- **count 是跨回调的记忆** —— 回调每次进来都是同一段代码，局部变量每次清零，只有全局变量能把「收到第几个」从上一次带到下一次

## 接线

```
PA2 (USART2_TX) ──→ USB-TTL 模块 RX
PA3 (USART2_RX) ←── USB-TTL 模块 TX
GND ───────────── GND
PB3 ──[220Ω]── LED ── GND
```

## 命令解析的三道关

```c
count++;                                                   /* ① 数到第几个了 */
if (count == 2 && strncmp((char *)Rxbuff, "ON", 2) == 0)   /* ② 长度 + 内容双条件 */
{ 点灯; count = 0; memset(Rxbuff, 0, sizeof(Rxbuff)); }     /* ③ 用完必须复位 */
```

| 关 | 作用 |
|----|------|
| 长度判断 | 只比内容不比长度，收到 "ONx" 也会误判 |
| 内容判断 | `strncmp` 只比前 N 个字符 |
| count / memset 复位 | 不复位的话，下一条命令会接着旧字符拼，永远对不上 |

## HAL 关键函数

| 函数 | 作用 |
|------|------|
| `HAL_UART_Receive_IT(&huart2, &Rxbuff[count], 1)` | 挂接收，**写入位置跟着 count 走** |
| `HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_SET/RESET)` | 点灯 / 灭灯 |
| `strncmp((char *)Rxbuff, "ON", 2)` | 比较缓冲区前 2 个字符是否相等 |
| `memset(Rxbuff, 0, sizeof(Rxbuff))` | 清空缓冲区 |

## 踩过的坑

1. **★ 忘了越界保护** —— `Rxbuff` 只有 3 格。发 "ABC" 这种杂鱼数据时 count 会走到 3，下次往 `&Rxbuff[3]` 写就越界、破坏相邻内存。必须加 `if (count >= 3) { count=0; memset(...); }`
2. **★ 命令处理完忘了复位** —— `count = 0` 和 `memset` 少一个都不行，否则下一条命令接在旧字符后面拼，永远匹配不上
3. **★ 回调里忘了重新挂接收** —— 同 05：接收中断是一次性的，不重挂只收得到第一帧
4. **`strncmp` 前要强转 `(char *)`** —— `Rxbuff` 是 `uint8_t[]`，`strncmp` 要 `char *`，不强转会有警告
5. **回调里的 `printf` 是阻塞发送** —— 发送期间 PC 若连发多个字节，可能 ORE 溢出导致接收停止。手动测试没事，做成产品要换环形缓冲区

## 待办

- [ ] 烧录：串口助手发 `ON` → LED 亮；发 `OFF` → LED 灭
- [ ] 先发 `ABCD` 再发 `ON`，验证越界保护生效（LED 仍能正常响应）
- [ ] 思考：命令改成 `OPEN`（4 字节），`Rxbuff` 要开多大、`count` 判断要改几处？

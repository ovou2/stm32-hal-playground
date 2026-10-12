# 03-EXTI-KEY · 笔记

## 原理

- EXTI 是**独立的边沿检测器**，触发源是**引脚电平的跳变**，不是"按键"本身
- 传递链：引脚边沿 → `EXTI0_IRQHandler()`（自动生成）→ `HAL_GPIO_EXTI_IRQHandler()`（清标志 + 分发）→ `HAL_GPIO_EXTI_Callback()`（**唯一自己要写的**，放 `USER CODE 4`）
- 回调在 HAL 库里是 `__weak` 空函数，同名重写即覆盖 —— **别改 HAL 库文件**
- EXTI 线按**引脚号**共享，PA0 / PB0 抢同一根 EXTI0，不能同时用

## 接线

```
PB0 ── 按键 ── GND        内部上拉：不按高、按下低 → 下降沿
PA1 ──[220Ω]── LED1 ── GND
PA2 ──[220Ω]── LED2 ── GND
```

## 回调三层过滤

```c
if (GPIO_Pin == KEY_EXTI0_Pin)                                                 /* 哪根线 */
  if (HAL_GPIO_ReadPin(KEY_EXTI0_GPIO_Port, KEY_EXTI0_Pin) == GPIO_PIN_RESET) /* ① 电平 */
  {
    uint32_t now = HAL_GetTick();
    if (now - last_tick >= 50)                                                 /* ② 时间 */
    { last_tick = now; HAL_GPIO_TogglePin(GPIOA, LED1_Pin | LED2_Pin); }
  }
```

| 过滤层 | 挡住什么 |
|--------|---------|
| 电平判定 | 松手瞬间触点**断开**弹跳产生的假下降沿 |
| 时间戳 50ms | 按下瞬间触点**闭合**弹跳产生的连续下降沿 |

> 轮询版（02）靠「等松手」达到同样效果；EXTI 是事件驱动，没这一步，只能靠上面两层补。
> 另需 `HAL_NVIC_EnableIRQ(EXTI0_IRQn)` 放行中断，CubeMX 已自动生成。

## 踩过的坑

1. **★ 改成 EXTI 模式后 Pull 被 CubeMX 重置** —— 从 `GPIO_Input` 切成 `GPXTI0` 时，`Pull-up` 被悄悄改成 `No pull-up/pull-down`，引脚悬空 → 中断要么不来要么乱来
2. **★ 时间戳必须 `uint32_t`** —— 写成 `uint16_t` 时，`HAL_GetTick()` 在 **65535ms（65.5 秒）后回绕**，回绕后差值恒 ≥ 50，消抖彻底失效。现象：刚上电正常，挂机一分钟后发疯
3. **★ 松手也会触发中断** —— 触点断开时弹跳会产生真下降沿，而人手按键 100~300ms 早超出 50ms 窗口 → 「按住亮、松手灭」。修法就是上表的电平判定
4. **中断里禁用 `HAL_Delay()`** —— 它靠 SysTick（优先级 15）走时，EXTI0 优先级 3，在 3 里等 15 = 死锁

## 待办

- [x] 修 `Pull` / 时间戳改 `uint32_t` / 加电平判定
- [ ] 上电干等 70 秒再按，验证回绕问题消失
- [ ] 快速连按 20 次，验证不漏计、不跳数

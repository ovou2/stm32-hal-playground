# 04-TIMER-LED · 笔记

## 原理

**两个版本都做过，对比着看最清楚：**

- **SysTick 版**：SysTick 是 Cortex-M3 **内核自带**的 24 位递减计数器，`HAL_Init()` 把它配成 1ms 一次。在 `SysTick_Handler` 里数 500 次 = 500ms
- **TIM2 版（当前）**：TIM2 是 **STM32 外设**，72MHz 时钟经**两级分频**得到中断频率
  - `Prescaler (PSC)` 先分频：72MHz ÷ (7199+1) = **10kHz**
  - `Counter Period (ARR)` 再分频：10kHz ÷ (9999+1) = **1Hz**
  - 公式：`中断频率 = 定时器时钟 ÷ (PSC+1) ÷ (ARR+1)`，两者都是 16 位，上限 65535

## 代码

```c
/* main.c · USER CODE BEGIN 2 —— 不写这行，中断永远不来 */
HAL_TIM_Base_Start_IT(&htim2);

/* main.c · USER CODE BEGIN 4 */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
  if (htim->Instance == TIM2)                  /* 多个定时器共用一个回调，必须判 */
    HAL_GPIO_TogglePin(GPIOA, LED1_Pin | LED2_Pin);
}
```

## ★ SysTick vs TIM

| | SysTick | TIM2 / TIM3 / TIM4 |
|---|---|---|
| 归属 | Cortex-M3 **内核** | STM32 **外设** |
| 数量 | 全芯片 **1 个** | 多个，各自独立 |
| 频率 | 固定 1ms | `PSC` + `ARR` 自由配 |
| 用途 | `HAL_Delay()` 的时基 | 定时中断 / **PWM** / 输入捕获 |
| 能做 PWM | **不能** | **能 —— 项目全靠它** |

> `System Core → SYS → Timebase Source` 里选 TIM2 **不是**"启用 TIM2"，那是把 HAL 的毫秒时基挪走，反而会让 TIM2 被独占。

## 踩过的坑

1. **★ 忘了 `HAL_TIM_Base_Start_IT(&htim2)`** —— CubeMX 只配硬件、不负责启动。少了这行编译下载都正常，就是不进中断
2. **★ 回调函数名不能自己起** —— 必须是 `HAL_TIM_PeriodElapsedCallback`（HAL 里的 `__weak` 函数），名字写错就成了一个没人调用的普通函数，编译不报错但永不执行
3. **★ CubeMX 里没勾 `TIM2 global interrupt`** —— 同样能编译能下载，就是不进中断，最难查
4. **删 `USER CODE` 里的代码时别把 `BEGIN` / `END` 标记一起删掉** —— 标记要成对留着，否则下次生成找不到位置
5. **`HAL_IncTick()` 不能动** —— 它是 HAL 毫秒时基的累加；SysTick 中断里也别调 `HAL_Delay()`（自己等自己）

## 待办

- [ ] 烧录验证 LED 是 **1 秒一闪**
- [ ] 把 `ARR` 改成 `4999` 重新生成，LED 应快一倍 —— 确认分频真的在起作用

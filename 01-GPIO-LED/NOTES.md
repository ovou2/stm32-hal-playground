# 01-GPIO-LED · 笔记

## 原理

- GPIO 输出 = 往 ODR 寄存器写位；推挽输出能主动输出高低
- 引脚上电默认是浮空输入，需 `MX_GPIO_Init()` 配成输出
- PA1 = GPIOA 的 bit1

## 接线

```
PA1 ──[220Ω]── LED 长脚 ── LED 短脚 ── GND
```

## HAL 关键函数

| 函数 | 作用 |
|------|------|
| `HAL_GPIO_WritePin(GPIOx, Pin, State)` | 设置电平，`GPIO_PIN_SET`(高) / `GPIO_PIN_RESET`(低) |
| `HAL_GPIO_TogglePin(GPIOx, Pin)` | 翻转电平 |
| `HAL_GPIO_ReadPin(GPIOx, Pin)` | 读电平，返回 `GPIO_PinState` |
| `HAL_GPIO_Init(GPIOx, &InitStruct)` | 按配置初始化引脚（CubeMX 自动生成） |
| `HAL_GPIO_DeInit(GPIOx, Pin)` | 恢复引脚到默认状态 |
| `HAL_GPIO_LockPin(GPIOx, Pin)` | 锁定引脚配置，防误改 |

> Pin 参数可以用 `|` 组合，一次改多个引脚：`HAL_GPIO_WritePin(GPIOA, GPIO_PIN_1|GPIO_PIN_2, GPIO_PIN_RESET)`

**`GPIO_InitTypeDef` 四个成员**

- `Pin` —— `GPIO_PIN_x`，可或运算组合
- `Mode` —— `OUTPUT_PP` 推挽 / `OUTPUT_OD` 开漏 / `INPUT` 输入 / `AF_PP` 复用 / `ANALOG` 模拟 / `IT_RISING` 等中断触发
- `Pull` —— `NOPULL` / `PULLUP` / `PULLDOWN`
- `Speed` —— `LOW` / `MEDIUM` / `HIGH`（翻转速度，不是电平高低）

**寄存器快写**（原子操作，比 HAL 函数快，做高速采样时会用）

```c
GPIOA->BSRR = GPIO_PIN_1;          /* 置位 */
GPIOA->BSRR = GPIO_PIN_1 << 16;    /* 复位 */
```

## 踩过的坑

1. **下载完灯不亮** —— Keil 没勾 `Reset and Run`，MCU 停在暂停状态，程序没跑。**按板子 RESET 键灯亮即确诊**。解决：魔术棒(`Alt+F7`) → Debug → Settings → Flash Download → 勾 `Reset and Run`
2. **LED 必须串限流电阻** —— 不串时电流约 20~30mA，超引脚 ±25mA 绝对上限。用 220Ω ~ 1kΩ
3. **PA1 上没有板载 LED** —— 蓝板的板载灯在 PC13，且低电平点亮；PA1 必须外接

## 待办

- [x] 把 `WritePin` 改成 `TogglePin` + `HAL_Delay(500)`，做成闪烁

# 02-GPIO-button · 笔记

## 原理

- GPIO 输入 = 读 IDR 寄存器里的位
- 按键配上拉 → 不按读高、按下读低（`GPIO_PIN_RESET`）；引脚悬空时读数随机跳，所以输入脚不能设 `NOPULL`
- 机械按键按下/松开的几毫秒内触点会弹跳（**抖动**），必须消抖

## 接线

```
PB0  ── 按键1 ── GND        按键用内部上拉，不需外接电阻
PB10 ── 按键2 ── GND
PA1 ──[220Ω]── LED1 ── GND
PA2 ──[220Ω]── LED2 ── GND
```

> 蓝板上**没有可编程按键**（只有 RESET 和 BOOT0），必须外接。

## 消抖逻辑

```
读到低 → 延时 20ms → 再确认仍是低 → 等松手 → 算一次有效按下
```

「再确认一次」不能省，少了它抖动期的一次误读就会被当成真按下。

## HAL 关键函数

| 函数 | 作用 |
|------|------|
| `HAL_GPIO_ReadPin(GPIOx, Pin)` | 读电平，返回 `GPIO_PIN_SET`(高) / `GPIO_PIN_RESET`(低) |
| `HAL_GPIO_TogglePin(GPIOx, Pin)` | 翻转电平，**不用自己写"读 + 判断 + 写"** |
| `HAL_GPIO_WritePin(GPIOx, Pin, State)` | 设置电平 |

## 踩过的坑

1. **消抖只延时、没二次确认** —— 抖动期的一次误读会被当成真按下
2. **`while(读到低);` 是阻塞等待** —— 按住不松手主循环就卡死，轮询式的固有缺陷，学完中断要换掉
3. **`RESET` / `SET` 不是 HAL 的枚举** —— 那是 CMSIS 的 `FlagStatus`，应写 `GPIO_PIN_RESET` / `GPIO_PIN_SET`
4. **硬编码 `GPIOA, GPIO_PIN_1`** —— `main.h` 里已有 `LED1_Pin` 宏，不用的话改引脚要满文件找

## 待办

- [x] 消抖加「二次确认」
- [x] `LED1_turn()` → `HAL_GPIO_TogglePin()`；删掉定义了却没调用的 `LED*_ON()`
- [x] 统一改用 `LED1_Pin` / `LED1_GPIO_Port` 宏
- [ ] 重新编译 + 烧录，验证按键能翻转对应 LED

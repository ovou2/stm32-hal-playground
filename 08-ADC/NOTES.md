# 08-ADC · 笔记

## 原理

- **12 位 ADC**：把 0~3.3V 切成 4096 份，读数 `0~4095`，分辨率 `3.3/4096 ≈ 0.8mV`
- **ADC 时钟 = PCLK2 / 6 = 12MHz**（F103 上限 14MHz，超了精度崩）
- **一次转换耗时 = 采样时间 + 12.5 个周期**；单次模式下每次转换都要重新 `Start`
- **光敏电阻**：光强↑ → 阻值↓ → 分压点电压变；模块把电阻变化转成电压给 ADC

## 接线

```
光敏模块 VCC ── 3.3V      ★ 别接 5V，AO 会超过 3.3V，PA4 不耐 5V
光敏模块 GND ── GND
光敏模块 AO  ── PA4 (ADC1_IN4)
```

## 单次转换四步（本项目写法）

```c
HAL_ADCEx_Calibration_Start(&hadc1);            /* ① 上电校准，F1 必做 */
while (1) {
  HAL_ADC_Start(&hadc1);                        /* ② 启动一次转换 */
  if (HAL_ADC_PollForConversion(&hadc1, 50) == HAL_OK)   /* ③ 等 EOC 置位 */
    adc_val = HAL_ADC_GetValue(&hadc1);         /* ④ 读结果 */
  voltage = (float)adc_val / 4095 * 3.3f;
}
```

## HAL 关键函数

| 函数 | 作用 |
|------|------|
| `HAL_ADCEx_Calibration_Start(&hadc1)` | 校准（F1 独有）。不做会有几十 mV 固定偏差 |
| `HAL_ADC_Start(&hadc1)` | 启动转换。单次模式下**每轮都要调** |
| `HAL_ADC_PollForConversion(&hadc1, ms)` | 阻塞等转换完成，等到返回 `HAL_OK`，超时返回 `HAL_TIMEOUT` |
| `HAL_ADC_GetValue(&hadc1)` | 读 `DR` 寄存器里的 12 位原始值 |
| `HAL_ADC_Stop(&hadc1)` | 停止转换 |
| `HAL_ADC_Start_IT` + `HAL_ADC_ConvCpltCallback` | 中断方式：转换完成自动进回调 |
| `HAL_ADC_Start_DMA(&hadc1, buf, len)` | DMA 方式：连续/多通道采样不占 CPU |

**三种用法怎么选**：单次轮询（本例，最简单）→ 中断（不想占主循环）→ DMA（多通道 / 高速连续）。

## CubeMX 里真正要配的 4 项

| 项 | 本项目值 | 说明 |
|----|----------|------|
| Channel | `ADC1_IN4` | 对应引脚 PA4 |
| Sampling Time | `1.5 cycles` → **改 239.5** | 光敏分压输出阻抗高，采样时间必须给足 |
| Continuous Conversion | `Disabled` | 关掉才能每轮手动 `Start` |
| Scan Conversion | `Disabled` | 单通道用不上；多通道才开 |

## 踩过的坑

1. **★ `SYS → Debug` 选成了 `No Debug`** —— 生成 `__HAL_AFIO_REMAP_SWJ_DISABLE()`，程序一跑 SWD 就废，**烧录报找不到芯片，只能按住复位硬烧**。改回 `Serial Wire` 重新生成
2. **★ 采样时间用 1.5 周期** —— 光敏电阻分压阻抗高，采样电容充不满，读数乱飘。用 239.5 周期
3. **★ 忘了校准** —— `HAL_ADCEx_Calibration_Start` 没调，读数整体偏几十 mV，还以为是硬件问题
4. **单次模式下忘了每轮 `Start`** —— 只在初始化时启动一次，之后读数永远不变
5. **模块接 5V 供电** —— AO 可能输出 5V，PA4 不是 5V 容忍引脚，会打坏

## 待办

- [ ] CubeMX 改 `SYS → Debug = Serial Wire` → 重新生成 → 验证能直接烧录
- [ ] 采样时间 1.5 → 239.5 周期，对比读数稳定性
- [ ] 手遮挡 / 手机照光，观察电压往哪个方向变；进阶换成 `HAL_ADC_Start_IT` 中断方式

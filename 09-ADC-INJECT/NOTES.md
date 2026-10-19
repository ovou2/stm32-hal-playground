# 09-ADC-INJECT · 笔记

## 原理

- 一个 ADC 有两组转换队列：**规则组**（最多 16 通道，结果**共用一个 `DR`**）和**注入组**（最多 4 通道，结果各存 `JDR1~JDR4`）
- 注入组能**插队**打断正在进行的规则转换，规则组已转换完的结果不会丢
- 两组各有完成标志（`EOC` / `JEOC`）和各自的回调，但**共用 `ADC1_2_IRQn` 一个中断向量**
- 本项目分工：规则组 = PA4 光敏（`IN4`，持续采）；注入组 = 内部温度（`IN16`）

## 两条调用链

```
规则组
HAL_ADC_Start_IT()
  → 转换完成置 EOC → ADC1_2_IRQHandler() → HAL_ADC_IRQHandler()
  → HAL_ADC_ConvCpltCallback()  → HAL_ADC_GetValue()          读 DR

注入组
HAL_ADCEx_InjectedStart_IT()
  → 转换完成置 JEOC → ADC1_2_IRQHandler() → HAL_ADC_IRQHandler()
  → HAL_ADCEx_InjectedConvCpltCallback() → HAL_ADCEx_InjectedGetValue()  读 JDR1
```

## HAL 关键函数

| 函数 | 作用 |
|------|------|
| `HAL_ADCEx_Calibration_Start(&hadc1)` | 校准，F1 必做 |
| `HAL_ADC_Start_IT(&hadc1)` | 规则组·中断方式启动 |
| `HAL_ADC_GetValue(&hadc1)` | 读规则组 `DR` |
| `HAL_ADCEx_InjectedStart_IT(&hadc1)` | 注入组·中断方式启动 |
| `HAL_ADCEx_InjectedGetValue(&hadc1, ADC_INJECTED_RANK_1)` | 读注入组 `JDR1`（RANK_1~4 对应 JDR1~4） |
| `HAL_ADCEx_InjectedStop_IT(&hadc1)` | 停注入组 |
| `HAL_ADC_ConvCpltCallback` / `HAL_ADCEx_InjectedConvCpltCallback` | 两个独立回调，各写各的 |

## 温度换算（内部 `IN16`）

```c
vsense = adc_val[1] * 3.3f / 4095.0f;          /* 原始值 → 电压 */
temp   = (1.43f - vsense) / 0.0043f + 25.0f;   /* V25=1.43V，斜率 4.3mV/℃ */
```

`V25` 个体差异大（1.34~1.52V），绝对值可能差十几度，只能看趋势；想准就用 `Vrefint`（`IN17`，1.2V）反推真实 VDD 再换算。

## CubeMX 要配的 5 项

| 项 | 值 |
|----|----|
| `EnableInjectedConversion` | Enabled |
| Injected `Number Of Conversions` | 1 |
| Injected Rank1 Channel | `Channel 16`（内部温度传感器） |
| Injected 采样时间 | **239.5 周期**（手册要求 ≥17.1µs） |
| NVIC | 勾 `ADC1_2_IRQn` |

## 踩过的坑

1. **★ 注入组用 `HAL_ADC_GetValue` 读** —— 读到的还是规则组的值（共用 DR 那个），注入结果必须走 `InjectedGetValue`
2. **★ 内部温度采样时间不足** —— 用 1.5 周期读出来是几十度的乱数，必须 239.5 周期
3. **★ 两组要分别启动** —— 只调 `HAL_ADC_Start_IT` 注入组永远不动，反之亦然
4. **注入组的外部触发选 Software** —— 不选触发源，`InjectedStart_IT` 也起不来
5. **光敏 1.5 周期也偏短** —— 分压电路阻抗高，光敏这一路同样建议 239.5 周期

## 待办

- [ ] 两组采样时间统一改 239.5，对比读数稳定性
- [ ] 加 `IN17`（Vrefint）反推真实 VDD，再算温度
- [ ] 进阶：规则组连续转换 + 注入组由 TIM 触发，验证"插队不丢数据"

# 参考 · CubeMX 生成的代码结构

> CubeMX 生成的是"骨架 + 一堆空格子"，你的代码只能塞进格子里。

## 一条铁律

**只写在 `/* USER CODE BEGIN xxx */` 和 `/* USER CODE END xxx */` 之间。**

写在格子外面 → 下次点 `GENERATE CODE` → 被覆盖，代码全没。

---

## main.c 分区地图

按文件从上到下的顺序。（行号是 `01-GPIO-LED` 当前的实际位置，重新生成后会变）

| 分区名 | 行 | 含义 | 放什么 |
|---|---|---|---|
| `Header` | 1 | 文件头注释 | 不用动 |
| `Includes` | 24 | 额外的 `#include` | `#include <stdio.h>` |
| `PTD` | 29 | Private TypeDef —— 类型定义 | `typedef struct {...} XXX;` |
| `PD` | 34 | Private Define —— 无参宏 | `#define BLINK_MS 500` |
| `PM` | 39 | Private Macro —— 带参宏 | `#define MAX(a,b) ((a)>(b)?(a):(b))` |
| `PV` | 45 | Private Variable —— **全局变量** | `uint8_t flag = 0;` |
| `PFP` | 51 | Private Function Prototype —— 函数**声明** | `void LedBlink(void);` |
| `0` | 56 | 用户区（在 `main()` **之前**） | 自己函数的**实现** |
| `1` | 67 | `main()` 开头 | main 里的局部变量 |
| `Init` | 76 | 在 `HAL_Init()` 之后 | 极少用 |
| `SysInit` | 83 | 在 `SystemClock_Config()` 之后 | 极少用 |
| `2` | 89 | 外设初始化完、进 `while` 之前 | 外设启动、上电初值 |
| `WHILE` | 94 | `while(1)` 循环体 | ⭐ **主逻辑写这里** |
| `3` | 102 | 同一个循环体的末尾 | 和 `WHILE` 等价 |
| `4` | 146 | 在 `main()` **之后** | ⭐ 函数实现 / **中断回调** |
| `Error_Handler_Debug` | 156 | `Error_Handler()` 内部 | 出错时打印信息 |
| `6` | 174 | `assert_failed()` 内部 | 断言报错 |

---

## 速查：我想写 XX，放哪个分区？

| 我要写的东西 | 放哪儿 |
|---|---|
| 全局变量 | `PV` |
| main 里的局部变量 | `1` |
| 常量宏 `#define` | `PD` |
| 自定义类型 `typedef` | `PTD` |
| 自己写的函数 —— **声明** | `PFP` |
| 自己写的函数 —— **实现** | `4`（推荐）或 `0` |
| 要 include 的头文件 | `Includes` |
| 外设启动（如 `HAL_TIM_Base_Start_IT`） | `2` |
| 主循环逻辑 | `WHILE` |
| 中断回调（如 `HAL_GPIO_EXTI_Callback`） | `4` |

**参数/变量写在哪，看它的"活多久、谁看得见"：**

- 整个程序都要用 → 全局变量 → `PV`
- 只在 main 里用 → 局部变量 → `1`
- 只是个固定数字 → 宏 → `PD`

---

## 三组"看起来重复"的分区怎么选

| 看着重复 | 区别 | 选哪个 |
|---|---|---|
| `0` vs `4` | 一个在 `main()` 前、一个在 `main()` 后，都是给函数实现用的 | **`4`** —— 在文件尾，不打断 main 的可读性 |
| `WHILE` vs `3` | 同一个 `while(1)` 循环体，`WHILE` 紧跟 `{`，`3` 在循环末尾 | **`WHILE`** —— CubeMX 有时会往 `3` 里插东西 |
| `PTD/PD/PFP` vs `main.h` 里的 `ET/EC/EFP` | 前者"私有"，只在本文件可见；后者"导出"，可以被别的 .c `#include` | 只在本文件用 → 写 main.c；要给别的文件用 → 写 main.h |

**`PFP` 只是个声明。** 函数体写在 `4`（`main()` 之后）时，如果 `PFP` 里没声明，编译器会报 `implicit declaration of function`。写 `0`（main 之前）则不需要声明。

### 补充：`USER CODE BEGIN WHILE` 其实不在 while 里面

CubeMX 生成的原样：

```c
  /* USER CODE BEGIN WHILE */   ← 在 while 的上一行（不在循环体内）
  while (1)
  {
    /* USER CODE END WHILE */   ← 在 { 的下一行

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */         ← 在 } 的下面
```

**两对标记各保护 `while (1)` 的一半：**

| 标记 | 从哪到哪 | 保护了什么 |
|---|---|---|
| `BEGIN WHILE` → `END WHILE` | `while (1)` 的上一行 → `{` 的下一行 | `while (1) {` 本身 + 循环体**开头** |
| `BEGIN 3` → `END 3` | 循环体中部 → `}` 的下面 | 循环体**结尾** + 右大括号 `}` |

**所以你写主循环代码有两个地方，都在循环体内，都会被保留：**

1. `while (1) {` 的下一行 → `/* USER CODE END WHILE */` 的上一行 ← **推荐**（阅读顺序在前）
2. `/* USER CODE BEGIN 3 */` 之后 → `}` 之前

`while (1) {` 和 `}` 被 CubeMX 包进保护区，是为了保证这对大括号不会被改写掉。

---

## 标记的确切形态

```c
/* USER CODE BEGIN 3 */
    你的代码
/* USER CODE END 3 */
```

- `BEGIN` 和 `END` 后面的名字必须**一字不差**，CubeMX 靠配对名字来识别保护区
- 名字不要手改。改了 CubeMX 会在新位置再插一份标记，你的代码就变成"孤儿"了

---

## 相关文件的分区（用到时再回来看）

| 文件 | 常写分区 | 用途 |
|---|---|---|
| `Core/Inc/main.h` | `Private defines` | CubeMX 自动生成的引脚宏（`LED1_Pin` / `LED1_GPIO_Port`） |
| | `Includes` `ET` `EC` `EM` `EFP` | **跨文件**用的类型 / 常量 / 宏 / 函数声明 |
| `Core/Src/gpio.c` | `0` `2` | GPIO 相关的自己的代码 |
| `Core/Src/stm32f1xx_it.c` | `xxx_IRQn 0` / `1` | ⭐ **中断服务函数 ISR**（第 2 周 EXTI 会动这里） |
| `Core/Src/stm32f1xx_hal_msp.c` | `MspInit 0` / `1` | 外设底层：时钟使能、引脚复用（一般不用改） |

---

## 口诀

> **格子外面写的代码，都是借来的；下次生成就还回去了。**

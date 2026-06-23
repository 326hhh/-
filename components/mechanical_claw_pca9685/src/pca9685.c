/**
 * @file    pca9685.c
 * @brief   PCA9685 16通道PWM驱动模块实现
 * @note    提供舵机角度控制接口，内部封装 I2C 通信及寄存器操作
 */
#include "pca9685.h"
#include "i2c.h"

/* ==================== 私有宏定义 ==================== */
#define PCA9685_ADDR    (0x40 << 1)                 /**< 设备 I2C 写地址 (7位地址 0x40 左移 1 位) */
#define REG_MODE1       0x00                        /**< 模式寄存器1 (睡眠/唤醒/自动递增/重启) */
#define REG_MODE2       0x01                        /**< 模式寄存器2 (输出驱动配置) */
#define REG_PRESCALE    0xFE                        /**< 预分频寄存器 (设置 PWM 频率) */
#define REG_LED0_ON_L   0x06                        /**< 通道0 ON 计数低字节 (通道起始地址) */                                               

/** @brief 舵机脉宽边界值 (基于 50Hz PWM，每计数步长约 0.5μs×prescale≈0.5μs×122≈61μs) */
#define SERVO_MIN_TICK  102                         /**< 0° 对应高电平时间 0.5ms (102 步) */
#define SERVO_MAX_TICK  512                         /**< 180° 对应高电平时间 2.5ms (512 步) */

/* ==================== 私有函数声明 ==================== */
static void PCA9685_WriteReg(uint8_t reg, uint8_t dat);
static void PCA9685_SetFreq(uint16_t freq);
static void PCA9685_SetPWM_Raw(uint8_t ch, uint16_t on, uint16_t off);

/* ==================== 私有函数实现 ==================== */

/**
 * @brief  向指定寄存器写入单字节数据
 * @param  reg  寄存器地址
 * @param  dat  待写入数据
 * @retval 无
 */
static void PCA9685_WriteReg(uint8_t reg, uint8_t dat)
{
    HAL_I2C_Mem_Write(&hi2c1, PCA9685_ADDR, reg, I2C_MEMADD_SIZE_8BIT, &dat, 1, 100);
}

/**
 * @brief  设置 PWM 输出频率
 * @param  freq  目标频率 (单位 Hz，典型舵机使用 50Hz)
 * @note   操作流程：
 *         1. 计算预分频值：prescale = 25MHz / (4096 * freq) - 1
 *         2. 进入睡眠模式，允许修改预分频寄存器
 *         3. 写入新预分频值
 *         4. 唤醒并设置 RESTART 位，重新启动 PWM 输出
 */
static void PCA9685_SetFreq(uint16_t freq)
{
    // 计算预分频值（内部振荡器 25MHz，四舍五入取整）
    uint8_t pre = (uint8_t)(25000000.0f / (4096.0f * freq) - 1 + 0.5f);
    PCA9685_WriteReg(REG_MODE1, 0x10);           // 进入睡眠 (SLEEP=1, OSC 关闭)
    PCA9685_WriteReg(REG_PRESCALE, pre);         // 设置分频器
    PCA9685_WriteReg(REG_MODE1, 0xA0);           // 唤醒 (SLEEP=0) + 自动增量 (AI=1) + 重启 (RESTART=1)               
    HAL_Delay(1);                                // 等待内部振荡器稳定 (>500µs）
}

/**
 * @brief  直接设置指定通道的 PWM ON/OFF 计数值（底层接口）
 * @param  ch   通道号 (0~15)
 * @param  on   高电平起始计数值 (0~4095)
 * @param  off  高电平结束计数值 (0~4095)，决定占空比
 * @note   每个通道占用 4 个连续寄存器 (LEDx_ON_L/H, LEDx_OFF_L/H)
 *         高 4 位自动清零，仅低 12 位有效
 */
static void PCA9685_SetPWM_Raw(uint8_t ch, uint16_t on, uint16_t off)
{
    uint8_t buf[4] = {
        on & 0xFF,                                  // ON 低 8 位
        (on >> 8) & 0x0F,                           // ON 高 4 位
        off & 0xFF,                                 // OFF 低 8 位
        (off >> 8) & 0x0F                           // OFF 高 4 位
    };
    HAL_I2C_Mem_Write(&hi2c1, PCA9685_ADDR, 
                    REG_LED0_ON_L + ch * 4, 
                    I2C_MEMADD_SIZE_8BIT,           // 计算通道寄存器首地址
                    buf, 4, 100);
}

/* ==================== 对外接口函数 ==================== */
 
/**
 * @brief  初始化 PCA9685 模块（50Hz 推挽输出）
 * @note   调用后所有通道频率设置为 50Hz，输出模式为推挽，可直接驱动舵机
 */
void PCA9685_Init(void)
{
    PCA9685_SetFreq(50);               // 设定舵机标准工作频率
    PCA9685_WriteReg(REG_MODE2, 0x04); // 推挽输出 (OUTDRV=1)，增强驱动能力
}

/**
 * @brief  控制单个舵机转动到指定角度
 * @param  ch     通道号 (0~15)，超出范围自动钳位为 15
 * @param  angle  目标角度 (0~180°)，超范围自动钳位为 180°
 * @note   角度与脉宽线性映射：0°→0.5ms，180°→2.5ms
 */
void Servo_SetAngle(uint8_t ch, uint8_t angle)
{
    /* 参数保护 */
    if(ch > 15)  ch = 15;
    if(angle > 180) angle = 180;
    
    /* 线性插值计算脉宽计数值 */
    uint16_t tick = SERVO_MIN_TICK + 
    (SERVO_MAX_TICK - SERVO_MIN_TICK) * angle / 180;

    PCA9685_SetPWM_Raw(ch, 0, tick);// ON=0 时刻开始高电平，OFF=tick 结束
}

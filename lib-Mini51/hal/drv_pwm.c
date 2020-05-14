#include "hal.h"
#include "drv_pwm.h"
#include "config.h"
#include "lib_timers.h"

#define PULSE_1MS       (2750) // 1ms pulse width => 1mS at 22Mhz/8 (WHY IS THIS 2MS AND NOT ONE THIS MAKES NO SENSE ALL MY MATH IS WRONG WAAH) //Fix by DIV_4 instad of DIV_8??

#define MWII_PWM_MAX  PULSE_1MS*20 // 1mS at 22Mhz/8
#define MWII_PWM_PRE    1

// returns whether driver is asking to calibrate throttle or not
bool pwmInit(drv_pwm_config_t *init)
{
#if (CONTROL_BOARD_TYPE == CONTROL_BOARD_HUBSAN_H107L) || (CONTROL_BOARD_TYPE == CONTROL_BOARD_HUBSAN_Q4)
		
    CLK_EnableModuleClock(PWM01_MODULE);
    CLK_EnableModuleClock(PWM23_MODULE);

    // PWM clock source
    CLK->CLKSEL1 &= ~(CLK_CLKSEL1_PWM23_S_Msk | CLK_CLKSEL1_PWM01_S_Msk);
    CLK->CLKSEL1 |= CLK_CLKSEL1_PWM23_S_HCLK | CLK_CLKSEL1_PWM01_S_HCLK;
    
    // Multifuncional pin set up PWM0-4
    SYS->P2_MFP &= ~(SYS_MFP_P22_Msk | SYS_MFP_P23_Msk | SYS_MFP_P24_Msk | SYS_MFP_P25_Msk);
    SYS->P2_MFP |= SYS_MFP_P22_PWM0 | SYS_MFP_P23_PWM1 | SYS_MFP_P24_PWM2 | SYS_MFP_P25_PWM3;

#define MWII_PWM_MASK ((1 << 0) | (1 << 1) | (1 << 2) | (1 << 3))

    // Even channel N and N+1 share prescaler
    PWM_SET_PRESCALER(PWM, 0, MWII_PWM_PRE);
    PWM_SET_PRESCALER(PWM, 2, MWII_PWM_PRE);
    PWM_SET_DIVIDER(PWM, 0, PWM_CLK_DIV_1);
    PWM_SET_DIVIDER(PWM, 1, PWM_CLK_DIV_1);
    PWM_SET_DIVIDER(PWM, 2, PWM_CLK_DIV_1);
    PWM_SET_DIVIDER(PWM, 3, PWM_CLK_DIV_1);

    PWM_Start(PWM, MWII_PWM_MASK);
    // No analog of PWM_Start for enabling auto-reload mode
    for (int i = 0; i <= 3; ++i) {
        PWM->PCR |= PWM_PCR_CH0MOD_Msk << (4 * i);
    }
    
    // Duty
    PWM_SET_CMR(PWM, 0, 0);
    PWM_SET_CMR(PWM, 1, 0);
    PWM_SET_CMR(PWM, 2, 0);
    PWM_SET_CMR(PWM, 3, 0);
    // Period, actually sets it to safe value 1000+1 
    PWM_SET_CNR(PWM, 0, MWII_PWM_MAX);
    PWM_SET_CNR(PWM, 1, MWII_PWM_MAX);
    PWM_SET_CNR(PWM, 2, MWII_PWM_MAX);
    PWM_SET_CNR(PWM, 3, MWII_PWM_MAX);
	
// end of CONTROL_BOARD_TYPE == CONTROL_BOARD_HUBSAN_H107L
#elif (CONTROL_BOARD_TYPE == CONTROL_BOARD_JXD_JD385) || (CONTROL_BOARD_TYPE == CONTROL_BOARD_WLT_V202)
    CLK_EnableModuleClock(PWM23_MODULE);
    CLK_EnableModuleClock(PWM45_MODULE);

    
		///* Switch HCLK clock source to XTL */ //ESC CODE!!
    CLK_SetHCLK(CLK_CLKSEL0_HCLK_S_XTAL,CLK_CLKDIV_HCLK(1)); //now there are 22 cycles per microsecond, 22mhz

    // PWM clock source
    CLK->CLKSEL1 &= ~CLK_CLKSEL1_PWM23_S_Msk;
    CLK->CLKSEL1 |= CLK_CLKSEL1_PWM23_S_HCLK; 
		volatile uint32_t freq = CLK_GetHCLKFreq(); //But what is HCLK? Running this shows => 0x2DC4 or 11,716Hz. We're going to want more like 3.2Mhz to get 20mS overflow at 65525 value for ESC
    CLK->CLKSEL2 &= ~CLK_CLKSEL2_PWM45_S_Msk;
    CLK->CLKSEL2 |= CLK_CLKSEL2_PWM45_S_HCLK;
    
    // Multifuncional pin set up PWM2-5
    SYS->P2_MFP &= ~(SYS_MFP_P24_Msk | SYS_MFP_P25_Msk | SYS_MFP_P26_Msk);
    SYS->P2_MFP |= SYS_MFP_P24_PWM2 | SYS_MFP_P25_PWM3 | SYS_MFP_P26_PWM4;
    SYS->P0_MFP &= ~SYS_MFP_P04_Msk;
    SYS->P0_MFP |= SYS_MFP_P04_PWM5;
    
    //

#define MWII_PWM_MASK ((1 << 2) | (1 << 3) | (1 << 4) | (1 << 5))

//    SYS_ResetModule(PWM_RST);

    // Even channel N and N+1 share prescaler
    PWM_SET_PRESCALER(PWM, 2, MWII_PWM_PRE); //this is just 1
    PWM_SET_PRESCALER(PWM, 4, MWII_PWM_PRE); //this is just 1
    PWM_SET_DIVIDER(PWM, 2, PWM_CLK_DIV_4); //this will get us 22/8 = 2.275Mhz or 363nS per tick, overflow at 23.8mS
    PWM_SET_DIVIDER(PWM, 3, PWM_CLK_DIV_4);
    PWM_SET_DIVIDER(PWM, 4, PWM_CLK_DIV_4);
    PWM_SET_DIVIDER(PWM, 5, PWM_CLK_DIV_4);

    PWM_Start(PWM, MWII_PWM_MASK);
    // No analog of PWM_Start for enabling auto-reload mode
    for (int i = 2; i <= 5; ++i) {
        PWM->PCR |= PWM_PCR_CH0MOD_Msk << (4 * i);
    }
//    PWM->PCR = PWM_PCR_CH3EN_Msk | PWM_PCR_CH3MOD_Msk;
    
  
    // Duty
		//needs to be between 1mS and 2mS or 5500
		//high cal
    //PWM_SET_CMR(PWM, 2,  (2000ul*PULSE_1MS)/1000ul);
    //PWM_SET_CMR(PWM, 3,  (2000ul*PULSE_1MS)/1000ul);
    //PWM_SET_CMR(PWM, 4,  (2000ul*PULSE_1MS)/1000ul);
    //PWM_SET_CMR(PWM, 5,  (2000ul*PULSE_1MS)/1000ul);
		
		PWM_SET_CMR(PWM, 2,  (PULSE_1MS)/1000ul);
    PWM_SET_CMR(PWM, 3,  (PULSE_1MS)/1000ul);
    PWM_SET_CMR(PWM, 4,  (PULSE_1MS)/1000ul);
    PWM_SET_CMR(PWM, 5,  (PULSE_1MS)/1000ul);
		
		
		
    // Period, start counting down from max value for a 20mS period
    PWM_SET_CNR(PWM, 2, MWII_PWM_MAX);  
    PWM_SET_CNR(PWM, 3, MWII_PWM_MAX);
    PWM_SET_CNR(PWM, 4, MWII_PWM_MAX);
    PWM_SET_CNR(PWM, 5, MWII_PWM_MAX);
		
		
		
#endif // 
    PWM_EnableOutput(PWM, MWII_PWM_MASK);
   
		//lib_timers_delaymilliseconds(10000); 
		
		//PWM_SET_CMR(PWM, 2, PULSE_1MS);
    //PWM_SET_CMR(PWM, 3, PULSE_1MS);
    //PWM_SET_CMR(PWM, 4, PULSE_1MS);
    //PWM_SET_CMR(PWM, 5, PULSE_1MS);
    
		return false;
}
void pwmWriteMotor(uint8_t index, uint16_t value)
{
#if CONTROL_BOARD_TYPE == CONTROL_BOARD_WLT_V202
    // Motor 0 BACK_R  - PWM4
    // Motor 1 FRONT_R - PWM5
    // Motor 2 BACK_L  - PWM3
    // Motor 3 FRONT_L - PWM2
    static uint8_t motor_to_pwm[] = { 4, 5, 3, 2 };
#elif CONTROL_BOARD_TYPE == CONTROL_BOARD_JXD_JD385
    // Motor 0 BACK_R  - PWM2
    // Motor 1 FRONT_R - PWM3
    // Motor 2 BACK_L  - PWM5
    // Motor 3 FRONT_L - PWM4
    static uint8_t motor_to_pwm[] = { 2, 3, 5, 4 };
#elif (CONTROL_BOARD_TYPE == CONTROL_BOARD_HUBSAN_H107L) || (CONTROL_BOARD_TYPE == CONTROL_BOARD_HUBSAN_Q4)
    // Motor 1 BACK_R  - PWM2
    // Motor 3 FRONT_R - PWM3
    // Motor 2 BACK_L  - PWM1
    // Motor 4 FRONT_L - PWM0
    static uint8_t motor_to_pwm[] = { 2, 3, 1, 0};
#endif
    if (index > 3) return;
		PWM_SET_CMR(PWM, motor_to_pwm[index], (value*PULSE_1MS)/1000ul); //convert 1 millisecond to microsecond and scale the input value. 
}

// Not implmented
//void pwmWriteServo(uint8_t index, uint16_t value)
//{
//}
//uint16_t pwmRead(uint8_t channel)
//{
//    return 0;
//}

#include "main.h"

#define __READ_BIT(var,pos) ((var) & (1<<(pos)))
#define __CLEAR_BIT(var,pos) ((var) &= ~(1<<(pos)))
#define __SET_BIT(var,pos) ((var) |= (1<<(pos)))
#define __TOGGLE_BIT(var,pos) ((var) ^= (1<<(pos)))

#define DAC_DHR8R1_Address              0x40007410
#define PI                              3.1415926535897932
#define WAVEFORM_SQUARE                 1
#define WAVEFORM_TRIANGLE               2
#define WAVEFORM_SAWTOOTH               3
#define WAVEFORM_SINEWAVE               4
#define WAVEFORM_MIN_VALUE              0
#define WAVEFORM_MAX_VALUE              255

// Those parameters control the output signal on the DAC
#define DAC_WAVEFORM_TYPE               WAVEFORM_SINEWAVE
#define DAC_SAMPLING_TIME_US            5
#define DAC_WAVEFORM_SIZE               100
#define DAC_SIGNAL_LOW_VALUE            WAVEFORM_MIN_VALUE
#define DAC_SIGNAL_HIGH_VALUE           WAVEFORM_MAX_VALUE

void GenerateDACsignal();
void Configure_RCC();
void Configure_GPIOs();
void Configure_TIM6();
void Configure_DAC1();
void Configure_DMA1();

uint8_t DAC_signal[1024];

int main(void)
{
  uint32_t count = 0;
  
  GenerateDACsignal();
  
  Configure_RCC();
  Configure_GPIOs();
  Configure_TIM6();
  Configure_DAC1();
  Configure_DMA1();
  
  __CLEAR_BIT(GPIOC->ODR, 8);
  __SET_BIT(GPIOC->ODR, 9);
  
  DMA_Cmd(DMA1_Channel3, ENABLE);
  DAC_Cmd(DAC_Channel_1, ENABLE);
  DAC_DMACmd(DAC_Channel_1, ENABLE);
  TIM_Cmd(TIM6, ENABLE);
  
  while(1)
  {
    if (count++>= 1000000)
    {
      count = 0;
      __TOGGLE_BIT(GPIOC->ODR, 8);
      __TOGGLE_BIT(GPIOC->ODR, 9);
    }
  }
}

void GenerateDACsignal()
{
  uint16_t count = 0;
  
  if (DAC_WAVEFORM_TYPE == WAVEFORM_SQUARE)
  {
    while (count < DAC_WAVEFORM_SIZE/2)
      DAC_signal[count++] = DAC_SIGNAL_LOW_VALUE;
    while (count < DAC_WAVEFORM_SIZE)
      DAC_signal[count++] = DAC_SIGNAL_HIGH_VALUE;
  }
  else if (DAC_WAVEFORM_TYPE == WAVEFORM_TRIANGLE)
  {
    while (count < DAC_WAVEFORM_SIZE/2)
    {
      DAC_signal[count] = DAC_SIGNAL_LOW_VALUE + (uint8_t)((double)(DAC_SIGNAL_HIGH_VALUE - DAC_SIGNAL_LOW_VALUE) / (double)DAC_WAVEFORM_SIZE * (double)count * 2.0);
      count++;
    }
    while (count < DAC_WAVEFORM_SIZE)
    {
      DAC_signal[count] = DAC_SIGNAL_HIGH_VALUE - (uint8_t)((double)(DAC_SIGNAL_HIGH_VALUE - DAC_SIGNAL_LOW_VALUE) / (double)DAC_WAVEFORM_SIZE * (double)(count - DAC_WAVEFORM_SIZE/2) * 2.0);
      count++;
    }
  }
  else if (DAC_WAVEFORM_TYPE == WAVEFORM_SAWTOOTH)
  {
    while (count < DAC_WAVEFORM_SIZE)
    {
      DAC_signal[count] = DAC_SIGNAL_LOW_VALUE + (uint8_t)((double)(DAC_SIGNAL_HIGH_VALUE - DAC_SIGNAL_LOW_VALUE) / (double)DAC_WAVEFORM_SIZE * (float)count);
      count++;
    }
  }
  else if (DAC_WAVEFORM_TYPE == WAVEFORM_SINEWAVE)
  {
    while (count < DAC_WAVEFORM_SIZE)
    {
      DAC_signal[count] = DAC_SIGNAL_LOW_VALUE + (uint8_t)( (double)(DAC_SIGNAL_HIGH_VALUE - DAC_SIGNAL_LOW_VALUE) * (1.0 + sin((double)count * 2.0 * PI / (double)DAC_WAVEFORM_SIZE)) / 2.0);
      count++;
    }
  }
}

void Configure_RCC()
{
  RCC->APB2ENR |= RCC_APB2ENR_IOPCEN;                                           // Enable GPIOC Clock
  RCC->APB1ENR |= RCC_APB1Periph_DAC;                                           // Enable DAC Clock
  RCC->APB1ENR |= RCC_APB1Periph_TIM6;                                          // Enable TIM6 Clock        
  RCC->AHBENR |= RCC_AHBPeriph_DMA1;                                            // Enable DMA1 Clock
}

void Configure_GPIOs()
{
  GPIOC->CRH = 0x44444422;                                                      // PC8 and PC9 as General Purpose Push Pull Output Max Speed = 2MHz
  
  GPIO_InitTypeDef GPIO_InitStructure;
  
  GPIO_InitStructure.GPIO_Pin =  GPIO_Pin_4;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AIN;
  GPIO_Init(GPIOA, &GPIO_InitStructure);
}

void Configure_TIM6()
{
  TIM_PrescalerConfig(TIM6, 24 - 1, TIM_PSCReloadMode_Update);
  TIM_SetAutoreload(TIM6, DAC_SAMPLING_TIME_US);
  TIM_SelectOutputTrigger(TIM6, TIM_TRGOSource_Update);
}

void Configure_DAC1()
{
  DAC_InitTypeDef DAC_InitStructure;
  
  DAC_InitStructure.DAC_Trigger = DAC_Trigger_T6_TRGO;
  DAC_InitStructure.DAC_WaveGeneration = DAC_WaveGeneration_None;
  DAC_InitStructure.DAC_OutputBuffer = DAC_OutputBuffer_Disable;
  
  DAC_Init(DAC_Channel_1, &DAC_InitStructure);
}

void Configure_DMA1()
{
  DMA_InitTypeDef DMA_InitStructure;
  
  DMA_DeInit(DMA1_Channel3);
  
  DMA_InitStructure.DMA_PeripheralBaseAddr = DAC_DHR8R1_Address;
  DMA_InitStructure.DMA_MemoryBaseAddr = (uint32_t)&DAC_signal;
  DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralDST;
  DMA_InitStructure.DMA_BufferSize = DAC_WAVEFORM_SIZE;
  DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
  DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
  DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;
  DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;
  DMA_InitStructure.DMA_Mode = DMA_Mode_Circular;
  DMA_InitStructure.DMA_Priority = DMA_Priority_High;
  DMA_InitStructure.DMA_M2M = DMA_M2M_Disable;
  
  DMA_Init(DMA1_Channel3, &DMA_InitStructure);
}
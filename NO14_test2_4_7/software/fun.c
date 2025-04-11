#include "headfile.h"
uint8_t key1_state,key2_state,key3_state,
key4_state,key1_last_state,key2_last_state,key3_last_state,key4_last_state;

float val,temp;
uint16_t s500;
typedef enum mode_x{
	 aut=0,noaut=1,sleep=3,
}mode_x;
mode_x mode=aut;
mode_x mode_last=aut;
uint8_t mode_kind=1,flag_pra;
uint8_t uart_get=0;
uint32_t pwm_fc[3];
uint8_t	ic_state;
void top_con(void)
{
	if(s500>=500) {if(mode!=sleep)mode_last=mode; mode=sleep; flag_pra=1;s500=0;}
}
void key_scan(void)
{
	key1_state = HAL_GPIO_ReadPin(GPIOB,GPIO_PIN_0);
	key2_state = HAL_GPIO_ReadPin(GPIOB,GPIO_PIN_1);
	key3_state = HAL_GPIO_ReadPin(GPIOB,GPIO_PIN_2);
	key4_state = HAL_GPIO_ReadPin(GPIOA,GPIO_PIN_0);
	
	if(key1_last_state==1&&key1_state==0){
		s500=0;
		if(mode==sleep){
			flag_pra=0;
			mode=mode_last;
		}
		else{
			if(mode==aut) mode=noaut;//change mode
			else mode=aut;
		}
	}
	if(key2_last_state==1&&key2_state==0){
		s500=0;
		if(mode==sleep){
			
			flag_pra=0;
			mode=mode_last ;
		}
		if(mode==noaut){
			mode_kind++;
		if(mode_kind>3) mode_kind=3;}
	}
	if(key3_last_state==1&&key3_state==0){
		s500=0;
		if(mode==sleep){	
			flag_pra=0;
			mode=mode_last ;
		}
		if(mode==noaut){
		mode_kind--;
		if(mode_kind<1) mode_kind=1;}
	}
	if(key4_last_state==1&&key4_state==0){
		
	}
	key1_last_state=key1_state;
	key2_last_state=key2_state;
	key3_last_state=key3_state;
	key4_last_state=key4_state;
}

void get_value(void)
{
	HAL_ADC_Start(&hadc2);
	uint32_t adc_num;
	adc_num=HAL_ADC_GetValue(&hadc2);
	val=(float) adc_num/4096*3.3;
	if(val<=1) temp=20;
	else if(val>=3) temp=40;
	else temp = val*10+10;
}
uint16_t s300;
void led_scan(void)
{
	
	uint32_t odr=GPIOC->ODR;
	GPIOC->ODR=0x0f<<8;
	if(mode==aut)
	{
		HAL_GPIO_WritePin(GPIOC,GPIO_PIN_8<<7,GPIO_PIN_RESET);
		HAL_GPIO_WritePin(GPIOD,GPIO_PIN_2,GPIO_PIN_SET);
		HAL_GPIO_WritePin(GPIOD,GPIO_PIN_2,GPIO_PIN_RESET);
	}	
	else
	{
		HAL_GPIO_WritePin(GPIOC,GPIO_PIN_8<<7,GPIO_PIN_SET);
		HAL_GPIO_WritePin(GPIOD,GPIO_PIN_2,GPIO_PIN_SET);
		HAL_GPIO_WritePin(GPIOD,GPIO_PIN_2,GPIO_PIN_RESET);
	}	
	if(mode!=sleep)
	switch(mode_kind){
		case 1:HAL_GPIO_WritePin(GPIOC,GPIO_PIN_8,GPIO_PIN_RESET);HAL_GPIO_WritePin(GPIOD,GPIO_PIN_2,GPIO_PIN_SET);
		HAL_GPIO_WritePin(GPIOD,GPIO_PIN_2,GPIO_PIN_RESET);break;
		case 2:HAL_GPIO_WritePin(GPIOC,GPIO_PIN_8<<1,GPIO_PIN_RESET);HAL_GPIO_WritePin(GPIOD,GPIO_PIN_2,GPIO_PIN_SET);
		HAL_GPIO_WritePin(GPIOD,GPIO_PIN_2,GPIO_PIN_RESET);break;
		case 3:HAL_GPIO_WritePin(GPIOC,GPIO_PIN_8<<2,GPIO_PIN_RESET);HAL_GPIO_WritePin(GPIOD,GPIO_PIN_2,GPIO_PIN_SET);
		HAL_GPIO_WritePin(GPIOD,GPIO_PIN_2,GPIO_PIN_RESET);break;
		
		default:HAL_GPIO_WritePin(GPIOC,GPIO_PIN_8|GPIO_PIN_9|GPIO_PIN_10,GPIO_PIN_SET);HAL_GPIO_WritePin(GPIOD,GPIO_PIN_2,GPIO_PIN_SET);
		HAL_GPIO_WritePin(GPIOD,GPIO_PIN_2,GPIO_PIN_RESET);
	}
	else{
		HAL_GPIO_WritePin(GPIOC,GPIO_PIN_8|GPIO_PIN_9|GPIO_PIN_10,GPIO_PIN_SET);HAL_GPIO_WritePin(GPIOD,GPIO_PIN_2,GPIO_PIN_SET);
		HAL_GPIO_WritePin(GPIOD,GPIO_PIN_2,GPIO_PIN_SET);
		HAL_GPIO_WritePin(GPIOD,GPIO_PIN_2,GPIO_PIN_RESET);
	}
	if(uart_get==1&&s300<=300){
		HAL_GPIO_WritePin(GPIOC,GPIO_PIN_8<<3,GPIO_PIN_RESET);
		HAL_GPIO_WritePin(GPIOD,GPIO_PIN_2,GPIO_PIN_SET);
		HAL_GPIO_WritePin(GPIOD,GPIO_PIN_2,GPIO_PIN_RESET);
		
	}
	else{
		HAL_GPIO_WritePin(GPIOC,GPIO_PIN_8<<3,GPIO_PIN_SET);
		HAL_GPIO_WritePin(GPIOD,GPIO_PIN_2,GPIO_PIN_SET);
		HAL_GPIO_WritePin(GPIOD,GPIO_PIN_2,GPIO_PIN_RESET);
		uart_get=0;
		s300=0;
	}
	HAL_GPIO_WritePin(GPIOC,GPIO_PIN_8<<4,GPIO_PIN_SET);
	HAL_GPIO_WritePin(GPIOC,GPIO_PIN_8<<5,GPIO_PIN_SET);
	HAL_GPIO_WritePin(GPIOC,GPIO_PIN_8<<6,GPIO_PIN_SET);
	HAL_GPIO_WritePin(GPIOD,GPIO_PIN_2,GPIO_PIN_SET);
	HAL_GPIO_WritePin(GPIOD,GPIO_PIN_2,GPIO_PIN_RESET);
	GPIOC->ODR=odr;

	
}
void key_uart(void)
{
	if(mode==sleep){
		switch (key_mode){
		case 0x01:{mode=mode_last;flag_pra=0;s500=0;break;}
		case 0x02:{mode=mode_last;flag_pra=0;s500=0;break;}
		case 0x03:{mode=mode_last;flag_pra=0;s500=0;break;}
		case 0x04:{mode=mode_last;flag_pra=0;s500=0;break;}
		}
		key_mode=0;
	}
	else 
	{
	switch (key_mode){
		case 0x01:{if(mode==aut) mode=noaut;//change mode
		else mode=aut;key_mode=0;}break;
		case 0x02:if(mode==noaut){mode_kind++;key_mode=0;if(mode_kind>3) mode_kind=3;}break;
		case 0x03:if(mode==noaut){mode_kind--;key_mode=0;if(mode_kind<1) mode_kind=1;}break;
		case 0x04:HAL_UART_Transmit(&huart1,(uint8_t *)"NULL",4,0X1000);key_mode=0;break;
		}
		key_mode=0;
	}
}
void data_show(void)
{
	char txt[20];
	LCD_DisplayStringLine(Line1,(uint8_t *)"        DATA        ");
	sprintf(txt,"     TEMP:%.1f      ",temp);
	LCD_DisplayStringLine(Line3,(uint8_t *)txt);	
	
	if(mode==aut){
		sprintf(txt,"     MODE:Auto        ");
		LCD_DisplayStringLine(Line4,(uint8_t *)txt);		
		if(temp<25) {mode_kind=1; sprintf(txt,"     GEAR:1        ");}
	  else if(temp>30) { mode_kind=3; sprintf(txt,"     GEAR:3        ");}
		else {sprintf(txt,"     GEAR:2        ");mode_kind=2;}
		LCD_DisplayStringLine(Line5,(uint8_t *)txt);
	}
	else{
		sprintf(txt,"     MODE:Manu      ");
		LCD_DisplayStringLine(Line4,(uint8_t *)txt);		
		switch(mode_kind){
			case 1:sprintf(txt,"     GEAR:1         ");LCD_DisplayStringLine(Line5,(uint8_t *)txt);break;
			case 2:sprintf(txt,"     GEAR:2         ");LCD_DisplayStringLine(Line5,(uint8_t *)txt);break;
			case 3:sprintf(txt,"     GEAR:3         ");LCD_DisplayStringLine(Line5,(uint8_t *)txt);break;
		}
	}	
}
void duty_change(void)
{
	switch (mode_kind){
		case 1:TIM2->CCR2=TIM2->ARR*0.1;break;
		case 2:TIM2->CCR2=TIM2->ARR*0.5;break;
		case 3:TIM2->CCR2=TIM2->ARR*0.8;break;
		default:TIM2->CCR2=0.1*TIM2->ARR;
	}
}
	
void sleep_show(void)
{
	LCD_DisplayStringLine(Line1,(uint8_t *)"                    ");
	char txt[20];
	LCD_DisplayStringLine(Line4,(uint8_t *)"     SLEEPING   ");
	sprintf(txt,"     TEMP:%.1f",temp);
	LCD_DisplayStringLine(Line5,(uint8_t *)txt);
	LCD_DisplayStringLine(Line3,(uint8_t *)"                  ");
}
volatile float TIM3CH2_Freq = 0.0;
volatile float TIM3CH2_Duty = 0.0;
volatile int capture_end_flag = 0;
 
volatile uint32_t high_val = 0;
volatile uint32_t low_val = 0;
void lcd_show(void)
{
	char txt[20];
	sprintf(txt,"s500=%d,s300=%d",s500,s300);
	LCD_DisplayStringLine(Line0,(uint8_t *)txt);
	if(flag_pra==0) data_show();
	else sleep_show();
	sprintf(txt,"f%.1f,d%.1f,d_n%.1f",TIM3CH2_Freq,TIM3CH2_Duty,(float)(TIM2->CCR2)/TIM2->ARR);
	LCD_DisplayStringLine(Line8,(uint8_t *)txt);
}

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim){
	if(htim->Instance==TIM1){
		s500++;
		if(uart_get==1)
			s300++;
	}
}

 
void HAL_TIM_IC_CaptureCallback(TIM_HandleTypeDef *htim)
{
	static uint8_t state_ic=1;
	if(htim->Instance==TIM3)
	{
		if(state_ic==1)
		{
			__HAL_TIM_SET_CAPTUREPOLARITY(&htim3,TIM_CHANNEL_1,TIM_INPUTCHANNELPOLARITY_FALLING);
			__HAL_TIM_SET_COUNTER(htim,0);
			state_ic=2;
		}
		else if(state_ic==2)
		{
			pwm_fc[0]=__HAL_TIM_GetCounter(&htim3);
			__HAL_TIM_SET_CAPTUREPOLARITY(&htim3,TIM_CHANNEL_1,TIM_INPUTCHANNELPOLARITY_RISING);
			state_ic=3;
		}
		else if(state_ic==3)
		{
			pwm_fc[1]=__HAL_TIM_GetCounter(&htim3);
			__HAL_TIM_SET_CAPTUREPOLARITY(&htim3,TIM_CHANNEL_1,TIM_INPUTCHANNELPOLARITY_RISING);
			state_ic=1;
			high_val=__HAL_TIM_GetCounter(htim);
			//计算频率
			TIM3CH2_Freq = (float)80000000 / 80 / (pwm_fc[1]);
			//计算占空比
			TIM3CH2_Duty = (float)(pwm_fc[0]+1) / (pwm_fc[1]+1);
		}								
	}
}
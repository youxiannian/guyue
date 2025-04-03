#include "headfile.h"
volatile float value_1,value_2;
float sr37_high=2.2,sr37_low=1.2,sr38_high=3,sr38_low=1.4;
uint8_t key1_state,key2_state,key3_state,key4_state;
uint8_t key1_last_state,key2_last_state,key3_last_state,key4_last_state;
uint8_t flag_pra,flag_duty,flag_num,shine_led1_flag,shine_led2_flag;
float sr37_num_right=0,sr38_num_right=0,sr37_test_num=0,sr38_test_num=0,sr37_pra=0,sr38_pra=0;
uint16_t t1,t2;//¼ÆÊýled1 ºÍled2


void led_scan(void)
{
	uint32_t odr=GPIOC->ODR;
	GPIOC->ODR=0xff00;
	if(0<t1&&t1<100)//set 1s clock let's led1 shine
	{
		HAL_GPIO_WritePin(GPIOC,GPIO_PIN_8,GPIO_PIN_RESET);
		HAL_GPIO_WritePin(GPIOD,GPIO_PIN_2,GPIO_PIN_SET);
		HAL_GPIO_WritePin(GPIOD,GPIO_PIN_2,GPIO_PIN_RESET);
	}
	if(t1>100)
	{
		HAL_GPIO_WritePin(GPIOC,GPIO_PIN_8,GPIO_PIN_SET);
		HAL_GPIO_WritePin(GPIOD,GPIO_PIN_2,GPIO_PIN_SET);
		HAL_GPIO_WritePin(GPIOD,GPIO_PIN_2,GPIO_PIN_RESET);
		shine_led1_flag=0;//close led1 flag
	}
	if(0<t2&&t2<100)//set 1s clock let's led2 shine
	{
		HAL_GPIO_WritePin(GPIOC,GPIO_PIN_9,GPIO_PIN_RESET);
		HAL_GPIO_WritePin(GPIOD,GPIO_PIN_2,GPIO_PIN_SET);
		HAL_GPIO_WritePin(GPIOD,GPIO_PIN_2,GPIO_PIN_RESET);
	}
	if(t2>100)
	{
		HAL_GPIO_WritePin(GPIOC,GPIO_PIN_9,GPIO_PIN_SET);
		HAL_GPIO_WritePin(GPIOD,GPIO_PIN_2,GPIO_PIN_SET);
		HAL_GPIO_WritePin(GPIOD,GPIO_PIN_2,GPIO_PIN_RESET);
		shine_led2_flag=0;//close led2 flag
	}
	if(flag_pra==0)
	{
		HAL_GPIO_WritePin(GPIOC,GPIO_PIN_10,GPIO_PIN_RESET);
		HAL_GPIO_WritePin(GPIOD,GPIO_PIN_2,GPIO_PIN_SET);
		HAL_GPIO_WritePin(GPIOD,GPIO_PIN_2,GPIO_PIN_RESET);
	}
	if(flag_pra==1)
	{
		HAL_GPIO_WritePin(GPIOC,GPIO_PIN_11,GPIO_PIN_RESET);
		HAL_GPIO_WritePin(GPIOD,GPIO_PIN_2,GPIO_PIN_SET);
		HAL_GPIO_WritePin(GPIOD,GPIO_PIN_2,GPIO_PIN_RESET);
	}
	if(flag_pra==2)
	{
		HAL_GPIO_WritePin(GPIOC,GPIO_PIN_12,GPIO_PIN_RESET);
		HAL_GPIO_WritePin(GPIOD,GPIO_PIN_2,GPIO_PIN_SET);
		HAL_GPIO_WritePin(GPIOD,GPIO_PIN_2,GPIO_PIN_RESET);
	}
	GPIOC->ODR =odr;
}

void get_value(void)
{
	uint16_t adc_num;
	HAL_ADC_Start(&hadc1);
	HAL_ADC_Start(&hadc2);
	
	adc_num=HAL_ADC_GetValue(&hadc1);
	value_2=(float)(3.3*adc_num/4096);
	adc_num=HAL_ADC_GetValue(&hadc2);
	value_1=(float)(3.3*adc_num/4096);
}

void key_scan(void)
{
	key1_state = HAL_GPIO_ReadPin(GPIOB,GPIO_PIN_0);
	key2_state = HAL_GPIO_ReadPin(GPIOB,GPIO_PIN_1);
	key3_state = HAL_GPIO_ReadPin(GPIOB,GPIO_PIN_2);
	key4_state = HAL_GPIO_ReadPin(GPIOA,GPIO_PIN_0);
	
	if(key1_last_state==1&&key1_state==0)
	{
		flag_pra++;
		flag_pra%=3;
	}
	if(key2_last_state==1&&key2_state==0)
	{
		if(flag_pra==0){
			if(value_1<=sr37_high&&value_1>=sr37_low){	sr37_num_right++;
				shine_led1_flag=1;
				t1=0;//set led begin 0 clock
			}
			sr37_test_num++;
			sr37_pra=(sr37_num_right/sr37_test_num)*100;
		}
		else if(flag_pra==1){
			flag_num++;
			flag_num%=4;
		}
	}	
	if(key3_last_state==1&&key3_state==0)
	{
		if(flag_pra==0){
			if(value_2<=sr38_high&&value_2>=sr38_low)	{sr38_num_right++;
				shine_led2_flag=1;
				t2=0;//set led begin 0 clock
			}
			sr38_test_num++;
			sr38_pra=(sr38_num_right/sr38_test_num)*100;
		}
		if(flag_pra==1)
		{
			switch(flag_num){
				case 1:
					sr37_low+=0.2;
					if(sr37_low>2.1)	sr37_low=1.2;
					break;
				case 0:
					sr37_high+=0.2;
					if(sr37_high>3.1)	sr37_high=2.2;
					break;
				case 3:
					sr38_low+=0.2;
					if(sr38_low>2.1)	sr38_low=1.2;
					break;
				case 2:
					sr38_high+=0.2;
					if(sr38_high>3.1)	sr38_high=2.2;
					break;
			}
		}
	}	
	if(key4_last_state==1&&key4_state==0)
	{
		if(flag_pra==2){
			sr37_pra=0;
			sr38_pra=0;
			sr37_num_right=0;sr38_num_right=0;sr37_test_num=0;sr38_test_num=0;
			LCD_Clear(Black);
		}
		if(flag_pra==1)
		{
			switch(flag_num){
				case 1:
					sr37_low-=0.2;
					if(sr37_low<1.1)	sr37_low=2.0;
					break;
				case 0:
					sr37_high-=0.2;
					if(sr37_high<2.1)	sr37_high=3.0;
					break;
				case 3:
					sr38_low-=0.2;
					if(sr38_low<1.1)	sr38_low=2.0;
					break;
				case 2:
					sr38_high-=0.2;
					if(sr38_high<2.1)	sr38_high=3.0;
					break;
			}
		}
	}		
	key1_last_state = key1_state;
	key2_last_state = key2_state;
	key3_last_state = key3_state;
	key4_last_state = key4_state;
}
void data_show(void)
{
	char txt[20];
	LCD_DisplayStringLine(Line1,(uint8_t *)"       GOODS          ");
	sprintf(txt,"     R37:%.2f        ",value_1);
	LCD_DisplayStringLine(Line3,(uint8_t *)txt);
	sprintf(txt,"     R38:%.2f       ",value_2);
	LCD_DisplayStringLine(Line4,(uint8_t *)txt);
}
void standard_show(void)
{
	char txt[20];
	LCD_DisplayStringLine(Line1,(uint8_t *)"      STANDARD       ");
	sprintf(txt,"    SR37:%.1f-%.1f      ",sr37_low,sr37_high);
	LCD_DisplayStringLine(Line3,(uint8_t *)txt);
	sprintf(txt,"    SR38:%.1f-%.1f      ",sr38_low,sr38_high);
	LCD_DisplayStringLine(Line4,(uint8_t *)txt);
}
void pass_show(void)
{
	char txt[20];
	LCD_DisplayStringLine(Line1,(uint8_t *)"        PASS       ");
	sprintf(txt,"     PR37:%.1f%%   ",sr37_pra);
	LCD_DisplayStringLine(Line3,(uint8_t *)txt);
	sprintf(txt,"     PR38:%.1f%%      ",sr38_pra);
	LCD_DisplayStringLine(Line4,(uint8_t *)txt);
}
void lcd_show(void)
{         
	HAL_GPIO_WritePin(GPIOD,GPIO_PIN_2,GPIO_PIN_RESET);	
	if(flag_pra == 0)
		data_show();
	else if(flag_pra == 1)
		standard_show();
	else 	pass_show();
}
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
	if(htim->Instance==TIM1)
	{
		if(shine_led1_flag==1){	
			t1++;
		}
		if(shine_led2_flag==1)
		{
			t2++;
		}
	}
}
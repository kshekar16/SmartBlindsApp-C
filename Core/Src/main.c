#include "main.h"
#include "ESP8266.h"
#include "string_manipulation.h"

#include <stdio.h>
#include <math.h>

void SystemClock_Config(void);
void Button_UpDown_Init(void);
void Button_OpenClose_Init(void);
void Motor_Init(void);
void delay(float);
void ADC_Init(void);
int ADC_Read(int);
static void MX_USART1_UART_Init(void);
float Get_Temperature(int);
int Convert_Fahrenheit(float);
int Get_Battery_Percentage(int);
int Get_Luminosity(int);
void CheckBrightness(int, int);
void CheckTemperature(int, int);
void AddTime(int, int, int, float);
void formatTime(char *);
void formatOpenTime(char *);
void formatCloseTime(char *);
void CheckTime(void);

void OpenBlinds(void);
void CloseBlinds(void);
int getBrightFlag(int, int);
int getTempFlag(int, int);
int getTimeFlag(void);

UART_HandleTypeDef huart1;
enum states current_state;
enum states previous_state;
uint8_t message[50];
int message_index;
int receive_flag;
int message_len;
int staip_size;
char wifi_name[50] = "";
char wifi_password[50] = "";
char stap_ip[20] = "";
int configured_flag;
int close_temp = 0;
int open_temp = 0;
int bright_flag;
int dark_flag;
int curr_pos;
char time_open[10] = "";
char time_close[10] = "";
char manual_control[15] = "";
int manual_control_flag = 0;
char current_time[10] = "";
float milli = 0;
float temp = 0;
int currentHours = 0;
int currentMinutes = 0;
int currentSeconds = 0;
int closeHour = 0;
int closeMin = 0;
int openHour = 0;
int openMin = 0;
int timeFlag = 0;
char Hour[3] = "";
char Minute[3] = "";
char Hc[3] = "";
char Mc[3] = "";
char Ho[3] = "";
char Mo[3] = "";

int brightFlag;
int timeFlag;
int tempFlag;


void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart){
	if (current_state == MANUAL){
		manual_control_flag = 1;
	}
	else{
		receive_flag = 1;
		message_index++;
		message_len++;
	}
}

void AddTime(int currentHours_i, int currentMinutes_i, int currentSeconds_i, float elapsedTime)
{
	currentSeconds_i += elapsedTime;
	if(currentSeconds_i >= 60)
	{
		currentMinutes_i += 1;
		currentSeconds_i = currentSeconds_i % 60;
		if(currentMinutes_i >= 60)
		{
			currentHours_i += currentMinutes_i % 60;
			currentMinutes_i = currentMinutes_i % 60;
			if(currentHours_i >= 24)
			{
				currentHours_i += currentHours_i % 24;
			}
		}
	}
	currentHours = currentHours_i;
	currentMinutes = currentMinutes_i;
	currentSeconds = currentSeconds_i;
	return;
}

int main(void)
{
	current_state = START;
	message_index = 0;
	receive_flag = 0;
	message_len = 0;
	configured_flag = 0;
	staip_size = 0;
	curr_pos = 0;


  int ADC_Lumen, ADC_Temperature, ADC_Battery, Temperature_F, Battery_Percentage,
  Bright_Outside = 0;
  float Temperature_C;

  HAL_Init();
  SystemClock_Config();
  MX_USART1_UART_Init();

	esp8266_reset(&huart1);
	if(esp8266_check_wifi_connection(&huart1)){
		  esp8266_setupTCP(&huart1);
		  milli = HAL_GetTick();
		  current_state = IDLE;
		  configured_flag = 1;
	}
	else
	{
		  esp8266_reset(&huart1);
	}


  Button_UpDown_Init();
  Button_OpenClose_Init();
  Motor_Init();
  ADC_Init();

  TIM2->CCR3 = 50;

  while (1)
  {
	  TIM2->CCR1 = 0;
	  TIM2->CCR2 = 0;
	  ADC_Lumen = ADC_Read(4);	//ADC_IN4 = PA4
	  ADC_Temperature = ADC_Read(10);	//ADC_IN10 = PC0
	  ADC_Battery = ADC_Read(11);	//ADC_IN11 = PC1

	  //Initiate system
	  	  while (current_state == START){
	  		  if (esp8266_init(&huart1)){
	  			  if (esp8266_setupTCP(&huart1)){
	  				  //esp8266_check_wifi_connection(&huart1);
	  				  current_state = IDLE;
	  		  		  previous_state = START;
	  				  break;
	  			  }
	  		  }
	  		  current_state = ERROR_STATE;
	  	  }

	  	  //State where the system does nothing
	  	  while(current_state == IDLE){
	  		  HAL_UART_Receive_IT(&huart1, &message[message_index], 1);
	  		  if (receive_flag == 1){
	  			  current_state = RECEIVE;
	  			  previous_state = IDLE;
	  			  receive_flag = 0;
	  			  break;
	  		  }
	  		  temp = HAL_GetTick();
	  		  if((temp - milli) > 60000)
	  		  {
	  			  float time_elapsed = (temp - milli) / 1000;
	  			  AddTime(currentHours, currentMinutes, currentSeconds, time_elapsed);
	  			  milli = HAL_GetTick();
		  		  if(current_state != RECEIVE && previous_state == IDLE)
		  		  {
		  			  CheckBrightness(dark_flag, bright_flag);
		  			  CheckTemperature(close_temp, open_temp);
		  			  CheckTime();
		  		  }
	  		  }
	  		 /*if(current_state != RECEIVE && previous_state == IDLE)
	  		 {
				  brightFlag = getBrightFlag(dark_flag, bright_flag);
				  tempFlag = getTempFlag(close_temp, open_temp);
				  timeFlag = getTimeFlag();
				  if(~brightFlag && tempFlag)
				  {
					  CloseBlinds();
				  }
				  else if(~brightFlag && timeFlag && timeFlag != 2 && brightFlag != 2)
				  {
					  CloseBlinds();
				  }
				  else if(~tempFlag && timeFlag && timeFlag != 2 && tempFlag != 2)
				  {
					  CloseBlinds();
				  }
				  else if(brightFlag && ~tempFlag && tempFlag != 2 && brightFlag != 2)
				  {
					  OpenBlinds();
				  }
				  else if(brightFlag && ~timeFlag && timeFlag != 2 && brightFlag != 2)
				  {
					  OpenBlinds();
				  }
				  else if(tempFlag && ~timeFlag && timeFlag != 2 && tempFlag != 2)
				  {
					  OpenBlinds();
				  }
				  else if((tempFlag || brightFlag || timeFlag) && timeFlag != 2 && tempFlag != 2 && brightFlag != 2)
				  {
					  CloseBlinds();
				  }
				  else
				  {
					  OpenBlinds();
				  }
			  }*/

	  		  previous_state = IDLE;
	  	      //CheckTemperature(close_temp, open_temp);
	  	  }

	  	  //State where the system connects to WiFi
	  	  while(current_state == CONNECT_WIFI){
	  		  HAL_UART_AbortReceive_IT(&huart1);
	  		  if (esp8266_sendmsg(&huart1, "OK\r\n", 4)){
	  			  if (esp8266_connectWifi(&huart1, wifi_name, wifi_password)){
	  				  if (esp8266_setupTCP(&huart1)){
	  					  current_state = IDLE;
	  					  milli = HAL_GetTick();
	  			  		  previous_state = CONNECT_WIFI;
	  					  break;
	  				  }
	  			  }
	  		  }

	  		  current_state = ERROR_STATE;
	  		  previous_state = CONNECT_WIFI;
	  	  }

	  	  while (current_state == GET_CUR_TEMP){
	  		  char message[15] = "TEMP";

	  		  HAL_UART_AbortReceive_IT(&huart1);

	  		  Temperature_C = Get_Temperature(ADC_Temperature);
	  		  Temperature_F = Convert_Fahrenheit(Temperature_C);

	  		  //Send data
	  		  esp8266_send_current_data(&huart1, (char *) message, Temperature_F, 4);
	  		  current_state = IDLE;
	  		  previous_state = GET_CUR_TEMP;
	  	  }

	  	  while (current_state == GET_CUR_POS){
	  		  char message[15] = "POS";

	  		  HAL_UART_AbortReceive_IT(&huart1);

	  		  //TODO: Do necessary stuff to get blind position below
	  		  curr_pos = TIM2->CCR3; //EXAMPLE

	  		  //Send data
	  		  esp8266_send_current_data(&huart1, (char *) message, curr_pos, 3);
	  		  current_state = IDLE;
	  		  previous_state = GET_CUR_POS;
	  	  }

	  	  while (current_state == GET_CUR_BAT){
	  		  char message[15] = "BAT";

	  		  HAL_UART_AbortReceive_IT(&huart1);

	  		  //TODO: Do necessary stuff to get battery data below
	  		  Battery_Percentage = Get_Battery_Percentage(ADC_Battery);

	  		  //Send data
	  		  esp8266_send_current_data(&huart1, (char *) message, Battery_Percentage, 3);
	  		  current_state = IDLE;
	  		  previous_state = GET_CUR_BAT;
	  	  }

	  	  while (current_state == CUR_STATE){

	  		  HAL_UART_AbortReceive_IT(&huart1);

	  		  current_state = IDLE;
	  		  previous_state = CUR_STATE;
	  	  }

	  	  while (current_state == MANUAL){
	  		  //Android sends message to ESP for interrupt
	  		  HAL_UART_Receive_IT(&huart1, (uint8_t*) manual_control, 15);

	  		  if (manual_control_flag){
	  			  if (strstr(manual_control, (char*) "SP") != NULL){
	  				  //TODO: Do necessary stuff to raise stop any manual action below
		  			  TIM2->CCR2 = 0;
		  			  TIM2->CCR1 = 0;
	  			  }
	  			  if (strstr(manual_control, (char*) "QT") != NULL){
	  				  //TODO: Do necessary stuff to quit out of manual control below
	  				  if (esp8266_sendmsg(&huart1, "K\n", 2))
	  				  {
	  					  manual_control_flag = 0;
	  					  current_state = IDLE;
	  					  previous_state = MANUAL;
	  					  break;
	  				  }
	  			  }
	  			  if (strstr(manual_control, (char*) "UP") != NULL){
	  				  //TODO: Do necessary stuff to raise the blinds up below
	  				  TIM2->CCR1 = 100;
	  			  }
	  			  if (strstr(manual_control, (char*) "DN") != NULL){
	  				  //TODO: Do necessary stuff to lower the blinds down below
	  				  TIM2->CCR2 = 100;
	  			  }
	  			  if (strstr(manual_control, (char*) "PU") != NULL){
	  				  //TODO: Do necessary stuff to pitch up the blinds up below (controlling the slats)
	  				  //open
	  				  delay(3);
	  				  if(TIM2->CCR3 < 100)
	  				  {
	  					  TIM2->CCR3 += 10;
	  				  }
	  			  }
	  			  if (strstr(manual_control, (char*) "CL") != NULL){
	  				  //TODO: Do necessary stuff to pitch down the blinds down below (controlling the slats)
	  				  //close
	  				  delay(3);
	  				  if(TIM2->CCR3 > 0)
	  				  {
	  					  TIM2->CCR3 -= 10;
	  				  }
	  			  }
	  			  HAL_UART_AbortReceive_IT(&huart1);
	  			  manual_control_flag = 0;

	  		  }

	  		  current_state = MANUAL;
	  		  previous_state = MANUAL;
	  	  }

	  	  while (current_state == TEMP_CONFIG){
	  		  HAL_UART_AbortReceive_IT(&huart1);

	  		  //TODO: Do any necessary stuff to initiate temperature configuration stuff

	  		  current_state = IDLE;
	  		  previous_state = TEMP_CONFIG;
	  	  }

	  	  while (current_state == TEMP_CLOSE_CONFIG){
	  		  HAL_UART_AbortReceive_IT(&huart1);

	  		  //TODO: Do any necessary stuff to configure closing temperature stuff

	  		  //Acknowledge that you are done configuring closing temperature stuff
	  		  esp8266_sendmsg(&huart1, "CLOSE_OK\n", 9);

	  		  current_state = IDLE;
	  		  previous_state = TEMP_CLOSE_CONFIG;
	  	  }

	  	  while (current_state == TEMP_OPEN_CONFIG){
	  		  HAL_UART_AbortReceive_IT(&huart1);

	  		  //TODO: Do any necessary stuff to configure opening temperature stuff

	  		  //Acknowledge that you are done configuring closing temperature stuff
	  		  esp8266_sendmsg(&huart1, "OPEN_OK\n", 8);

	  		  current_state = IDLE;
	  		  previous_state = TEMP_OPEN_CONFIG;
	  	  }

	  	  while (current_state == LIGHT_CONFIG){
	  		  HAL_UART_AbortReceive_IT(&huart1);

	  		  //TODO: Do necessary stuff to get Light information bellow
	  		  //Returns 1 when bright outside
	  		  //Returns 0 when not
	  		  Bright_Outside = Get_Luminosity(ADC_Lumen);
	  		  current_state = IDLE;
	  		  previous_state = LIGHT_CONFIG;
	  	  }

	  	  while (current_state == BRIGHT_CONFIG){
	  		  HAL_UART_AbortReceive_IT(&huart1);

	  		  //TODO: Do necessary stuff to configure when bright bellow
	  		  //Bright open and bright outside, open

	  		  //Acknowledge that you are done configuring closing temperature stuff
	  		  esp8266_sendmsg(&huart1, "BRI_OK\n", 7);

	  		  current_state = IDLE;
	  		  previous_state = BRIGHT_CONFIG;
	  	  }

	  	  while (current_state == DARK_CONFIG){
	  		  HAL_UART_AbortReceive_IT(&huart1);

	  		  //TODO: Do necessary stuff to configure when dark bellow
	  		  //Dark open and dark outside


	  		  //Acknowledge that you are done configuring closing temperature stuff
	  		  esp8266_sendmsg(&huart1, "DAR_OK\n", 7);
	  		  current_state = IDLE;
	  		  previous_state = DARK_CONFIG;
	  	  }

	  	  while (current_state == TIME_CONFIG){
	  		  HAL_UART_AbortReceive_IT(&huart1);

	  		  //TODO: Do necessary stuff to initiate time configuration

	  		  current_state = IDLE;
	  		  previous_state = TIME_CONFIG;
	  	  }

	  	  while (current_state == TIME_OPEN_CONFIG){
	  		  HAL_UART_AbortReceive_IT(&huart1);

	  		  //TODO: Do necessary stuff to configure opening time below

	  		  //Acknowledge that you are done configuring closing time stuff
	  		  esp8266_sendmsg(&huart1, "OP_OK\n", 6);
	  		  current_state =IDLE;
	  		  previous_state = TIME_OPEN_CONFIG;

	  	  }

	  	  while (current_state == TIME_CLOSE_CONFIG){
	  		  HAL_UART_AbortReceive_IT(&huart1);

	  		  //TODO: Do necessary stuff to configure closing time below

	  		  //Acknowledge that you are done configuring closing time stuff
	  		  esp8266_sendmsg(&huart1, "CL_OK\n", 6);
	  		  current_state =IDLE;
	  		  previous_state = TIME_CLOSE_CONFIG;

	  	  }

	  	  //State where the system is receiving message
	  	  while (current_state == RECEIVE){
	  		  HAL_UART_AbortReceive_IT(&huart1);
	  		  if (HAL_UART_Receive(&huart1, &message[message_index], 1, 100) == HAL_OK){
	  			  message_len++;
	  			  message_index++;
	  		  }
	  		  if (message[message_index-1] == '\n'){
	  			  current_state = IDLE;
	  			  manipulate_string((char *) message, message_len);

	  			  if (configured_flag == 0 && strstr((char*) message, (char *) "CONNECT") != NULL){
	  				  current_state = IDLE;
	  				  previous_state = RECEIVE;
	  			  }
	  			  else if(wifi_credential_search((char*) message, wifi_name, wifi_password, message_len)){
	  				  current_state = CONNECT_WIFI;
	  				  previous_state = RECEIVE;
	  			  }
	  			  else if (strstr((char *) message, (char *) "RQ_TEMP")!= NULL){
	  				  current_state = GET_CUR_TEMP;
	  				  previous_state = RECEIVE;
	  			  }

	  			  else if (strstr((char *) message, (char *) "RQ_POS")!= NULL){
	  				  current_state = GET_CUR_POS;
	  				  previous_state = RECEIVE;
	  			  }

	  			  else if (strstr((char *) message, (char *) "RQ_BAT")!= NULL){
	  				  current_state = GET_CUR_BAT;
	  				  previous_state = RECEIVE;
	  			  }
	  			  else if (strstr((char*) message, (char *) "TEMP_CONFIG") != NULL){
	  				  //First acknowledge the message
	  				  if (esp8266_sendmsg(&huart1, "K\n", 2)){
	  					  current_state = TEMP_CONFIG;
		  				  previous_state = RECEIVE;
	  				  }
	  			  }
	  			  else if (strstr((char *) message, (char *) "TEMP_CLOSE") != NULL){
	  				  current_state = TEMP_CLOSE_CONFIG;
	  				  previous_state = RECEIVE;

	  				  //Get integer value of closing temperature from message
	  				  close_temp = get_temperature((char *) message, message_len);
	  			  }
	  			  else if (strstr((char *) message, (char *) "TEMP_OPEN") != NULL){
	  				  current_state = TEMP_OPEN_CONFIG;
	  				  previous_state = RECEIVE;

	  				  //Get integer value of opening temperature from message
	  				  open_temp = get_temperature((char *) message, message_len);

	  			  }
	  			  else if (strstr((char *) message, (char *) "LIGHT_CONFIG") != NULL){
	  				  if (esp8266_sendmsg(&huart1, "K\n", 2)){
	  					  current_state = LIGHT_CONFIG;
		  				  previous_state = RECEIVE;
	  				  }
	  			  }
	  			  else if (strstr((char *) message, (char *) "BRIGHT") != NULL){
	  				  current_state = BRIGHT_CONFIG;
	  				  previous_state = RECEIVE;
	  				  //Check condition for when it is bright
	  				  //1 - OPEN
	  				  //2 - CLOSE
	  				  bright_flag = 1;
	  				  if (strstr((char*) message, (char *) "OPEN")){
	  					  bright_flag = 2;
	  				  }
	  			  }
	  			  else if (strstr((char *) message, (char *) "DARK") != NULL){
	  				  current_state = DARK_CONFIG;
	  				  previous_state = RECEIVE;
	  				  //Check condition for when it is dark
	  				  //1 - OPEN
	  				  //2 - CLOSE
	  				  dark_flag = 1;
	  				  if (strstr((char*) message, (char *) "OPEN")){
	  					  dark_flag = 2;
	  				  }
	  			  }
	  			  else if (strstr((char *) message, (char *) "TIME_CONFIG") != NULL){
	  				  if (esp8266_sendmsg(&huart1, "K\n", 2)){
	  					  current_state = TIME_CONFIG;
		  				  previous_state = RECEIVE;
	  				  }
	  			  }
	  			  else if (strstr((char *) message, (char *) "TM_OP") != NULL){
	  				  //Extract Opening time from message
	  				  //open_text format: <hour>:<minute><AM or PM>
	  				  extract_time ((char*)message, message_len, time_open);
	  				  formatOpenTime(time_open);
	  				  current_state = TIME_OPEN_CONFIG;
	  				  previous_state = RECEIVE;
	  			  }
	  			  else if (strstr((char *) message, (char *) "TM_CL") != NULL){
	  				  //Extract Opening time from message
	  				  //open_text format: <hour>:<minute><AM or PM>
	  				  extract_time ((char*) message, message_len, time_close);
	  				  formatCloseTime(time_close);
	  				  current_state = TIME_CLOSE_CONFIG;
	  				  previous_state = RECEIVE;
	  			  }

	  			  else if (strstr((char*) message, (char *) "MANUAL") != NULL){
	  				  if (esp8266_sendmsg(&huart1, "K\n", 2)){
	  					  current_state = MANUAL;
		  				  previous_state = RECEIVE;
	  					  HAL_UART_AbortReceive_IT(&huart1);
	  				  }
	  			  }
	  			  else if (strstr((char*) message, (char *) "CUR_TIME") != NULL){
	  				  //<hour>:<minute><AM or PM>
	  				  extract_time ((char*) message, message_len, current_time);
	  				  formatTime(current_time);
					  current_state = CUR_STATE;
	  				  previous_state = RECEIVE;
	  			  }
	  			  HAL_UART_AbortReceive(&huart1);
	  			  memset(message, 0, message_len);
	  			  message_index = 0;
	  			  message_len = 0;
	  		  }
	  	  }
  /* USER CODE END 3 */
  }

}

void OpenBlinds(void)
{
	  while(TIM2->CCR3 < 100)
	  {
		  delay(3);
		  TIM2->CCR3 += 10;
	  }
}

void CloseBlinds(void)
{
	  while(TIM2->CCR3 > 0)
	  {
		  delay(3);
		  TIM2->CCR3 -= 10;
	  }
}

int getTimeFlag()
{
	if(closeHour <= currentHours && closeMin != 0)
	{
		if(closeMin <= currentMinutes && closeMin != 0)
		{
			return 1;
		 }
	}
	else if(openHour <= currentHours && ~openHour)
	{
		if(openMin <= currentMinutes && ~openHour)
		{
			return 0;
		}
	}
	else
	{
		return 2;
	}
}

void CheckTime()
{
	if(closeHour <= currentHours)
	{
		if(closeMin <= currentMinutes)
		{
			  while(TIM2->CCR3 > 0)
			  {
				  delay(3);
				  TIM2->CCR3 -= 10;
			  }
		 }
	}
	else if(openHour <= currentHours)
	{
		if(openMin <= currentMinutes)
		{
			  while(TIM2->CCR3 < 100)
			  {
				  delay(3);
				  TIM2->CCR3 += 10;
			  }
		}
	}
}

void formatCloseTime(char * close_time)
{
	Hc[0] = time_close[0];
	Hc[1] = time_close[1];
	Mc[0] = time_close[3];
	Mc[1] = time_close[4];

	if(time_close[5] == 'P')
	{
		closeHour = atoi(Hc);
		closeHour += 12;
		closeMin = atoi(Mc);
	}
	else
	{
		closeHour = atoi(Hc);
		closeMin = atoi(Mc);
	}
}

void formatOpenTime(char * open_time)
{
	Ho[0] = time_open[0];
	Ho[1] = time_open[1];
	Mo[0] = time_open[3];
	Mo[1] = time_open[4];

	if(time_open[5] == 'P')
	{
		openHour = atoi(Ho);
		openHour += 12;
		openMin = atoi(Mo);
	}
	else
	{
		openHour = atoi(Ho);
		openMin = atoi(Mo);
	}
}
void formatTime(char * current_time)
{
	Hour[0] = current_time[0];
	Hour[1] = current_time[1];
	Minute[0] = current_time[3];
	Minute[1] = current_time[4];

	currentHours = atoi(Hour);
	currentMinutes = atoi(Minute);
	return;
}

int getTempFlag(int close_temp, int open_temp)
{
	  int ADC_Temperature = ADC_Read(10);	//ADC_IN10 = PC0
	  float Temperature_C = Get_Temperature(ADC_Temperature);
	  int Temperature_F = Convert_Fahrenheit(Temperature_C);
	  if(Temperature_F < close_temp)
	  {
		  return 1;
	  }
	  else if(Temperature_F > open_temp)
	  {
		  return 0;
	  }
	  else
	  {
		  return 2;
	  }
}

void CheckTemperature(int close_temp, int open_temp)
{
	  int ADC_Temperature = ADC_Read(10);	//ADC_IN10 = PC0
	  float Temperature_C = Get_Temperature(ADC_Temperature);
	  int Temperature_F = Convert_Fahrenheit(Temperature_C);
	  if(Temperature_F < close_temp)
	  {
		  while(TIM2->CCR3 > 0)
		  {
			  delay(3);
			  TIM2->CCR3 -= 10;
		  }
	  }
	  else if(Temperature_F > open_temp)
	  {
		  while(TIM2->CCR3 < 100)
		  {
			  delay(3);
			  TIM2->CCR3 += 10;
		  }
	  }
	  else
	  {
		  //Do nothing.
	  }
}
int getBrightFlag(int dark_flag, int bright_flag)
{
	  int ADC_Lumen = ADC_Read(4);	//ADC_IN4 = PA4
	  int Bright_Outside = Get_Luminosity(ADC_Lumen);
	  //above closing, close 2
	  //below opening, open 1
	  if(bright_flag == 2 && Bright_Outside)
	  {
		  return 0;
	  }
	  //Bright close and bright outside, open
	  else if(bright_flag == 1 && Bright_Outside)
	  {
		  return 1;
	  }
	  if (dark_flag == 2 && ~Bright_Outside)
	  {
		  return 0;
	  }
	  else if(dark_flag == 1 && ~Bright_Outside)
	  {
		  return 1;
	  }
	  else
	  {
		  return 2;
	  }
}

void CheckBrightness(int dark_flag, int bright_flag)
{
	  int ADC_Lumen = ADC_Read(4);	//ADC_IN4 = PA4
	  int Bright_Outside = Get_Luminosity(ADC_Lumen);

	  if(bright_flag == 1 && Bright_Outside)
	  {
		  while(TIM2->CCR3 < 100)
		  {
			  delay(3);
			  TIM2->CCR3 += 10;
		  }
	  }
	  //Bright close and bright outside, open
	  else if(bright_flag == 2 && Bright_Outside)
	  {
		  while(TIM2->CCR3 > 0)
		  {
			  delay(3);
			  TIM2->CCR3 -= 10;
		  }
	  }
	  else if (dark_flag == 1 && ~Bright_Outside)
	  {
		  while(TIM2->CCR3 < 100)
		  {
			  delay(3);
			  TIM2->CCR3 += 10;
		  }
	  }
	  else if(dark_flag == 2 && ~Bright_Outside)
	  {
		  while(TIM2->CCR3 > 0)
		  {
			  delay(3);
			  TIM2->CCR3 -= 10;
		  }
	  }
	  else
	  {
		  //Do nothing.
	  }
}
int Get_Luminosity(int ADC_Lumen)
{
	//Returns 1 if light
	//Returns 0 if dark
	float Resistor = 10000;
	float Vcc = 5;
	float Photocell;
	float inputVoltage = ADC_Lumen;
	float Vo = (inputVoltage / 4096) * Vcc;
	Photocell = ((5 * Resistor) / Vo) - Resistor;

	if(Vo < 2.5)
	{
		return 0;
	}
	else
	{
		return 1;
	}
}

int Get_Battery_Percentage(int ADC_Battery)
{
	//float TrueVoltage = ADC_Battery * 2;
	float TrueVoltage = ((float) ADC_Battery / 4096) * 3;
	float Percentage = ((TrueVoltage) / 1.56) * 100;
	if (Percentage > 100)
	{
		return 100;
	}
	else
	{
		return Percentage;
	}
}

int Convert_Fahrenheit(float Temperature_C)
{
	float Temperature_F = round(Temperature_C * 1.8 + 32);
	return (int) Temperature_F;
}

float Get_Temperature(int ADC_Temperature)
{
	float Resistor = 22000; //10 kOhm resistors used
	float Vcc = 3;	//Vcc = 3V
	float Photocell;
	float inputVoltage = ADC_Temperature;
	float Vo = (inputVoltage/4096) * Vcc;
	Photocell = ((3 * Resistor) / Vo) - Resistor;
	float Resistance_Rx = Photocell - 20000;	//Find change in resistance from 25 degrees celcius
	//Temperature coefficient = -4.4% (20,000 * 0.044 = 880)/1 degree Celcius
	float Temperature_Rx = Resistance_Rx / (880);
	if(Temperature_Rx < 0)
	{
		return 25 + (-1 * Temperature_Rx);
	}
	else
	{
		return 25 - Temperature_Rx;
	}
}


static void MX_USART1_UART_Init(void)
{

  /* USER CODE BEGIN USART1_Init 0 */

  /* USER CODE END USART1_Init 0 */

  /* USER CODE BEGIN USART1_Init 1 */

  /* USER CODE END USART1_Init 1 */
  huart1.Instance = USART1;
  huart1.Init.BaudRate = 115200;
  huart1.Init.WordLength = UART_WORDLENGTH_8B;
  huart1.Init.StopBits = UART_STOPBITS_1;
  huart1.Init.Parity = UART_PARITY_NONE;
  huart1.Init.Mode = UART_MODE_TX_RX;
  huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart1.Init.OverSampling = UART_OVERSAMPLING_16;
  huart1.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  huart1.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
  if (HAL_UART_Init(&huart1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART1_Init 2 */

  /* USER CODE END USART1_Init 2 */

}

void ADC_Init(void)
{
	RCC->AHBENR |= RCC_AHBENR_GPIOCEN;
	GPIOA->MODER &= ~(3 << (2 * 4));

	GPIOA->MODER |= (3 << (2 * 4));			//Set PA4 to analog
	GPIOC->MODER |= (3 << (2 * 0));			//Set PC0 to analog
	GPIOC->MODER |= (3 << (2 * 1));			//Set PC1 to analog

	RCC->APB2ENR |= RCC_APB2ENR_ADC1EN;		//Enable ADC in RCC

	RCC->CR2 |= RCC_CR2_HSI14ON;

	while(!(RCC->CR2 & RCC_CR2_HSI14RDY));

	ADC1->CR |= ADC_CR_ADEN;

	while(!(ADC1->ISR & ADC_ISR_ADRDY));
	while((ADC1->CR & ADC_CR_ADSTART));
}

int ADC_Read(int channel)
{
	ADC1->CHSELR = 0;
	ADC1->CHSELR |= 1 << channel;
	while(!(ADC1->ISR & ADC_ISR_ADRDY));
	ADC1->CR |= ADC_CR_ADSTART;
	while(!(ADC1->ISR & ADC_ISR_EOC));
	int x = ADC1->DR;
	return x;
}

void Motor_Init(void)
{
	/* Previously used:
	   	   TIM2->CH1 for motor up (PA5)
		   TIM2->CH2 for motor down (PA1)
	       TIM2->CH3 for motor open/close (PA2)
	       TIM2->CH4 for LED (PA3)
	 */

	/* Currently used:
	 	 PA0: Motor Up (TIM2->CH1)
	 	 PA1: Motor Down (TIM2->CH2)
	 	 PA2: Motor Open (TIM3->CH3)
	 	 PA3: Motor Close (TIM3->CH4l8
	 */
	RCC->AHBENR |= RCC_AHBENR_GPIOAEN;


	GPIOA->MODER &= ~(3 << (2 * 0));	//Clear Moder 0
	GPIOA->MODER &= ~(3 << (2 * 1));	//Clear Moder 1
	GPIOA->MODER &= ~(3 << (2 * 2));	//Clear Moder 2
	GPIOA->MODER &= ~(3 << (2 * 3));	//Clear Moder 3

	GPIOA->MODER |= (2 << (2 * 0));		//Set PA5 to Alternate Function Mode
	GPIOA->MODER |= (2 << (2 * 1));		//Set PA1 Alternative Function Mode
	GPIOA->MODER |= (2 << (2 * 2));		//Set PA2 to Alternative Function Mode
	GPIOA->MODER |= (2 << (2 * 3));		//Set PA3 to Alternative Function Mode

	GPIOA->AFR[0] &= ~(0xF << (4 * 0));		//Clear AF5
	GPIOA->AFR[0] &= ~(0xF << (4 * 1));		//Clear AF3
	GPIOA->AFR[0] &= ~(0xF << (4 * 2));		//Clear AF2
	GPIOA->AFR[0] &= ~(0xF << (4 * 3));		//Clear AF1

	GPIOA->AFR[0] |= (2 << (4 * 0));	//Set to PA5 to AF1
	GPIOA->AFR[0] |= (2 << (4 * 1));	//Set PA1 to AF1
	GPIOA->AFR[0] |= (2 << (4 * 2));	//Set PA2 to AF1
	GPIOA->AFR[0] |= (2 << (4 * 3));	//Set PA3 to AF1

	RCC->APB1ENR |= RCC_APB1ENR_TIM2EN;		//Enable Timer2 in RCC
	TIM2->PSC = 480 - 1;
	TIM2->ARR = 100 - 1;

	TIM2->CCMR1 |= TIM_CCMR1_OC1PE | TIM_CCMR1_OC1M_2 | TIM_CCMR1_OC1M_1;	//TIM2->CH1
	TIM2->CCMR1 |= TIM_CCMR1_OC2M_2 | TIM_CCMR1_OC2M_1 | TIM_CCMR1_OC2PE;	//TIM2->CH2
	TIM2->CCMR2 |= TIM_CCMR2_OC3M_2 | TIM_CCMR2_OC3M_1 | TIM_CCMR2_OC3PE;	//TIM2->CH3
	TIM2->CCMR2 |= TIM_CCMR2_OC4M_2 | TIM_CCMR2_OC4M_1 | TIM_CCMR2_OC4PE;	//TIM2->CH4


	TIM2->CCER |= TIM_CCER_CC1E;	//Enable output in capture/control register 1
	TIM2->CCER |= TIM_CCER_CC2E;	//Enable output in capture/control register 2
	TIM2->CCER |= TIM_CCER_CC3E;
	TIM2->CCER |= TIM_CCER_CC4E;

	TIM2->CR1 |= TIM_CR1_CEN;		//Enable timer counter

	GPIO_InitTypeDef GPIO_InitStruct;
	GPIO_InitStruct.Pin = GPIO_PIN_2;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Pull = GPIO_PULLUP;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_MEDIUM;
	HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_2, GPIO_PIN_SET);

	return;
}

void Button_OpenClose_Init(void)
{
	/*Previously used:
			PC6 to manually operate blinds up	(TIM3->CH1)
			PC7 to manually operate blinds down (TIM3->CH2)
			PC8 to manually operate blinds open (TIM3->CH3)
			PC9 to manually operate blinds close (TIM3->CH4)
		*/

		/*Currently used:
		 	 PA6 to manually operate blinds up (TIM3->CH1)
		 	 PA7 to manually operate blinds down (TIM3->CH2)
		 	 PB0 to manually operate blinds open (TIM3->CH3)
		 	 PB1 to manually operate blinds close (TIM3->CH4)


		 */

		//Timer
		RCC->AHBENR |= RCC_AHBENR_GPIOBEN;

		GPIOB->MODER &= ~(3 << (2 * 0));	//Clear PB0
		GPIOB->MODER &= ~(3 << (2 * 1));	//Clear PB1

		GPIOB->MODER |= (2 << (2 * 0));		//Set PB0 to AF
		GPIOB->MODER |= (2 << (2 * 1));		//Set PB1 to AF

		GPIOB->AFR[0] &= ~(0xF << (4 * 0));	//Clear AFR[0]
		GPIOB->AFR[0] &= ~(0xF << (4 * 1));	//Clear AFR[1]

		GPIOB->AFR[0] |= (1 << (4 * 0));	//Set AFR[6] to AF1
		GPIOB->AFR[0] |= (1 << (4 * 1));	//Set AFR[7] to AF1

		GPIOB->PUPDR &= ~(3 << (2 * 0));	//Clear PUPDR 6
		GPIOB->PUPDR &= ~(3 << (2 * 1));	//Clear PUPDR 7

		GPIOB->PUPDR |= (2 << (2 * 0));		//Set Pull-Down
		GPIOB->PUPDR |= (2 << (2 * 1));		//Set Pull-Down

		RCC->APB1ENR |= RCC_APB1ENR_TIM3EN;	//Enable Timer3 in RCC

		TIM3->PSC = 1 - 1;
		TIM3->ARR = 0xFFFFFFFF;

		TIM3->CCMR2 &= ~TIM_CCMR2_CC3S;
		TIM3->CCMR2 &= ~TIM_CCMR2_CC4S;

		TIM3->CCMR2 |= TIM_CCMR2_CC3S_0;
		TIM3->CCMR2 |= TIM_CCMR2_CC4S_0;

		TIM3->CCER &= ~(TIM_CCER_CC3P | TIM_CCER_CC3NP);
		TIM3->CCER &= ~(TIM_CCER_CC4P | TIM_CCER_CC4NP);

		TIM3->CCER |= TIM_CCER_CC3E;
		TIM3->CCER |= TIM_CCER_CC4E;

		TIM3->DIER |= TIM_DIER_CC3IE;
		TIM3->DIER |= TIM_DIER_CC4IE;

		TIM3->CCMR2 |= TIM_CCMR2_IC3F_3 | TIM_CCMR2_IC3F_2 | TIM_CCMR2_IC3F_1 | TIM_CCMR2_IC3F_0;
		TIM3->CCMR2 |= TIM_CCMR2_IC4F_3 | TIM_CCMR2_IC4F_2 | TIM_CCMR2_IC4F_1 | TIM_CCMR2_IC4F_0;

}

void Button_UpDown_Init(void)
{
	/*Previously used:
		PC6 to manually operate blinds up	(TIM3->CH1)
		PC7 to manually operate blinds down (TIM3->CH2)
		PC8 to manually operate blinds open (TIM3->CH3)
		PC9 to manually operate blinds close (TIM3->CH4)
	*/

	/*Currently used:
	 	 PA6 to manually operate blinds up (TIM3->CH1)
	 	 PA7 to manually operate blinds down (TIM3->CH2)
	 	 PB0 to manually operate blinds open (TIM3->CH3)
	 	 PB1 to manually operate blinds close (TIM3->CH4)


	 */

	//Timer
	RCC->AHBENR |= RCC_AHBENR_GPIOAEN;

	GPIOA->MODER &= ~(3 << (2 * 6));	//Clear PA6
	GPIOA->MODER &= ~(3 << (2 * 7));	//Clear PA7

	GPIOA->MODER |= (2 << (2 * 6));		//Set PA6 to AF
	GPIOA->MODER |= (2 << (2 * 7));		//Set PA7 to AF

	GPIOA->AFR[0] &= ~(0xF << (4 * 6));	//Clear AFR[6]
	GPIOA->AFR[0] &= ~(0xF << (4 * 7));	//Clear AFR[7]

	GPIOA->AFR[0] |= (1 << (4 * 6));	//Set AFR[6] to AF1
	GPIOA->AFR[0] |= (1 << (4 * 7));	//Set AFR[7] to AF1

	GPIOA->PUPDR &= ~(3 << (2 * 6));	//Clear PUPDR 6
	GPIOA->PUPDR &= ~(3 << (2 * 7));	//Clear PUPDR 7

	GPIOA->PUPDR |= (2 << (2 * 6));		//Set Pull-Down
	GPIOA->PUPDR |= (2 << (2 * 7));		//Set Pull-Down

	RCC->APB1ENR |= RCC_APB1ENR_TIM3EN;	//Enable Timer3 in RCC

	TIM3->PSC = 1 - 1;
	TIM3->ARR = 0xFFFFFFFF;

	TIM3->CCMR1 &= ~TIM_CCMR1_CC1S;
	TIM3->CCMR1 &= ~TIM_CCMR1_CC2S;

	TIM3->CCMR1 |= TIM_CCMR1_CC1S_0;
	TIM3->CCMR1 |= TIM_CCMR1_CC2S_0;

	TIM3->CCER &= ~(TIM_CCER_CC1P | TIM_CCER_CC1NP);
	TIM3->CCER &= ~(TIM_CCER_CC1P | TIM_CCER_CC2NP);

	TIM3->CCER |= TIM_CCER_CC1E;
	TIM3->CCER |= TIM_CCER_CC2E;

	TIM3->DIER |= TIM_DIER_CC1IE;
	TIM3->DIER |= TIM_DIER_CC2IE;

	TIM3->CR1 |= (2 << 8);	//Set clock div4

	NVIC->ISER[0] = 1 << TIM3_IRQn;

	TIM3->CCMR1 |= TIM_CCMR1_IC1F_3 | TIM_CCMR1_IC1F_2 | TIM_CCMR1_IC1F_1 | TIM_CCMR1_IC1F_0;
	TIM3->CCMR1 |= TIM_CCMR1_IC2F_3 | TIM_CCMR1_IC2F_2 | TIM_CCMR1_IC2F_1 | TIM_CCMR1_IC2F_0;

}

void TIM3_IRQHandler()
{
	if((TIM3->SR & TIM_SR_UIF) != 0)
	{
		TIM3->SR &= ~1;
		return;
	}

	if(TIM3->SR & TIM_SR_CC1IF)
	{
		TIM2->CCR1 = 100;
		int __attribute((unused)) useless;
		useless = TIM2->CCR1;
		//return;
	}
	if(TIM3->SR & TIM_SR_CC2IF)
	{
		TIM2->CCR2 = 100;
		int __attribute((unused)) useless;
		useless = TIM2->CCR2;
	}
	if(TIM3->SR & TIM_SR_CC3IF)		//Blinds open
	{
		while((GPIOB->IDR & (1 << 0)) != 0)
		{
			delay(2);
			if(TIM2->CCR3 < 100)
			{
				TIM2->CCR3 += 10;
			}
		}
		int __attribute((unused)) useless;
		useless = TIM2->CCR3;
	}
	if(TIM3->SR & TIM_SR_CC4IF)		//Blinds Close
	{
		while((GPIOB->IDR & (1 << 1)) != 0)
		{
			delay(2);
			if(TIM2->CCR3 > 0)
			{
				TIM2->CCR3 -= 10;
			}
		}
		int __attribute((unused)) useless;
		useless = TIM2->CCR3;
	}

	while((GPIOA->IDR & (1 << 6)) != 0);		//Wait for button to stop being pressed
	while((GPIOA->IDR & (1 << 7)) != 0);
	while((GPIOB->IDR & (1 << 0)) != 0);
	while((GPIOB->IDR & (1 << 1)) != 0);

	int __attribute((unused)) useless;
	useless = TIM3->CCR1;
	useless = TIM3->CCR2;
	useless = TIM3->CCR3;
	useless = TIM3->CCR4;
	TIM2->CCR1 = 0;
	TIM2->CCR2 = 0;
	return;
}

void delay(float val)
{
	int i, max = (int)(val * 50000);
	for(i = 0; i < max; i++);
}


/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Initializes the CPU, AHB and APB busses clocks 
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }
  /** Initializes the CPU, AHB and APB busses clocks 
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */

  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(char *file, uint32_t line)
{ 
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     tex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/

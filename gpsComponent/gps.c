#include "legato.h"
#include "interfaces.h"
#include <arpa/inet.h>
#include <pthread.h>

#define VERSION "2017-06-06_1617_001"

#define SERVER_ADDR_V4 "103.31.19.236"   //gps server IP
#define SERVER_PORT  7778



#define GPSTIMEOUT 15

#define GPS_INTERVAL 30

#define LATITUDE_OFFSET  5
#define LONGITUDE_OFFSET 5

#define INVALID_DATA	0xFF


#define PAUSE_STATE 0
#define MOVE_STATE 1
#define STOP_STATE 2
#define ALERT_STATE 3


//Lock 
#define CAR_LOCK		0x00
#define CAR_UNLOCK		0x01

//ACC
#define ACC_ON			0x00
#define ACC_OFF			0x01

//Alert
#define BATTERY_ALERT 0x01
#define MOVE_ALERT    0x02
#define VIBRATE_ALERT 0x04



#define STOP_INTERVAL_TIME  60
#define ALERT_INTERVAL_TIME  60


#define MYDEBUG(param, fmt...) \
		do\
		{\
			char buf[512]={0};\
			sprintf(buf, param, ##fmt);\
			LE_INFO("aaaaa   %s", buf);\
		}while(0)


#define COPY_DATA(p, data, len) \
      do\
	  {\
	  	len =  sizeof(data);\
		memcpy(p, &data, len);\
		p+= len;\
	  }while(0)


	  
typedef struct gpsData
{
	uint16_t startTab;
	uint8_t dataLen;
	uint8_t cmdType;
	uint8_t unitId_h;
	uint16_t unitId_l;

	//date and time
	uint8_t year;
	uint8_t month;
	uint8_t day;
	uint8_t hour;
	uint8_t min;
	uint8_t second;

	uint8_t satelliteNum;

	uint32_t longitude;
	uint32_t latitude;
	uint32_t altitude;
	uint8_t  speed;
	uint16_t heading;
	uint16_t country;
	uint8_t  mcc;
	uint16_t lac;

	uint8_t cellId_h;
	uint16_t cellId_l;

	uint8_t batteryLevel;

	uint8_t sigalLevel;
	uint8_t carOnOff;
	uint8_t acc;
	uint8_t alert;

	uint32_t dataUsage;
	uint8_t dataType;
	
	uint8_t chksum;
	uint8_t stopTab;
} s_gpsData;	  


typedef struct gCtlData
{
	uint8_t curState;
	uint16_t pauseStartTime;
	
	uint32_t longitude;     //raw data
	uint32_t latitude;     

	uint8_t alertYear;
	uint8_t alertMonth;
	uint8_t alertDay;
	uint8_t alertHour;
	uint8_t alertMin;
	uint8_t alertSecond;
	
	uint8_t alertCount;
}s_gCtlData;


static s_gpsData gpsData;


//static s_gpsData gpsData;
static s_gCtlData gCtrlData;

uint8_t g_curAlertState;      //1:battery alert 2:move alert 3.vibrate alert
uint8_t g_curLockState;     //0: lock, 1: unlock
uint32_t g_count;

static pthread_mutex_t alert_mutex;

static void led_thread()
{
	bool lock_value;
	bool led_value;

	lock_value = le_gpioPin13_Read();
			
	//pull down
	if(lock_value == 0)
	{		
		if(g_curLockState == CAR_UNLOCK)
		{
			g_curLockState = CAR_LOCK;				
			
			le_gpioPin2_Activate();
			led_value = le_gpioPin2_Read();
			MYDEBUG("LockState:lock: %d", CAR_LOCK);
			
		}
		else 
		{
			g_curLockState = CAR_UNLOCK;
			g_curAlertState = INVALID_DATA;
											
			le_gpioPin2_Deactivate();
			led_value = le_gpioPin2_Read();
			//MYDEBUG("GPIO2(led)read Deactivate: %d", led_value);
			MYDEBUG("LockState:unlock: %d", CAR_UNLOCK);
		} 
	}
	
	if(g_curAlertState == MOVE_ALERT)
	{
		MYDEBUG("g_curAlertState move");
		led_value = le_gpioPin2_Read();
		if(led_value == 0)
		{
			le_gpioPin2_Activate();
		}
		else
		{
			le_gpioPin2_Deactivate();
		}
		
	}

}



static uint8_t GetChksum(char *buf, uint32_t len)
{
	uint32_t i = 0;
	uint32_t total = 0;
	uint8_t chksum = 0;
	for(i=0; i<len; i++)
	{
		total += buf[i];
	}
	
	LE_INFO("aaaaa total---0x%05x", total);
	
	chksum = total%256;
	
	LE_INFO("aaaaa chksum---0x%05x", chksum);
	
	return chksum;	
}


static void PrintDataBuf(char *buf, uint32_t len)
{
	uint32_t i = 0;
	for(i=0; i<len; i++)
	{
		if(i == 40 || i == 41)
		{
			LE_INFO("aaaaa %d---0x%02x", i, buf[i]);
		}
	}
}

#if 0
static void PrintRcvBuf(char *buf, uint32_t len)
{
	uint32_t i = 0;
	for(i=0; i<len; i++)
	{
		LE_INFO("aaaaa rcv:%d---0x%02x", i, buf[i]);
	}
}
#endif

static void SetDataBuf(char *buf)
{	
	uint8_t len = 0;
	char *p = buf;
	
	gpsData.startTab=htons(0x5d37);
	len =  sizeof(gpsData.startTab);
	memcpy(p, &gpsData.startTab, len);
	p+= len;
	
	gpsData.dataLen = 47;
	len =  sizeof(gpsData.dataLen);
	memcpy(p, &gpsData.dataLen, len);
	p+= len;
	
	gpsData.cmdType = 0;
	len =  sizeof(gpsData.cmdType);
	memcpy(p, &gpsData.cmdType, len);
	p+= len;
	
	//1135
	gpsData.unitId_h = 0x00;
	COPY_DATA(p, gpsData.unitId_h, len);
	
	
	gpsData.unitId_l = htons(1135);
	COPY_DATA(p, gpsData.unitId_l, len);

	//date and time
#if 0
	time_t cur_t;
	struct tm *p_tm;
	time(&cur_t);

	p_tm = gmtime(&cur_t);

	
	gpsData.day = p_tm->tm_mday;
	COPY_DATA(p, gpsData.day, len);
	
	gpsData.month = p_tm->tm_mon+1;
	COPY_DATA(p, gpsData.month, len);
	
	//gpsData.year = (p_tm->tm_year+1900-2000);
	gpsData.year = (p_tm->tm_year -100);
	COPY_DATA(p, gpsData.year, len);	
	
	gpsData.hour = p_tm->tm_hour;
	COPY_DATA(p, gpsData.hour, len);
	
	gpsData.min = p_tm->tm_min;
	COPY_DATA(p, gpsData.min, len);
	
	gpsData.second = p_tm->tm_sec;
	COPY_DATA(p, gpsData.second, len);
#endif

	COPY_DATA(p, gpsData.day, len);
	COPY_DATA(p, gpsData.month, len);
	COPY_DATA(p, gpsData.year, len);	
	COPY_DATA(p, gpsData.hour, len);
	COPY_DATA(p, gpsData.min, len);
	COPY_DATA(p, gpsData.second, len);
	
	LE_INFO("aaaaaa dateTime:%02d-%02d-%02d %02d:%02d:%02d\n", gpsData.year, gpsData.month, 
			gpsData.day, gpsData.hour, gpsData.min, gpsData.second);	
	

	gpsData.satelliteNum = 4;
	COPY_DATA(p, gpsData.satelliteNum, len);
	
	//ok
	gpsData.longitude = htonl(gpsData.longitude);     //113.900193, 0x06C9FAA1, ---0x42E3CCE6
	COPY_DATA(p, gpsData.longitude, len);
	
	gpsData.latitude = htonl(gpsData.latitude);     //22.514805,0x01578c75----0x41B41E52
	COPY_DATA(p, gpsData.latitude, len);
	
	gpsData.altitude =  htonl(0x41C00000);    //24.0--->0x41C00000
	COPY_DATA(p, gpsData.altitude, len);
	
	COPY_DATA(p, gpsData.speed, len);
	
	gpsData.heading = htons(9);
	COPY_DATA(p, gpsData.heading, len);
	
	gpsData.country = htons(460);
	COPY_DATA(p, gpsData.country, len);
	
	gpsData.mcc = 0;
	COPY_DATA(p, gpsData.mcc, len);
	
	gpsData.lac = htons(0x00);
	COPY_DATA(p, gpsData.lac, len);
	
	gpsData.cellId_h = 0x00;
	COPY_DATA(p, gpsData.cellId_h, len);
	
	gpsData.cellId_l = htons(0x0000);
	COPY_DATA(p, gpsData.cellId_l, len);
	
	gpsData.batteryLevel = 80;
	COPY_DATA(p, gpsData.batteryLevel, len);
	
	gpsData.sigalLevel = 2;
	COPY_DATA(p, gpsData.sigalLevel, len);
	
	gpsData.carOnOff = INVALID_DATA;
	COPY_DATA(p, gpsData.carOnOff, len);
	
	//acc and alet
	pthread_mutex_lock(&alert_mutex);
	if(g_curAlertState == MOVE_ALERT)
	{
		gpsData.acc = ACC_ON;
	}
	else
	{
		gpsData.acc = ACC_OFF;
	}
	gpsData.alert = g_curAlertState;
	
	pthread_mutex_unlock(&alert_mutex);	
	
	COPY_DATA(p, gpsData.acc, len);
	
	COPY_DATA(p, gpsData.alert, len);
	
	gpsData.dataUsage = htonl(49);
	COPY_DATA(p, gpsData.dataUsage, len);
	
	gpsData.dataType = 0x00;
	COPY_DATA(p, gpsData.dataType, len);
	
	gpsData.chksum = GetChksum(buf, 47);
	//gpsData.chksum = 0xCE;
	COPY_DATA(p, gpsData.chksum, len);
	
	gpsData.stopTab = 0x7E;
	COPY_DATA(p, gpsData.stopTab, len);
}

static void SendGPSData(void)
{
    int sockFd = 0;
    struct sockaddr_in servAddr;

    if ((sockFd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        //sprintf(buffer, "Failed to create socket");
        return;
    }

    LE_INFO("Connecting to %s (www.sierrawireless.com)\n", SERVER_ADDR_V4);

    servAddr.sin_family = AF_INET;
    servAddr.sin_port = htons(SERVER_PORT);
    servAddr.sin_addr.s_addr = inet_addr(SERVER_ADDR_V4);

    if (connect(sockFd, (struct sockaddr *)&servAddr, sizeof(servAddr)) < 0)
    {
        ;//sprintf(buffer, "Failed to connect to www.sierrawireless.com.");
		
		LE_INFO("aaaaa Failed to connect to www.sierrawireless.com.\n");
    }
    else
    {
        ;//sprintf(buffer, "Connection to www.sierrawireless.com was successful.");
		LE_INFO("aaaaaa Connection to www.sierrawireless.com was successful.\n");
    }
	
	char send_buf[49]={0};

	
	LE_INFO("aaaaaa gpsData size ===%d\n", sizeof(gpsData));
	
	SetDataBuf(send_buf);
	
	PrintDataBuf(send_buf, 49);
		
	//int ret = send(sockFd, send_buf, strlen(send_buf), 0);
	int ret = send(sockFd, send_buf, 49, 0);
	
	LE_INFO("aaaaaa send data num ret ===%d\n", ret);
	
	//LE_INFO("aaaaaa not send data num \n");
	
	//start receive data
#if 0
	char rcv_buf[14]={0};
	uint8_t rcv_num = 0;
	LE_INFO("aaaaaa start reveive data \n");
	
	if((rcv_num=recv(sockFd,rcv_buf,13,0)) == -1)
	{  
			LE_INFO("aaaaaa reveive data failed!!!!!!\n");
	} 
	else
	{
		PrintRcvBuf(rcv_buf, 12);
	}
#endif

 
    close(sockFd);
}

static  void getresult(int result)
{
	switch(result)
	{
		case LE_OK:
		LE_INFO("aaaaa LE_OK===%d, Successful.", LE_OK);
		break;

		case LE_NOT_FOUND:
		LE_INFO("aaaaa LE_NOT_FOUND===%d, Referenced item does not exist or could not be found.", LE_NOT_FOUND);
		break;

		case LE_NOT_POSSIBLE:
		LE_INFO("aaaaa LE_NOT_POSSIBLE===%d, LE_NOT_POSSIBLE.", LE_NOT_POSSIBLE);
		break;

		case LE_OUT_OF_RANGE:
		LE_INFO("aaaaa LE_OUT_OF_RANGE===%d, An index or other value is out of range..", LE_OUT_OF_RANGE);
		break;

		case LE_NO_MEMORY:
		LE_INFO("aaaaa LE_NO_MEMORY===%d, Insufficient memory is available. ", LE_NO_MEMORY);
		break;

		case LE_NOT_PERMITTED:
		LE_INFO("aaaaa LE_NOT_PERMITTED===%d, Current user does not have permission to perform requested action. ", LE_NOT_PERMITTED);
		break;

		case LE_FAULT:
		LE_INFO("aaaaa LE_FAULT===%d, Unspecified internal error. ", LE_FAULT);
		break;

		case LE_COMM_ERROR:
		LE_INFO("aaaaa LE_COMM_ERROR===%d, Communications error. ", LE_COMM_ERROR);
		break;

		case LE_TIMEOUT:
		LE_INFO("aaaaa LE_TIMEOUT===%d, A time-out occurred. ", LE_TIMEOUT);
		break;

		case LE_OVERFLOW:
		LE_INFO("aaaaa LE_OVERFLOW===%d, An overflow occurred or would have occurred. ", LE_OVERFLOW);
		break;

		case LE_UNDERFLOW:
		LE_INFO("aaaaa LE_UNDERFLOW===%d, An underflow occurred or would have occurred. ", LE_UNDERFLOW);
		break;

		case LE_WOULD_BLOCK:
		LE_INFO("aaaaa LE_WOULD_BLOCK===%d, Would have blocked if non-blocking behaviour was not requested. ", LE_WOULD_BLOCK);
		break;

		case LE_DEADLOCK:
		LE_INFO("aaaaa LE_DEADLOCK===%d, Would have caused a deadlock.", LE_DEADLOCK);
		break;

		case LE_FORMAT_ERROR:
		LE_INFO("aaaaa LE_FORMAT_ERROR===%d, Format error. ", LE_FORMAT_ERROR);
		break;

		case LE_DUPLICATE:
		LE_INFO("aaaaa LE_DUPLICATE===%d, Duplicate entry found or operation already performed. ", LE_DUPLICATE);
		break;

		case LE_BAD_PARAMETER:
		LE_INFO("aaaaa LE_BAD_PARAMETER===%d, Parameter is invalid. ", LE_BAD_PARAMETER);
		break;

		case LE_CLOSED:
		LE_INFO("aaaaa LE_CLOSED===%d, The resource is closed. ", LE_CLOSED);
		break;

		case LE_BUSY:
		LE_INFO("aaaaa LE_BUSY===%d, The resource is busy. ", LE_BUSY);
		break;

		case LE_UNSUPPORTED:
		LE_INFO("aaaaa LE_UNSUPPORTED===%d, The underlying resource does not support this operation. ", LE_UNSUPPORTED);
		break;

		case LE_IO_ERROR:
		LE_INFO("aaaaa LE_IO_ERROR===%d,  An IO operation failed. ", LE_IO_ERROR);
		break;

		case LE_NOT_IMPLEMENTED:
		LE_INFO("aaaaa LE_NOT_IMPLEMENTED===%d, Unimplemented functionality. ", LE_NOT_IMPLEMENTED);
		break;

		case LE_UNAVAILABLE:
		LE_INFO("aaaaa LE_UNAVAILABLE===%d, A transient or temporary loss of a service or resource. ", LE_UNAVAILABLE);
		break;

		case LE_TERMINATED:
		LE_INFO("aaaaa LE_TERMINATED===%d, The process, operation, data stream, session, etc. has stopped. ", LE_TERMINATED);
		break;

		default:
		LE_INFO("aaaaa UNKNOWN RESULT");
		break;
	}
}

static int32_t ConvertFloat2Hex(float data)
{
	int32_t result = 0;
	memcpy(&result, &data, sizeof(data));
	return result;
}

static void GetGPS(void)
{
    int32_t latitude;
    int32_t longitude;

    int32_t horizontalAccuracy;
	
	//speed var
	uint32_t hSpeed;
	uint32_t hSpeedAccuracy;
	int32_t vSpeed;
	int32_t vSpeedAccuracy;
	
	
	uint16_t hours = 0;
	uint16_t minutes = 0;
	uint16_t seconds = 0;

	uint16_t milliseconds = 0;
	
	
	uint16_t year = 0;
	uint16_t month = 0;
	uint16_t day = 0;


	le_result_t result;

	uint8_t error = 0;
	//uint8_t curState = 0;
	//uint16_t curStartTime = 0;

	int32_t ret = -1;

	ret = system("/legato/systems/current/bin/gnss get posInfo");

	LE_INFO("aaaaa ret ==%d", ret);


	result = le_pos_GetDate(&year, &month, &day);
	if (result != LE_OK)
	{
		LE_INFO("aaaaa GET GPS date error");
		error++;
	}

	result = le_pos_GetTime(&hours, &minutes, &seconds, &milliseconds);
	if (result != LE_OK)
	{
		LE_INFO("aaaaa GET GPS time error");
		error++;
	}

	result = le_pos_Get2DLocation(&latitude, &longitude, &horizontalAccuracy);
	if (result != LE_OK)
	{
		LE_INFO("aaaaa GET GPS Location error");
		error++;
	}
	
	result = le_pos_GetMotion (&hSpeed, &hSpeedAccuracy, &vSpeed, &vSpeedAccuracy);
	if (result != LE_OK)
	{
		LE_INFO("aaaaa GET GPS speed error");
		error++;
	}
	else
	{
		LE_INFO("aaaaa hspeed=====================%d", hSpeed);
	}
	


    if (error == 0)
    {

    	//curStartTime =  minutes*60+seconds;
		if(g_count == 0 || g_count == 1)
		{
			gCtrlData.curState = STOP_STATE;
		}

    	if(abs(latitude - gCtrlData.latitude) > LATITUDE_OFFSET || abs(longitude - gCtrlData.longitude) > LONGITUDE_OFFSET )
    	{
			gCtrlData.curState = MOVE_STATE;
			
			pthread_mutex_lock(&alert_mutex);
			if(g_curLockState == CAR_LOCK)
			{
				g_curAlertState = MOVE_ALERT;	
				LE_INFO("aaaaa MMMMMMMMMMMMMMMMMMMMMMMMMMMMMMM");
			}			
			pthread_mutex_unlock(&alert_mutex);			
    	}
    	else
    	{
			//curState = STOP_STATE;
			gCtrlData.curState = STOP_STATE;
			
			pthread_mutex_lock(&alert_mutex);
			g_curAlertState = INVALID_DATA;
			pthread_mutex_unlock(&alert_mutex);				
    	}

    	gpsData.year = year%100;
    	gpsData.month = month;
    	gpsData.day = day;

    	gpsData.hour = hours;
    	gpsData.min = minutes;
    	gpsData.second = seconds;
	
		gpsData.latitude = ConvertFloat2Hex(latitude/1000000.0);
		gpsData.longitude = ConvertFloat2Hex(longitude/1000000.0);

    	gpsData.heading = horizontalAccuracy;
		
		gpsData.speed = hSpeed;
		
	    gCtrlData.latitude = latitude;
    	gCtrlData.longitude = longitude;

		//LE_INFO("aaaaa get gps ok ok ok ok!!!, datetime:%04d-%02d-%02d, %02d:%02d:%02d, Loc:%d,%d, %d", year, month, day, hours, minutes, seconds, latitude, longitude, horizontalAccuracy);
    	LE_INFO("aaaaa get gps ok ok !!!,state:%d,  datetime:%04d-%02d-%02d, %02d:%02d:%02d, Loc:%d,%d, %d", gCtrlData.curState, gpsData.year+2000, gpsData.month, gpsData.day, gpsData.hour, gpsData.min, gpsData.second , longitude,latitude, gpsData.heading);

    }
    else
    {
    	getresult(result);
		LE_INFO("aaaaa get gps non non !!!!!-----%d", result);

    }

}



COMPONENT_INIT
{
	LE_INFO("aaaaa Hello, gps. 2222 version %s", VERSION);
	
	int32_t ret = -1;
	//pthread_t led_thread_id;
	
	le_gpioPin2_SetPushPullOutput(LE_GPIOPIN2_ACTIVE_HIGH, false);    //default value=0, led off, 
	le_gpioPin13_SetInput(LE_GPIOPIN13_ACTIVE_HIGH);     //default high, pull down , low, lock	
	

	ret = system("/legato/systems/current/bin/gnss fix");
	LE_INFO("aaaaa gnss fix ret ==%d", ret);
	sleep(10);
	
	ret = system("/legato/systems/current/bin/cm data connect &");
	LE_INFO("aaaaa cm data connect ret ==%d", ret);
	sleep(10);
	
	uint32_t ttff = 0;

    le_result_t result = le_gnss_GetTtff(&ttff);
	
    LE_ASSERT((result == LE_OK)||(result == LE_BUSY));
	
	while(result != LE_OK)
	{
		result = le_gnss_GetTtff(&ttff);
		LE_INFO("aaaaa TTFF start not available");
		sleep(2);
	}
	
    LE_INFO("aaaaa TTFF start = %d msec", ttff);
    
	le_pos_FixState_t fixState;
	LE_ASSERT((le_pos_GetFixState(&fixState) == LE_OK));
	LE_INFO("aaaaa position fix state %d", fixState);

	
	memset(&gpsData, 0, sizeof(gpsData));
	memset(&gCtrlData, 0, sizeof(gCtrlData));
	
	g_count = 0;
	gpsData.carOnOff = INVALID_DATA;
	gpsData.acc = ACC_OFF;	

	pthread_mutex_lock(&alert_mutex);
	g_curAlertState = INVALID_DATA;      //1:battery alert 2:move alert 3.vibrate alert
	g_curLockState = CAR_UNLOCK;         //0: lock, 1: unlock	
	pthread_mutex_unlock(&alert_mutex);	
	
	
	//create led thread
#if 0
	if((ret = pthread_create(&led_thread_id,NULL,led_thread,NULL)) != 0)
	{
		MYDEBUG("create thread error, -----------------------------------");
	}
#endif

#if 0
	  le_timer_Ref_t getLedTimer = le_timer_Create("GetLedTimer");
	  le_clk_Time_t intervalTick = {1, 0};
	  le_timer_SetInterval(getLedTimer, intervalTick);
	  le_timer_SetHandler(getLedTimer, &led_thread);
	  le_timer_Start(getLedTimer);
#endif



	while(1)
	{
		led_thread();
		
		if((g_count%30) == 0)
		{
			GetGPS();		
			SendGPSData();

			if(g_curLockState == CAR_UNLOCK)
			{
				MYDEBUG("lock state unlock");
			}
			else
			{
				MYDEBUG("lock state lock!!!");
			}

		}
		
		g_count++;





		sleep(1);
		//sleep(10);
	}
}

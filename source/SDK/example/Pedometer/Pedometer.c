/*
 * AIROHA Bluetooth Device Firmware
 * COPYRIGHT (C) 2014 AIROHA TECHNOLOGY CORP. CO., LTD. ALL RIGHTS RESERVED
 *
 */

#include "reside_flash.inc"

#include "rc.h"

#include "xl362.h"
#ifdef SDK_SUPPORTED_PEDOMETER

#define _PEDOMETER_C_

/* Time out */
#define XL362_DELAY_POWER_ON        15      //5 ms
#define XL362_DELAY_RESET           3       //1 ms
#define XL362_DELAY_MEASURE         65      //10 ms

#if (PEDOMETER_FIFO_MODE)
#define APP_FIFO_SAMPLE             12
#endif

#define X_CHANNEL   0
#define Y_CHANNEL   1
#define Z_CHANNEL   2

/* State */
#define PEDOMETER_STATE_NONE        0
#define PEDOMETER_STATE_CONFIG      1
#define PEDOMETER_STATE_STANDBY     2
#define PEDOMETER_STATE_MEASURE     3

#define TIMEWINDOW_MIN	10		//Time Window Minimum 10  0.02s~0.2s
#define TIMEWINDOW_MAX	100		//Time Window Maximum 100 0.02s~2s
#define REGULATION		4		//Find Stable Rule Steps
#define INVALID			2		//Lose Stable Rule Steps

U8 buffer[7];

U16 Acceleration[3] = {0,0,0};  //Three Axis Acceleration Value
U16 _array0[3] = {1,1,1};		//Register for Digital Filter
U16 _array1[3] = {1,1,1};		//Register for Digital Filter
U16 _array2[3] = {0,0,0};		//Register for Digital Filter
U16 _array3[3] = {0,0,0};		//Register for Digital Filter
U16 _adresult[3] = {0,0,0};		//Digital Filter Result
U16 _max[3] = {0,0,0};			//Maximum for Calculating Dynamic Threshold and Precision
U16 _min[3] = {4096,4096,4096};	//Minimum for Calculating Dynamic Threshold and Precision
U16 _vpp[3] = {0,0,0};			//Peak-to-Peak for Calculating Dynamic Precision 
U16 _dc[3] = {0,0,0};			//Dynamic Threshold
U16 _precision[3] = {24,24,24};	//Dynamic Precision 
U16 _old_fixed[3] = {0,0,0};	//Old Fixed Value
U16 _new_fixed[3] = {0,0,0};	//New Fixed Value
U16 STEPS = 0;
U16 OLD_STEPS = 0;              //Old Steps and New Steps
U8 _bad_flag[3] = {0,0,0};		//Bad Flag for Peak-to-Peak Value Less Than 0.5625g
U8 sampling_counter = 0;		//Record the Sample Times

U8 TempSteps = 0;				//Record the Temp Steps
U8 InvalidSteps = 0;			//Record the Invalid Steps
U8 ReReg = 2;					//Flag for Rule
								//2-New Begin
								//1-Have Begun, But Do not Find the Rule
								//0-Have Found the Rule

U8 Interval = 0;
U8 CountDigitalFilterTimes = 0;

typedef struct
{
	OST XDATA_PTR timerPtr;
    U8 state;
    U8 filterCtl;
    U8 immEnableMeasure;
#if (PEDOMETER_TEST_ENABLE)
    U8 scaleFactor;
#endif
}PEDOMETER_CTL;

PEDOMETER_CTL XDATA gPedometer_ctl;

#if (PEDOMETER_TEST_ENABLE) /* Sensor test function*/
VOID Pedometer_GetStatus(VOID)
{
    U8 tmp;

    tmp = xl362_ReadReg(XL362_REG_DEVID_AD);
    #if (DBG_ENABLE_PEDOMETER)
    LightDbgPrint("MMI - device: %x\n", (U8)tmp);
    #endif

    tmp = xl362_ReadReg(XL362_REG_STATUS);
    #if (DBG_ENABLE_PEDOMETER)
    LightDbgPrint("MMI - status: %x\n", (U8)tmp);
    #endif

}

VOID Pedometer_ReadAcceleration(VOID)
{
    U8 status; 
    U16 x, y, z;

    if(!SYS_IsTimerExpired(&gPedometer_ctl.timerPtr))
    {
        return;
    }

    status = xl362_ReadReg(XL362_REG_STATUS);
    #if (DBG_ENABLE_PEDOMETER)
    LightDbgPrint("MMI - status: %x\n", (U8)status);
    #endif
    if(status & XL362_STATUS_DATA_READY)
    {
        xl362_ReadMultiReg(XL362_REG_XDATA_L, buffer, 6);
        x = (U16)(buffer[1] & 0x0F);
        x = (x<<8) | buffer[0];
        x >>= gPedometer_ctl.scaleFactor;

        y = (U16)(buffer[3] & 0x0F);
        y = (y<<8) | buffer[2];
        y >>= gPedometer_ctl.scaleFactor;

        z = (U16)(buffer[5]& 0x0F);
        z = (z<<8) | buffer[4];
        z >>= gPedometer_ctl.scaleFactor;
        #if (DBG_ENABLE_PEDOMETER)
        LightDbgPrint("MMI - X:%d,Y:%d,Z:%d",(U8)x,(U8)y,(U8)z);
        #endif
    }
    SYS_SetTimer(&gPedometer_ctl.timerPtr, XL362_DELAY_MEASURE);
}
#endif  /* PEDOMETER_TEST_ENABLE */

VOID TimeWindow(VOID)
{
	if(2 == ReReg)		//if it is the first step, add TempStep directly
	{
		TempSteps++;
		Interval = 0;
		ReReg = 1;
		InvalidSteps = 0;	
	}
	else				//if it is not the first step, process as below
	{
		if((Interval >= TIMEWINDOW_MIN) && (Interval <= TIMEWINDOW_MAX))//if the step interval in the time window
		{
			InvalidSteps = 0;	
			if(1 == ReReg)					//if still not find the rule
			{
				TempSteps++;				//make TempSteps add one
				if(TempSteps >= REGULATION)	//if TempSteps reach the regulation number
				{
					ReReg = 0;				//Have found the rule
					STEPS = STEPS + TempSteps;//Update STEPS
					TempSteps = 0;
				}
				Interval = 0;
			}
			else if(0 == ReReg)				//if have found the rule, Updata STEPS directly
			{
				STEPS++;
				TempSteps = 0;
				Interval = 0;
			}
		}
		else if(Interval < TIMEWINDOW_MIN)	//if time interval less than the time window under threshold
		{	
			if(0 == ReReg)					//if have found the rule
			{
				if(InvalidSteps < 255) 	InvalidSteps++;	//make InvalidSteps add one
				if(InvalidSteps >= INVALID)				//if InvalidSteps reach the INVALID number, search the rule again.
				{	
					InvalidSteps = 0;
					ReReg = 1;
					TempSteps = 1;
					Interval = 0;
				}
				else				//otherwise, just discard this step
				{
					Interval=0;
				}
			}
			else if(1 == ReReg)		//if have not found the rule, the process looking for rule before is invalid, then search the rule again.
			{
				InvalidSteps = 0;	
				ReReg = 1;
				TempSteps = 1;
				Interval = 0;
			}
		}
		else if(Interval > TIMEWINDOW_MAX)	//if the interval more than upper threshold, the steps is interrupted, then searh the rule again.
		{
			InvalidSteps = 0;	
			ReReg = 1;						
			TempSteps = 1;
			Interval = 0;
		}
	}		
}

VOID Pedometer_CalculateStep(VOID)
{
    U16 samples;
    U16 num;
    U8 status; 
    U8 i;

    if(!SYS_IsTimerExpired(&gPedometer_ctl.timerPtr))
    {
        return;
    }

    OLD_STEPS = STEPS;

    status = xl362_ReadReg(XL362_REG_STATUS);
    #if (DBG_ENABLE_PEDOMETER)
        //LightDbgPrint("MMI - status: %x\n", (U8)status);
    #endif

    #if (PEDOMETER_FIFO_MODE)
        if(!(status & XL362_STATUS_FIFO_WATERMARK))
    #else
        if(!(status & XL362_STATUS_DATA_READY))
    #endif
        {
            SYS_SetTimer(&gPedometer_ctl.timerPtr, XL362_DELAY_MEASURE);
            return;

        }

    #if (PEDOMETER_FIFO_MODE)
    {
        U8 SampleH, SampleL;
        SampleH = xl362_ReadReg(XL362_REG_FIFO_ENTRIES_H);
        SampleL = xl362_ReadReg(XL362_REG_FIFO_ENTRIES_L);
        samples = (SampleH << 8) | SampleL;
        #ifdef DBG_ENABLE_PEDOMETER
            //SampleH = BYTE1(samples);
            //SampleL = BYTE0(samples);
            //LightDbgPrint("MMI - Samples:%x,%x,%x\n", (U8)SampleH,(U8)SampleL, (U8)status);
        #endif
    }
    #endif

#if (PEDOMETER_FIFO_MODE)
    //for(num=0; num<APP_FIFO_SAMPLE; num+=6)
    for(num=0; num<samples; num+=6)
#endif
    {
    	if(Interval < 0xFF)
    	{
    		Interval++;
    	}

        #if (PEDOMETER_FIFO_MODE)
    	    xl362_ReadMultiFIFO(buffer, 6);                     //read 3-axis data from FIFO
        #else
            xl362_ReadMultiReg(XL362_REG_XDATA_L, buffer, 6);   //read 3-axis data from register
        #endif

    	Acceleration[X_CHANNEL] = (U16)(buffer[1] & 0x0F);
    	Acceleration[X_CHANNEL] = (Acceleration[X_CHANNEL]<<8) | buffer[0];

    	Acceleration[Y_CHANNEL] = (U16)(buffer[3] & 0x0F);
    	Acceleration[Y_CHANNEL] = (Acceleration[Y_CHANNEL]<<8) | buffer[2];

    	Acceleration[Z_CHANNEL] = (U16)(buffer[5] & 0x0F);
    	Acceleration[Z_CHANNEL] = (Acceleration[Z_CHANNEL]<<8) | buffer[4];
    	
    	//Translate the twos complement to true binary code	(256+2048)->1g, (2048-256)->(-1)g										  
    	for(i=X_CHANNEL; i<=Z_CHANNEL; i++)
    	{
    		if(Acceleration[i] < 2048)
    		{
    			Acceleration[i] = Acceleration[i] + 2048;	
    		}
    		else if(Acceleration[i] >= 2048)
    		{
    			Acceleration[i] = Acceleration[i] - 2048;	
    		}
    	}

    	for(i=X_CHANNEL; i<=Z_CHANNEL; i++)
    	{
    		_array3[i] = _array2[i];
    		_array2[i] = _array1[i];
    		_array1[i] = _array0[i];
       		_array0[i] = Acceleration[i];
    		if(CountDigitalFilterTimes < 6)
    		{
    			CountDigitalFilterTimes++;
    		}
    		if(CountDigitalFilterTimes > 4)
    		{
    		   	_adresult[i] = _array0[i]+_array1[i]+_array2[i]+_array3[i];
    		  	_adresult[i] = _adresult[i]>>2;	//digital filter
    		}
    		if(_adresult[i] > _max[i])
    		{
    		    _max[i] = _adresult[i];     //Find the maximum of acceleration
		    }
    		if(_adresult[i] < _min[i])
    		{
    		    _min[i] = _adresult[i];     //Find the minimum of acceleration
            }
    	}

      	sampling_counter++;		//record the sample times

    	//----------------------------------Calculate the dynamic threshold and precision-----------------------//
        if(60 == sampling_counter)
        {               
          	sampling_counter=0;
    			
    		for(i=X_CHANNEL; i<=Z_CHANNEL; i++)
    		{
    			_vpp[i] = _max[i] -_min[i];
            	_dc[i] = _min[i] + (_vpp[i]>>1);
    			//make the maximum register min and minimum register max, so that they can be updated at next cycle immediately
    			_max[i] = 0;
            	_min[i] = 4096;					
    			_bad_flag[i] = 0;

    			if(_vpp[i] >= 1600)                             //1600---6.25g(experiment value, decided by customer)
    			{
    				_precision[i] = _vpp[i]/320;                //320---1.25g(experiment value, decided by customer) 
    			}
            	else if((_vpp[i] >= 496) && (_vpp[i]<1600))     //496---1.9375g(experiment value, decided by customer)            
    			{
    				_precision[i] = 48;                         //48---0.1875g(experiment value, decided by customer)
    			}
           		else if((_vpp[i] >= 144) && (_vpp[i] < 496))    //144---0.5625g(experiment value, decided by customer)  
                {
    				_precision[i] = 32;                         //32---0.125g(experiment value, decided by customer)
    			}  
    			else
           		{ 
              		_precision[i] = 16;
                	_bad_flag[i] = 1;
            	}
    		}
      	}
    	
    	//--------------------------Linear Shift Register--------------------------------------

    	for(i=X_CHANNEL;i<=Z_CHANNEL;i++)
    	{
    		_old_fixed[i] = _new_fixed[i];

        	if(_adresult[i] >= _new_fixed[i])                         
        	{   
         		if((_adresult[i]-_new_fixed[i]) >= _precision[i])
    			{
    				_new_fixed[i] = _adresult[i];
    			}
        	}
        	if(_adresult[i] < _new_fixed[i])
       	 	{   
           		if((_new_fixed[i]-_adresult[i]) >= _precision[i])
    			{
    				_new_fixed[i] = _adresult[i];
    			}
        	}
    	}

    	//------------------------- Judgement of dynamic threshold ----------------------------------
    	if((_vpp[X_CHANNEL] >= _vpp[Y_CHANNEL]) && (_vpp[X_CHANNEL] >= _vpp[Z_CHANNEL]))
    	{
    		if((_old_fixed[X_CHANNEL] >= _dc[X_CHANNEL]) && (_new_fixed[X_CHANNEL] < _dc[X_CHANNEL])
                && (0 == _bad_flag[X_CHANNEL]))        
    		{
    			TimeWindow();
    		} 
    	}
    	else if((_vpp[Y_CHANNEL] >= _vpp[X_CHANNEL]) && (_vpp[Y_CHANNEL] >= _vpp[Z_CHANNEL]))
    	{
    		if((_old_fixed[Y_CHANNEL] >= _dc[Y_CHANNEL]) && (_new_fixed[Y_CHANNEL] < _dc[Y_CHANNEL])
                &&(0 == _bad_flag[Y_CHANNEL]))        
    		{
    			TimeWindow();
    		}
    	}
    	else if((_vpp[Z_CHANNEL] >= _vpp[Y_CHANNEL]) && (_vpp[Z_CHANNEL] >= _vpp[X_CHANNEL]))
    	{
    		if((_old_fixed[Z_CHANNEL] >= _dc[Z_CHANNEL]) && (_new_fixed[Z_CHANNEL] < _dc[Z_CHANNEL])
    			&&(0 == _bad_flag[Z_CHANNEL]))        
    		{
    			TimeWindow();
    		}
    		
    	}
    }
	
    if(STEPS != OLD_STEPS)
    {
        //get step result
        LightDbgPrint("MMI - STEP: %d", (U8)STEPS);
    }
    
    SYS_SetTimer(&gPedometer_ctl.timerPtr, XL362_DELAY_MEASURE);
}

VOID Pedometer_ResetSteps(VOID)
{
    STEPS = 0;
    OLD_STEPS = 0;
}

U16 Pedometer_ReadCurrentSteps(VOID)
{
    return STEPS;
}

VOID Pedometer_StartMeasurement(VOID)
{
    if(gPedometer_ctl.state < PEDOMETER_STATE_STANDBY)
    {
        gPedometer_ctl.immEnableMeasure = 1;
    }
    else if(gPedometer_ctl.state == PEDOMETER_STATE_STANDBY)
    {
        gPedometer_ctl.state = PEDOMETER_STATE_MEASURE;
        xl362_WriteReg(XL362_REG_POWER_CTL, XL362_POWER_CTL_MEASURE);       /* XL362_REG_POWER_CTL */
        SYS_SetTimer(&gPedometer_ctl.timerPtr, XL362_DELAY_MEASURE);
    }
}

VOID Pedometer_StopMeasurement(VOID)
{
    gPedometer_ctl.immEnableMeasure = 0;
    if(gPedometer_ctl.state == PEDOMETER_STATE_MEASURE)
    {
        gPedometer_ctl.state = PEDOMETER_STATE_STANDBY;
        xl362_WriteReg(XL362_REG_POWER_CTL, XL362_POWER_CTL_STANDBY);       /* XL362_REG_POWER_CTL */
    }
}

VOID Pedometer_Config(VOID)
{
    if(!SYS_IsTimerExpired(&gPedometer_ctl.timerPtr))
    {
        return;
    }

    #if (PEDOMETER_FIFO_MODE)
        xl362_WriteReg(XL362_REG_FIFO_CONTROL, XL362_FIFO_MODE_DISABLE);
        xl362_WriteReg(XL362_REG_FIFO_CONTROL, XL362_FIFO_MODE_OLDEST_SAVED);
        xl362_WriteReg(XL362_REG_FIFO_SAMPLES, (APP_FIFO_SAMPLE-1));            /*(x * 3 �V 1)*/
    #endif

    xl362_WriteReg(XL362_REG_FILTER_CTL, gPedometer_ctl.filterCtl);

    if(!gPedometer_ctl.immEnableMeasure)
    {
        gPedometer_ctl.state = PEDOMETER_STATE_STANDBY;
    }
    else
    {
        gPedometer_ctl.immEnableMeasure = 0;
        xl362_WriteReg(XL362_REG_POWER_CTL, XL362_POWER_CTL_MEASURE);       /* XL362_REG_POWER_CTL */
        SYS_SetTimer(&gPedometer_ctl.timerPtr, XL362_DELAY_MEASURE);
        gPedometer_ctl.state = PEDOMETER_STATE_MEASURE;
    }
}

VOID Pedometer_SW_Reset(VOID)
{
    if(!SYS_IsTimerExpired(&gPedometer_ctl.timerPtr))
    {
        return;
    }

    /* Software reset, sensor placed in standby after reset */
    xl362_WriteReg(XL362_REG_SOFT_RESET, XL362_REG_SOFT_RESET_KEY);

    /* Need to delay at least 0.5ms */
    SYS_SetTimer(&gPedometer_ctl.timerPtr, XL362_DELAY_RESET);

    gPedometer_ctl.state = PEDOMETER_STATE_CONFIG;
}

VOID Pedometer(VOID)
{
    switch(gPedometer_ctl.state)
    {
        case PEDOMETER_STATE_NONE:
            Pedometer_SW_Reset();
        break;

        case PEDOMETER_STATE_CONFIG:
            Pedometer_Config();
        break;

        case PEDOMETER_STATE_STANDBY:
            // test
            Pedometer_StartMeasurement();
        break;

        case PEDOMETER_STATE_MEASURE:
            #if (PEDOMETER_TEST_ENABLE)
                Pedometer_ReadAcceleration();
            #else
                Pedometer_CalculateStep();
            #endif
        break;
    }
}

VOID Pedometer_SetFilterCtl(U8 rate, U8 range)
{
    #if (PEDOMETER_TEST_ENABLE)
    switch(range)
    {
        case PEDOMETER_MEASURE_RANGE_2G:
            gPedometer_ctl.scaleFactor = 10;
        break;

        case PEDOMETER_MEASURE_RANGE_4G:
            gPedometer_ctl.scaleFactor = 9;
        break;

        case PEDOMETER_MEASURE_RANGE_8G:
            gPedometer_ctl.scaleFactor = 8;
        break;
    }
    #endif

    range = range << 6;
    gPedometer_ctl.filterCtl = (rate | range);

    #if (DBG_ENABLE_PEDOMETER)
    LightDbgPrint("MMI - Pedometer_SetFilterCtl:%x", (U8)gPedometer_ctl.filterCtl);
    #endif
}

VOID Pedometer_Init(VOID)
{
	OS_memset((U8 XDATA_PTR)&gPedometer_ctl, 0, sizeof(gPedometer_ctl));

    Pedometer_SetFilterCtl(PEDOMETER_DATA_RATE_50, PEDOMETER_MEASURE_RANGE_8G);

    SYS_SetTimer(&gPedometer_ctl.timerPtr, XL362_DELAY_POWER_ON);
}

#endif

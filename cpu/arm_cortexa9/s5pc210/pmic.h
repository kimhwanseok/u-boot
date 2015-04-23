/**************************************************************************************
* 
*	Project Name : S5PC200 Validation
*
*	Copyright 2010 by Samsung Electronics, Inc.
*	All rights reserved.
*
*	Project Description :
*		This software is only for validating functions of the S5PC200.
*		Anybody can use this software without our permission.
*  
*--------------------------------------------------------------------------------------
* 
*	File Name : pmic.h
*  
*	File Description : PMIC test
*
*	Author : GiYoung Yoo
*	Dept. : AP Development Team
*	Created Date : 2010/7/6
*	Version : 0.1 
* 
*	History
*	- Created(GiYoung Yoo 2010/7/6)
*  
**************************************************************************************/
#ifndef __PMIC_H__
#define __PMIC_H__

#define MAX8649_ADDR    0xc0    // VDD_INT -I2C1
#define MAX8649A_ADDR   0xc4    //VDD_G3D - I2C0
#define MAX8952_ADDR    0xc0    //VDD_ARM - I2C0

typedef enum
{
	eSingle_Buck ,
	eMAXIM ,
	eNS ,
} PMIC_MODEL_et;

typedef enum
{
	eVDD_ARM ,
	eVDD_INT ,
	eVDD_G3D ,
} PMIC_et;

typedef enum
{
	eVID_MODE0 ,
	eVID_MODE1 ,
	eVID_MODE2 ,
	eVID_MODE3,
} PMIC_MODE_et;

typedef struct {
	char*	name;           //voltage name
	u8	ucDIV_Addr;     //device address
	u8	ucI2C_Ch;       //iic ch
}PMIC_Type_st; 

extern void PMIC_InitIp(void);

void PMIC_SetVoltage_Buck(PMIC_et ePMIC_TYPE ,PMIC_MODE_et uVID, float fVoltage_value );

#endif //__PMIC_H__


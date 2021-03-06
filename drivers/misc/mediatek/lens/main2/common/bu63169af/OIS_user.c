/*
 * Copyright (C) 2015 MediaTek Inc.
 * Copyright (C) 2021 XiaoMi, Inc.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */

#ifndef OIS_USER_C
#define OIS_USER_C
#endif

#include <linux/mutex.h>
#include "OIS_head.h"

unsigned short int FOCUS_VAL = 0x0122;
static DEFINE_MUTEX(ois_mutex);

/* ============================================ */
void VCOSET0(void)
{
	unsigned short int CLK_PS = 24000;
	unsigned short int FVCO_1 = 36000; /* Target Frequency [kHz] */
	/* 27000 for 63163 */
	/* 36000 for 63165 */
	unsigned short int FREF = 25; /* Reference Clock Frequency [kHz] */

	unsigned short int DIV_N = CLK_PS / FREF - 1; /* calc DIV_N */
	unsigned short int DIV_M = FVCO_1 / FREF - 1; /* calc DIV_M */

	I2C_OIS_per_write(0x62,
			  DIV_N); /* Divider for internal reference clock */
	I2C_OIS_per_write(0x63, DIV_M);  /* Divider for internal PLL clock */
	I2C_OIS_per_write(0x64, 0x4060); /* Loop Filter */

	I2C_OIS_per_write(0x60, 0x3011); /* PLL */
	I2C_OIS_per_write(0x65, 0x0080); /*  */
	I2C_OIS_per_write(0x61, 0x8002); /* VCOON */
	I2C_OIS_per_write(0x61, 0x8003); /* Circuit ON */
	I2C_OIS_per_write(0x61, 0x8809); /* PLL ON */
}

void VCOSET1(void)
{
	I2C_OIS_per_write(0x05,
			  0x000C); /* Prepare for PLL clock as master clock */
	I2C_OIS_per_write(0x05, 0x000D); /* Change to PLL clock */
}

/* ///////////////////////////////////////////////////////// */
/* Write Data to Slave device via I2C master device */
/* --------------------------------------------------------- */
/* <Function> */
/* I2C master send these data to the I2C slave device. */
/* This function relate to your own circuit. */
/*  */
/* <Input> */
/* unsigned char       slvadr  I2C slave adr */
/* unsigned char       size    Transfer Size */
/* unsigned char       *dat    data matrix */
/*  */
/* <Output> */
/* none */
/*  */
/* <Description> */
/* [S][SlaveAdr][W]+[dat[0]]+...+[dat[size-1]][P] */
/*  */
/* ========================================================= */
void WR_I2C(unsigned char slvadr, unsigned char size, unsigned char *dat)
{
	s4AF_WriteReg_BU63169AF(slvadr << 1, dat, size);
}

/* ********************************************************* */
/* Read Data from Slave device via I2C master device */
/* --------------------------------------------------------- */
/* <Function> */
/* I2C master read data from the I2C slave device. */
/* This function relate to your own circuit. */
/*  */
/* <Input> */
/* unsigned char       slvadr  I2C slave adr */
/* unsigned char       size    Transfer Size */
/* unsigned char       *dat    data matrix */
/*  */
/* <Output> */
/* unsigned short int       16bit data read from I2C Slave device */
/*  */
/* <Description> */
/* if size == 1 */
/* [S][SlaveAdr][W]+[dat[0]]+         [RS][SlaveAdr][R]+[RD_DAT0]+[RD_DAT1][P]
 */
/* if size == 2 */
/* [S][SlaveAdr][W]+[dat[0]]+[dat[1]]+[RS][SlaveAdr][R]+[RD_DAT0]+[RD_DAT1][P]
 */
/*  */
/* ********************************************************* */
unsigned short int RD_I2C(unsigned char slvadr, unsigned char size,
			  unsigned char *dat)
{
	unsigned short int read_data = 0;
	unsigned short int read_data_h = 0;

	if (size == 1) {
		dat[1] = 0;
		s4AF_ReadReg_BU63169AF(slvadr << 1, dat, 2,
				       (unsigned char *)&read_data, 2);
	} else if (size == 2) {
		s4AF_ReadReg_BU63169AF(slvadr << 1, dat, 2,
				       (unsigned char *)&read_data, 2);
	}

	read_data_h = read_data >> 8;
	read_data = read_data << 8;
	read_data = read_data | read_data_h;

	return read_data;
}

/* ********************************************************* */
/* Write Factory Adjusted data to the non-volatile memory */
/* --------------------------------------------------------- */
/* <Function> */
/* Factory adjusted data are sotred somewhere */
/* non-volatile memory. */
/*  */
/* <Input> */
/* struct _FACT_ADJ       Factory Adjusted data */
/*  */
/* <Output> */
/* none */
/*  */
/* <Description> */
/* You have to port your own system. */
/*  */
/* ********************************************************* */
void store_FADJ_MEM_to_non_volatile_memory(struct _FACT_ADJ param)
{

}

/* ********************************************************* */
/* Read Factory Adjusted data from the non-volatile memory */
/* --------------------------------------------------------- */
/* <Function> */
/* Factory adjusted data are sotred somewhere */
/* non-volatile memory.  I2C master has to read these */
/* data and store the data to the OIS controller. */
/*  */
/* <Input> */
/* none */
/*  */
/* <Output> */
/* struct _FACT_ADJ       Factory Adjusted data */
/*  */
/* <Description> */
/* You have to port your own system. */
/*  */
/* ********************************************************* */
struct _FACT_ADJ get_FADJ_MEM_from_non_volatile_memory(void)
{
	unsigned short ReadData;

	if (0x01 == g_module_id_OIS) {
		mutex_lock(&ois_mutex);
		s4EEPROM_ReadReg_BU63169AF(0x1205, &ReadData);
		FADJ_MEM.gl_CURDAT = (unsigned short int)ReadData;
		mutex_unlock(&ois_mutex);

		mutex_lock(&ois_mutex);
		s4EEPROM_ReadReg_BU63169AF(0x1207, &ReadData);
		FADJ_MEM.gl_HALOFS_X = (unsigned short int)ReadData;
		mutex_unlock(&ois_mutex);

		mutex_lock(&ois_mutex);
		s4EEPROM_ReadReg_BU63169AF(0x1209, &ReadData);
		FADJ_MEM.gl_HALOFS_Y = (unsigned short int)ReadData;
		mutex_unlock(&ois_mutex);

		mutex_lock(&ois_mutex);
		s4EEPROM_ReadReg_BU63169AF(0x120B, &ReadData);
		FADJ_MEM.gl_HX_OFS = (unsigned short int)ReadData;
		mutex_unlock(&ois_mutex);

		mutex_lock(&ois_mutex);
		s4EEPROM_ReadReg_BU63169AF(0x120D, &ReadData);
		FADJ_MEM.gl_HY_OFS = (unsigned short int)ReadData;
		mutex_unlock(&ois_mutex);

		mutex_lock(&ois_mutex);
		s4EEPROM_ReadReg_BU63169AF(0x120F, &ReadData);
		FADJ_MEM.gl_PSTXOF = (unsigned short int)ReadData;
		mutex_unlock(&ois_mutex);

		mutex_lock(&ois_mutex);
		s4EEPROM_ReadReg_BU63169AF(0x1211, &ReadData);
		FADJ_MEM.gl_PSTYOF = (unsigned short int)ReadData;
		mutex_unlock(&ois_mutex);

		mutex_lock(&ois_mutex);
		s4EEPROM_ReadReg_BU63169AF(0x1213, &ReadData);
		FADJ_MEM.gl_GX_OFS = (unsigned short int)ReadData;
		mutex_unlock(&ois_mutex);

		mutex_lock(&ois_mutex);
		s4EEPROM_ReadReg_BU63169AF(0x1215, &ReadData);
		FADJ_MEM.gl_GY_OFS = (unsigned short int)ReadData;
		mutex_unlock(&ois_mutex);

		mutex_lock(&ois_mutex);
		s4EEPROM_ReadReg_BU63169AF(0x1217, &ReadData);
		FADJ_MEM.gl_KgxHG = (unsigned short int)ReadData;
		mutex_unlock(&ois_mutex);

		mutex_lock(&ois_mutex);
		s4EEPROM_ReadReg_BU63169AF(0x1219, &ReadData);
		FADJ_MEM.gl_KgyHG = (unsigned short int)ReadData;
		mutex_unlock(&ois_mutex);

		mutex_lock(&ois_mutex);
		s4EEPROM_ReadReg_BU63169AF(0x121B, &ReadData);
		FADJ_MEM.gl_KGXG = (unsigned short int)ReadData;
		mutex_unlock(&ois_mutex);

		mutex_lock(&ois_mutex);
		s4EEPROM_ReadReg_BU63169AF(0x121D, &ReadData);
		FADJ_MEM.gl_KGYG = (unsigned short int)ReadData;
		mutex_unlock(&ois_mutex);

		mutex_lock(&ois_mutex);
		s4EEPROM_ReadReg_BU63169AF(0x121F, &ReadData);
		FADJ_MEM.gl_SFTHAL_X = (unsigned short int)ReadData;
		mutex_unlock(&ois_mutex);

		mutex_lock(&ois_mutex);
		s4EEPROM_ReadReg_BU63169AF(0x1221, &ReadData);
		FADJ_MEM.gl_SFTHAL_Y = (unsigned short int)ReadData;
		mutex_unlock(&ois_mutex);

		mutex_lock(&ois_mutex);
		s4EEPROM_ReadReg_BU63169AF(0x1223, &ReadData);
		FADJ_MEM.gl_TMP_X_ = (unsigned short int)ReadData;
		mutex_unlock(&ois_mutex);

		mutex_lock(&ois_mutex);
		s4EEPROM_ReadReg_BU63169AF(0x1225, &ReadData);
		FADJ_MEM.gl_TMP_Y_ = (unsigned short int)ReadData;
		mutex_unlock(&ois_mutex);

		mutex_lock(&ois_mutex);
		s4EEPROM_ReadReg_BU63169AF(0x1227, &ReadData);
		FADJ_MEM.gl_KgxH0 = (unsigned short int)ReadData;
		mutex_unlock(&ois_mutex);

		mutex_lock(&ois_mutex);
		s4EEPROM_ReadReg_BU63169AF(0x1229, &ReadData);
		FADJ_MEM.gl_KgyH0 = (unsigned short int)ReadData;
		mutex_unlock(&ois_mutex);
	} else {
		mutex_lock(&ois_mutex);
		s4EEPROM_ReadReg_BU63169AF(0x1299, &ReadData);
		FADJ_MEM.gl_CURDAT = (unsigned short int)ReadData;
		mutex_unlock(&ois_mutex);

		mutex_lock(&ois_mutex);
		s4EEPROM_ReadReg_BU63169AF(0x129B, &ReadData);
		FADJ_MEM.gl_HALOFS_X = (unsigned short int)ReadData;
		mutex_unlock(&ois_mutex);

		mutex_lock(&ois_mutex);
		s4EEPROM_ReadReg_BU63169AF(0x129D, &ReadData);
		FADJ_MEM.gl_HALOFS_Y = (unsigned short int)ReadData;
		mutex_unlock(&ois_mutex);

		mutex_lock(&ois_mutex);
		s4EEPROM_ReadReg_BU63169AF(0x129F, &ReadData);
		FADJ_MEM.gl_HX_OFS = (unsigned short int)ReadData;
		mutex_unlock(&ois_mutex);

		mutex_lock(&ois_mutex);
		s4EEPROM_ReadReg_BU63169AF(0x12A1, &ReadData);
		FADJ_MEM.gl_HY_OFS = (unsigned short int)ReadData;
		mutex_unlock(&ois_mutex);

		mutex_lock(&ois_mutex);
		s4EEPROM_ReadReg_BU63169AF(0x12A3, &ReadData);
		FADJ_MEM.gl_PSTXOF = (unsigned short int)ReadData;
		mutex_unlock(&ois_mutex);

		mutex_lock(&ois_mutex);
		s4EEPROM_ReadReg_BU63169AF(0x12A5, &ReadData);
		FADJ_MEM.gl_PSTYOF = (unsigned short int)ReadData;
		mutex_unlock(&ois_mutex);

		mutex_lock(&ois_mutex);
		s4EEPROM_ReadReg_BU63169AF(0x12A7, &ReadData);
		FADJ_MEM.gl_GX_OFS = (unsigned short int)ReadData;
		mutex_unlock(&ois_mutex);

		mutex_lock(&ois_mutex);
		s4EEPROM_ReadReg_BU63169AF(0x12A9, &ReadData);
		FADJ_MEM.gl_GY_OFS = (unsigned short int)ReadData;
		mutex_unlock(&ois_mutex);

		mutex_lock(&ois_mutex);
		s4EEPROM_ReadReg_BU63169AF(0x12AB, &ReadData);
		FADJ_MEM.gl_KgxHG = (unsigned short int)ReadData;
		mutex_unlock(&ois_mutex);

		mutex_lock(&ois_mutex);
		s4EEPROM_ReadReg_BU63169AF(0x12AD, &ReadData);
		FADJ_MEM.gl_KgyHG = (unsigned short int)ReadData;
		mutex_unlock(&ois_mutex);

		mutex_lock(&ois_mutex);
		s4EEPROM_ReadReg_BU63169AF(0x12AF, &ReadData);
		FADJ_MEM.gl_KGXG = (unsigned short int)ReadData;
		mutex_unlock(&ois_mutex);

		mutex_lock(&ois_mutex);
		s4EEPROM_ReadReg_BU63169AF(0x12B1, &ReadData);
		FADJ_MEM.gl_KGYG = (unsigned short int)ReadData;
		mutex_unlock(&ois_mutex);

		mutex_lock(&ois_mutex);
		s4EEPROM_ReadReg_BU63169AF(0x12B3, &ReadData);
		FADJ_MEM.gl_SFTHAL_X = (unsigned short int)ReadData;
		mutex_unlock(&ois_mutex);

		mutex_lock(&ois_mutex);
		s4EEPROM_ReadReg_BU63169AF(0x12B5, &ReadData);
		FADJ_MEM.gl_SFTHAL_Y = (unsigned short int)ReadData;
		mutex_unlock(&ois_mutex);

		mutex_lock(&ois_mutex);
		s4EEPROM_ReadReg_BU63169AF(0x12B7, &ReadData);
		FADJ_MEM.gl_TMP_X_ = (unsigned short int)ReadData;
		mutex_unlock(&ois_mutex);

		mutex_lock(&ois_mutex);
		s4EEPROM_ReadReg_BU63169AF(0x12B9, &ReadData);
		FADJ_MEM.gl_TMP_Y_ = (unsigned short int)ReadData;
		mutex_unlock(&ois_mutex);

		mutex_lock(&ois_mutex);
		s4EEPROM_ReadReg_BU63169AF(0x12BB, &ReadData);
		FADJ_MEM.gl_KgxH0 = (unsigned short int)ReadData;
		mutex_unlock(&ois_mutex);

		mutex_lock(&ois_mutex);
		s4EEPROM_ReadReg_BU63169AF(0x12BD, &ReadData);
		FADJ_MEM.gl_KgyH0 = (unsigned short int)ReadData;
		mutex_unlock(&ois_mutex);
	}

	return FADJ_MEM; /* Note: This return data is for DEBUG. */
}

/* ********************************************************* */
/* Wait */
/* --------------------------------------------------------- */
/* <Function> */
/*  */
/* <Input> */
/* unsigned long int       time    on the micro second time scale */
/*  */
/* <Output> */
/* none */
/*  */
/* <Description> */
/*  */
/* ********************************************************* */
void Wait_usec(unsigned long int time)
{
	/* Please write your source code here. */
}

#ifdef DEBUG_FADJ
/* ********************************************************* */
/* Printf for DEBUG */
/* --------------------------------------------------------- */
/* <Function> */
/*  */
/* <Input> */
/* const char *format, ... */
/* Same as printf */
/*  */
/* <Output> */
/* none */
/*  */
/* <Description> */
/*  */
/* ********************************************************* */
int debug_print(const char *format, ...)
{
	return 0;
}

#endif

#pragma once

#define ANTI_LOG_LUT_NAME	"Anti Log\0"

typedef struct _lut_param_name_value_struct
{
	unsigned char param_name[32];
	double value;
}lut_param_name_value_struct;

typedef struct _lut_information_struct
{
	int index;
	unsigned char lut_name_ptr[256];
	void (*lut_creation_fn)(UINT16 * pLUT,UINT32 param_count,lut_param_name_value_struct * pParamValue,char * fileName);
	void (*lut_param_update_fn)(void * pArg,unsigned char * pLutName,UINT32 param_count,lut_param_name_value_struct * pParamValue);
	unsigned int param_count;
	lut_param_name_value_struct param_array[20];
}lut_information_struct;

typedef enum AntiLog_Param_Enum
{
	ANTILOG_DISPLAY_GAMMA=0,
	ANTILOG_FILM_GAMMA=1,
	ANTILOG_REF_WHITE=2,
	ANTILOG_REF_BLACK=3,
	ANTILOG_SOFT_CLIP=4,
};
void anitlog_creation_fn(UINT16 * pLUT,UINT32 param_count,lut_param_name_value_struct * pParamValue,char * fileName);
void invert_lut_creation_fn(UINT16 * pLUT,UINT32 param_count,lut_param_name_value_struct * pParamValue,char * fileName);
void transparent_lut_creation_fn(UINT16 * pLUT,UINT32 param_count,lut_param_name_value_struct * pParamValue,char * fileName);
int load_lut_fromfile(UINT16 * pLUT,char * pLutFileName);
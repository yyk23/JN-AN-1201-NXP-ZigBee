/*
 * app_uart.h
 *
 *  Created on: 2017��10��23��
 *      Author: 1
 */

#ifndef APP_UART_H_
#define APP_UART_H_

#include "app_events.h"
#define BUILD_UINT16(loByte, hiByte) \
          ((uint16)(((loByte) & 0x00FF) + (((hiByte) & 0x00FF) << 8)))


//���Ҵ��붨��
#define XTHN_COMPANY        0x81
#define STWE_COMPANY		0x03
#define SWTL_COMPANY        0x05
//�豸YCLID����
#define YCLID_METHANE                       0x59020112  //��Ȼ��й©������
#define YCLID_SMOKE                         0x59020106  //����������
#define YCLID_CALL                          0x59020136  //�������д�����
#define YCLID_PIR                           0x59020102 //���⴫����
#define YCLID_ED                            0x59020132  //�Ŵ�̽������
#define YCLID_WA                            0x59020116 //ˮ��������

#define YCLID_WQ                            0x59040102  //ˮ�ʼ��


#define UART_RX_TIMEOUT_VALUE     1000  //�������˳��͹��ĺ�Ľ�����ʱʱ��
#define UART_TX_TIMEOUT_VALUE     100    //���ڷ�����ʱʱ�䣬100ms


#define JOIN_FLAG_VALUE           55

#define VALID_VALUE               0xAA

#define M_PROFILE_ID              HA_PROFILE_ID
#define M_CLUSTER_ID              HOME_AUTOMATION_IASZONE_CLUSTER_ID  //0x0500
#define ATTYSW_CLUSTER_ID         0xFC01

//Ĭ�ϵ͵���ֵ
#define DEF_LOW_POWER_VALUE        10

//Ĭ������ʱ��
#define DEF_HEARTBEAT_VALUE        0x0003

#pragma pack(1)    //����1�ֽڶ���,ָ�����뷽ʽ

typedef uint16 tDev_TypeCode;
typedef uint8  tDev_SvCode;
typedef uint8  tDev_HvCode;
typedef uint32 tDev_NumID;
typedef uint32 tDev_YCLID;


 typedef  union {
        uint8 YCL_Array[13];
        struct {
        	uint8  Num;
        	uint32 YCL_ID;
        	uint64 Mac;
        };
}uYcl;

typedef  union {
       uint8 YCL_Array[4];
       uint32 YCL_ID;
}ucs;

typedef  union {
	uint8 Sv_Array[10];
	struct {
		uint32 Sv_YCL_ID;
		uint8 Sv_Mainv_Num;
		uint8 Sv_Secv_Num;
		uint8 Sv_Modv_Num;
		uint8 Sv_Dev_Date[3];
	};

}uSoft_Ver;

typedef  union {
	uint8 Hv_Array[11];
	struct {
		uint8 Hv_Logo;
		uint32 Hv_YCL_ID;
		uint16 Hv_TecPro;
		uint16 Hv_Dev_Date[2];
		uint8  Hv_Dev_Company;
		uint8  Hv_Prod_Ser;
	};
}uHard_Ver;

typedef struct
{
    uint8 				dev_state ;
    uint8 				dev_power_state;
    uint8 				*dev_data;
    uint8 				dev_data_num;
 }tsSen_Status_Data;


typedef struct
{
	uint8    			Dev_Serve_CompanyCode;//���������Ҵ���
	tDev_TypeCode   	Dev_TypeCode;//�豸���룬����˾��Ʒ����
	tDev_SvCode         Dev_SvCode;//�����汾
	tDev_HvCode         Dev_HvCode;//Ӳ���汾
	tDev_NumID          Dev_NumID; //�豸���
	tDev_YCLID 			Dev_YCLID;
}tsEP_SenDev_inf;


typedef struct
{
	uint8    			Valid;
	uint16   			heartbeat_value;//�������ڣ���λ����
}tsEP_Dev_HeartBeat;


typedef struct
{
	uint8               Data_valid;
	tsEP_Dev_HeartBeat 	sEP_Dev_HeartBeat;//��������
	uYcl  				sM_YCL;//ͨ��ģ���YCL
	uint16              M_ProfileID;
	uint16 				M_ClusterID;//Cluster ID
	uSoft_Ver 			sM_Sv;//ͨ��ģ��������汾
	uHard_Ver 			sM_Hv;//ͨ��ģ���Ӳ���汾
	uint8 				Sensing_flag;//����ģ��ʹ��
	uint8               S_valid_flag;//����ģ�� =������Ϣ����Ч�Լ��
	uint8               Serve_CompanyCode;//���г��Ҵ���
	uYcl 				sS_YCL;//����ģ���YCL
	uSoft_Ver 			sS_Sv;//����ģ��������汾
	uHard_Ver 			sS_Hv;//����ģ���Ӳ���汾
}tsEP_Dev_Inf;

typedef struct
{
	tDev_TypeCode Dev_TypeCode;//
	uint32 S_YCL_ID;

}tsMS_Type_Sw;

//Attr_Type�Ķ���
#define ATTR_ID         0x00  //�豸��ClusterID������
#define CMD_ID          0x01  //�豸����ID
#define SIG_ID     		0x02  //�豸�ź�ǿ��
#define BASIC_ATTR_ID   0x10
//Attr_ZID����


typedef union
{
	uint32  Attr_SW_Data;
	struct{
			uint8  Attr_Type;//
			uint8  Attr_DataType;
			uint16 Attr_ZID;
	};
}tsAttr_Sw;

typedef struct
{
	uYcl  				sM_YCL;//ͨ��ģ���YCL
	uint16 				M_ClusterID;//Cluster ID
	uint16   			heartbeat_value;//����ʱ��
	uSoft_Ver 			sM_Sv;//ͨ��ģ��������汾
	uHard_Ver 			sM_Hv;//ͨ��ģ��������汾
	uYcl 				sS_YCL;//����ģ��������汾
	uint8               Serve_CompanyCode;//����ģ��ĳ���
	uSoft_Ver 			sS_Sv;//����ģ��������汾
	uHard_Ver 			sS_Hv;//����ģ���Ӳ���汾

}tsCluster_Basic_Attr;

/* Basic Type */
typedef enum
{
	 E_CLD_BASIC_M_YCL             = 0xF001, //ͨ��ģ���YCL
	 E_CLD_BASIC_M_CLUSTERID       = 0xF002, // Cluster ID
	 E_CLD_BASIC_HEARTBEAT_VALUE   = 0xF003,//����ʱ��
	 E_CLD_BASIC_M_SV              = 0xF004,//ͨ��ģ��������汾
	 E_CLD_BASIC_M_HV              = 0xF005,//ͨ��ģ��������汾
	 E_CLD_BASIC_S_YCL         	   = 0xF020,//����ģ��������汾
	 E_CLD_BASIC_S_COMPANYCODE     = 0xF021,//����ģ��ĳ���
	 E_CLD_BASIC_S_SV			   = 0xF022,//����ģ��������汾
	 E_CLD_BASIC_S_HV         	   = 0xF023 //����ģ���Ӳ���汾
} tsCluster_Basic_AttrID;


typedef struct
{
	uint16 				dev_status;
	uint8  				power_value;
}tsCluster_IASZONE_Attr;


/* IAS Zone Type */
typedef enum
{
	 E_CLD_IASZONE_STATUS             = 0x0002, //״̬
	 E_CLD_IASZONE_POWER_VALUE        = 0xFF00, //����
} tsCluster_IASZONE_AttrID;


typedef struct
{
	uint8 				Attr_Num;
	tsAttr_Sw  			Attr_Sw_1;
	tsAttr_Sw  			Attr_Sw_2;
	tsAttr_Sw  			Attr_Sw_3;
	tsAttr_Sw  			Attr_Sw_4;
}tsCluster_AttrSw_Attr;
#define ATTR_SW_NUM                  4
typedef enum
{
	 E_CLD_ATTRSW_ATTR_NUM             = 0x0000,
	 E_CLD_ATTRSW_ATTR_SW1             = 0x0001,
	 E_CLD_ATTRSW_ATTR_SW2             = 0x0002,
	 E_CLD_ATTRSW_ATTR_SW3             = 0x0003,
	 E_CLD_ATTRSW_ATTR_SW4             = 0x0004,

} tsCluster_AttrSw_AttrID;



//������Կ�Ķ���
typedef  union {
       uint8 LinkKeyArray[16];
       struct {
    	   	   uYcl    LinkKey_YCL;
       	       uint32  LinkKey_Last4By;
       };
}usLinkKey;


tsEP_Dev_Inf  	        sEP_Dev_Inf; //ͨ���豸������Ϣ�Ĵ洢
tsSen_Status_Data  		sSen_Status_Data;
uint8  			    	alarm_state;
uint8 			        Uart_TaskID;
usLinkKey               sLinkKey;
tsCluster_Basic_Attr   sCluster_Basic_Attr;
tsCluster_IASZONE_Attr sCluster_IASZONE_Attr;
tsCluster_AttrSw_Attr  sCluster_AttrSw_Attr;

#pragma pack()


PUBLIC void Uart_Task_Init(void);

PUBLIC void User_Uart_Init(void);
PUBLIC uint16 Uart_ProcessEvent( uint8 task_id, uint16 events );
PUBLIC void app_StartJoinConfig(bool flag);
PUBLIC bool app_SendsSatusDate(void);
PUBLIC void app_UartSendMesg(APP_uartEventType  type);
PUBLIC bool  app_SendDveInf(void);



#endif /* APP_UART_H_ */
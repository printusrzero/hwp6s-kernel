/************************************************************************
  Copyright   : 2005-2007, Huawei Tech. Co., Ltd.
  File name   : AtParseCmd.h
  Author      : ---
  Version     : V200R001
  Date        : 2005-04-19
  Description : ��ͷ�ļ�������---
  History     :
  1. Date:2005-04-19
     Author: ����Ⱥ
     Modification:Create
  2.��    ��   : 2006��08��09��
    ��    ��   : ����Ƽj60010247
    �޸�����   : ���ⵥA32D03479����PC����ʵ��ʱ��#pragma pack(0)��#pragma pack()�ӱ��뿪��
  3.��    ��   : 2007��10��11��
    ��    ��   : x68770
    �޸�����   : ���ⵥA32D13027

************************************************************************/

#ifndef _ATPARSECMD_H_
#define _ATPARSECMD_H_


/*****************************************************************************
  1 ����ͷ�ļ�����
*****************************************************************************/
/*#include "ATCmdProc.h" */
#include "TafTypeDef.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

#pragma pack(4)
/*****************************************************************************
  2 �궨��
*****************************************************************************/

#define AT_CHECK_BASE_HEX                       (16)
#define AT_CHECK_BASE_OCT                       (8)
#define AT_CHECK_BASE_DEC                       (10)

/*******************************************************************************
  3 ö�ٶ���
*******************************************************************************/
typedef enum
{
    AT_NONE_STATE,                      /* ��ʼ״̬ */

        AT_B_CMD_NAME_STATE,            /* AT����������״̬ */
        AT_B_CMD_PARA_STATE,            /* AT�����������״̬ */

        AT_D_CMD_NAME_STATE,                /* AT D����������״̬ */
        AT_D_CMD_DIGIT_STATE,               /* AT D��������״̬ */
        AT_D_CMD_CHAR_STATE,                /* AT D������ĸ״̬ */
        AT_D_CMD_RIGHT_ARROW_STATE,         /* AT D�����Ҽ�ͷ״̬ */
        AT_D_CMD_DIALSTRING_STATE,          /* AT D������ַ���״̬ */
        AT_D_CMD_SEMICOLON_STATE,           /* AT D����ֺ�״̬ */
        AT_D_CMD_CHAR_G_STATE,              /* AT D�����ַ�G״̬ */
        AT_D_CMD_CHAR_I_STATE,              /* AT D�����ַ�I״̬ */
        AT_D_CMD_LEFT_QUOT_STATE,           /* AT D����������״̬ */
        AT_D_CMD_RIGHT_QUOT_STATE,          /* AT D����������״̬ */

        AT_DM_CMD_NAME_STATE,
        AT_DM_CMD_STAR_STATE,              /* AT D�����һ��*״̬ */
        AT_DM_CMD_WELL_STATE,               /* AT D�����ַ�#״̬ */
        AT_DM_CMD_NUM_STATE,               /* AT D�����ַ�#״̬ */

        AT_S_CMD_NAME_STATE,             /* AT S������״̬ */
        AT_S_CMD_SET_STATE,              /* AT S��������״̬ */
        AT_S_CMD_READ_STATE,             /* AT S�����ѯ״̬ */
        AT_S_CMD_TEST_STATE,             /* AT S�������״̬ */
        AT_S_CMD_PARA_STATE,             /* AT S�������״̬ */

        AT_E_CMD_NAME_STATE,               /* AT��չ������״̬ */
        AT_E_CMD_SET_STATE,                /* AT��չ����Ⱥ�״̬ */
        AT_E_CMD_TEST_STATE,               /* AT��չ�����ѯ����״̬ */
        AT_E_CMD_READ_STATE,               /* AT��չ������Բ���״̬ */
        AT_E_CMD_PARA_STATE,               /* AT��չ�������״̬ */
        AT_E_CMD_COLON_STATE,              /* AT��չ�����״̬ */
        AT_E_CMD_LEFT_QUOT_STATE,          /* AT��չ����������״̬ */
        AT_E_CMD_RIGHT_QUOT_STATE,         /* AT��չ����������״̬ */

        AT_PARA_LEFT_BRACKET_STATE,      /* ����ƥ��������״̬ */
        AT_PARA_NUM_STATE,               /* ����ƥ�����״̬ */
        AT_PARA_LETTER_STATE,            /* ����ƥ����ĸ״̬ */
        AT_PARA_NUM_COLON_STATE,         /* ����ƥ�����ֶ���״̬ */
        AT_PARA_NUM_SUB_STATE,           /* ����ƥ�䷶Χ����״̬ */
        AT_PARA_NUM_SUB_COLON_STATE,     /* ����ƥ�䷶Χ���ֶ���״̬ */
        AT_PARA_QUOT_COLON_STATE,        /* ����ƥ�����Ŷ���״̬ */
        AT_PARA_RIGHT_BRACKET_STATE,     /* ����ƥ��������״̬ */
        AT_PARA_SUB_STATE,               /* ����ƥ�䷶Χ״̬ */
        AT_PARA_LEFT_QUOT_STATE,         /* ����ƥ��������״̬ */
        AT_PARA_RIGHT_QUOT_STATE,        /* ����ƥ��������״̬ */
        AT_PARA_COLON_STATE,             /* ����ƥ�䶺��״̬ */
        AT_PARA_ZERO_STATE,              /* ����ƥ������0״̬ */
        AT_PARA_ZERO_SUB_STATE,          /* ����ƥ�䷶Χ����0״̬ */
        AT_PARA_HEX_STATE,               /* ����ƥ��ʮ������״̬, 0x��0X */
        AT_PARA_HEX_SUB_STATE,           /* ����ƥ�䷶Χʮ������״̬ */
        AT_PARA_HEX_NUM_STATE,           /* ����ƥʮ��������״̬ */
        AT_PARA_HEX_NUM_SUB_STATE,       /* ����ƥ�䷶Χʮ����������״̬ */
        AT_PARA_NO_QUOT_LETTER_STATE,    /* ����ƥ����˫���Ű�����ĸ״̬ */

        AT_W_CMD_F_STATE,            /* AT��������&״̬ */
        AT_W_CMD_NAME_STATE,            /* AT����������W״̬ */
        AT_W_CMD_PARA_STATE,            /* AT����������W״̬ */

        AT_BUTT_STATE                    /* ��Ч״̬ */
}AT_STATE_TYPE_ENUM;

/*****************************************************************************
  4 ȫ�ֱ�������
*****************************************************************************/

/*****************************************************************************
  5 ��Ϣͷ����
*****************************************************************************/
/*ģ���������士HEADER */

/*****************************************************************************
  6 ��Ϣ����
*****************************************************************************/


/*****************************************************************************
  7 STRUCT����
*****************************************************************************/
/* �ж���������ָ��*/
typedef TAF_UINT32 (*pATChkCharFuncType)(TAF_UINT8);

/* ��״̬�����ͣ�����ж����������ɹ�������뷵�ض�Ӧ����״̬*/
typedef struct
{
    pATChkCharFuncType  pFuncName;                      /*    �ж���������,����ɹ�������next_state*/
    AT_STATE_TYPE_ENUM next_state;                    /*    ��һ��״̬ */
}AT_SUB_STATE_STRU;

/* ��ǰ״̬�����ͣ������״̬���ڵ�ǰ״̬��������Ӧ����״̬��*/
typedef struct
{
    AT_STATE_TYPE_ENUM curr_state;                    /*��ǰ״̬*/
    AT_SUB_STATE_STRU  *pSubStateTab;             /*��Ӧ����̬��*/
}AT_MAIN_STATE_STRU;

/*****************************************************************************
  8 UNION����
*****************************************************************************/


/*****************************************************************************
  8 OTHERS����
*****************************************************************************/
AT_STATE_TYPE_ENUM At_FindNextSubState(AT_SUB_STATE_STRU *pSubStateTab,TAF_UINT8 ucInputChar);
AT_STATE_TYPE_ENUM At_FindNextMainState(AT_MAIN_STATE_STRU *pMainStateTab,TAF_UINT8 ucInputChar,AT_STATE_TYPE_ENUM InputState);
TAF_VOID At_RangeCopy(TAF_UINT8 *pucDst,TAF_UINT8 * pucBegain, TAF_UINT8 * pucEnd);
TAF_UINT32 At_RangeToU32(TAF_UINT8 * pucBegain, TAF_UINT8 * pucEnd);


/* ���ַ����е�ĳһ�ο�����ָ���� */
extern VOS_VOID atRangeCopy(VOS_UINT8 *pucDst,VOS_UINT8 * pucBegain, VOS_UINT8 * pucEnd);

/* ���ַ����е�ĳһ��ת���޷�������ֵ */
extern VOS_UINT32 atRangeToU32(VOS_UINT8 * pucBegain, VOS_UINT8 * pucEnd);

extern AT_STATE_TYPE_ENUM atFindNextSubState(AT_SUB_STATE_STRU *pSubStateTab,VOS_UINT8 ucInputChar);
extern AT_STATE_TYPE_ENUM atFindNextMainState(AT_MAIN_STATE_STRU *pMainStateTab,VOS_UINT8 ucInputChar,AT_STATE_TYPE_ENUM InputState);
extern VOS_UINT32 atAuc2ul(VOS_UINT8 *nptr,VOS_UINT16 usLen,VOS_UINT32 *pRtn);


#if ((TAF_OS_VER == TAF_WIN32) || (TAF_OS_VER == TAF_NUCLEUS))
#pragma pack()
#else
#pragma pack(0)
#endif

#ifdef __cplusplus
    #if __cplusplus
        }
    #endif
#endif

#endif /* end of MapsTemplate.h*/



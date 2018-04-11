/*
 * Array_list.c
 *
 *  Created on: 2018年3月23日
 *      Author: 1
 *      此文件实现线性表的实现
 */

//用数组实现线性表
#include "jendefs.h"
#include "app_CIE_uart.h"
#include "Array_list.h"


PUBLIC uint8 LocateElem(sarrayList *alist,uYcl ycl);
PUBLIC uint8 ArrayLength(sarrayList *alist);
sarrayList Galist;
//初始化顺序表：给出初始化长度
PUBLIC bool Array_init(sarrayList *alist,ElemType * arrayhead , uint8 totalsize ,uint8 num )
{

	alist->Array = arrayhead;
	alist->current_num=num;
	alist->size=totalsize;
    if(NULL==alist->Array)
        return FALSE;
    else
        return TRUE;
}

//删除顺序表
PUBLIC void DeleteArray(sarrayList  * alist)
{
	alist->Array = NULL;
	alist->current_num = 0;
	alist->size = 0;

}



//判断顺序列表为空
PUBLIC bool IsEmpty(sarrayList * alist)
{
    if(0==alist->current_num)
    {

        return TRUE;
    }
    else
    {

        return FALSE;
    }

}
PUBLIC uint8 test(void)
{
	return 1;
}


PUBLIC uint8 LocateElem(sarrayList *alist,uYcl ycl)
{
	uint8 i=0;
	uYcl tycl=ycl;
	uint8 pos=0;
	sEnddev_BasicInf tEnddev_BasicInf;
    for(i=0 ; i<ArrayLength(alist) ; i++)
    {
    	tEnddev_BasicInf = alist->Array[i];
    	if((tEnddev_BasicInf.ycl.sYCL.Mac == tycl.sYCL.Mac) && (tEnddev_BasicInf.ycl.sYCL.YCL_ID.u32YCL_ID == tycl.sYCL.YCL_ID.u32YCL_ID))
    	{
    		pos = i+1;
    	}

    }
    return pos;
}


//列表中有多少个元素
PUBLIC uint8 ArrayLength(sarrayList *alist)
{
    return alist->current_num;
}

//取出某个元素
PUBLIC bool GetElem(sarrayList* alist,uint8 index,ElemType *e)
{
    if((index>ArrayLength(alist))||(index==0))
    {
        return FALSE;
    }
    else
    {
        *e=alist->Array[index-1];
        return TRUE;
    }
}

//遍历顺序表，并打印
PUBLIC void PrintfArray(sarrayList *alist)
{
	uint8 i=0;

    for(i=0;i<alist->current_num;i++)
    {

    }

}

//添加某个元素
PUBLIC bool AddElem(sarrayList *alist,ElemType e)
{
	uint8 index=0;
     //如果顺序表存储空间已满，则需要重新分配内存
    if(alist->current_num==alist->size)
    {
    	return FALSE;//列表已满
    }
    index=ArrayLength(alist);
    alist->Array[index]=e;
    ++alist->current_num;
    return TRUE;
}

//删除某个元素
PUBLIC bool DelElem(sarrayList *alist,uint8 index)
{
	uint8 i=0;
    //先判断插入位置是否合法
    if((index==0)||(index>ArrayLength(alist)))
    {
        return FALSE;
    }
    for(i = index; i < ArrayLength(alist); i++)
    {
        alist->Array[i-1]=alist->Array[i];
    }
    --alist->current_num;

    return TRUE;
}

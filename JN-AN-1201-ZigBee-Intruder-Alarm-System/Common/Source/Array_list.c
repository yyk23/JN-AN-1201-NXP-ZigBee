/*
 * Array_list.c
 *
 *  Created on: 2018年3月23日
 *      Author: 1
 *      此文件实现线性表的实现
 */

//用数组实现线性表
#include "jendefs.h"
#include "Array_list.h"


//初始化顺序表：给出初始化长度
bool Array_init(sarrayList *alist,ElemType * arrayhead , uint8 totalsize )
{

	alist->Array = arrayhead;
	alist->current_num=0;
	alist->size=totalsize;
    if(NULL==alist->Array)
        return FALSE;
    else
        return TRUE;
}

//删除顺序表
void DeleteArray(sarrayList  * alist)
{
	alist->Array = NULL;
	alist->current_num = 0;
	alist->size = 0;

}



//判断顺序列表为空
bool IsEmpty(sarrayList * alist)
{
    if(0==alist->current_num)
    {
        printf("the arrayList is empty!\n");
        return TRUE;
    }
    else
    {
        printf("the arrayList is not empty!\n");
        return FALSE;
    }

}

//列表中有多少个元素
uint8 ArrayLength(sarrayList *alist)
{
    return alist->current_num;
}

//取出某个元素
bool GetElem(sarrayList* alist,uint8 index,ElemType *e)
{
    if((index>ArrayLength(alist))||(index==0))
    {
        printf("无效的地址!\n");
        return FALSE;
    }
    else
    {
        *e=alist->Array[index-1];
        return TRUE;
    }
}

//遍历顺序表，并打印
void PrintfArray(sarrayList *alist)
{
	uint8 i=0;
    printf("array elements are: ");
    for(i=0;i<alist->current_num;i++)
    {
        printf("%d\t",alist->Array[i]);
    }
    printf("\n");
}

//判断某个元素的位置
//通过YCL来判断元素的位置
uint8 LocateElem(sarrayList *alist,uYcl ycl)
{
	uint8 i=0;
    for(i=0;i<ArrayLength(alist);i++)
    {
    	if(memcmp(alist->Array[i].ycl,ycl,sizeof(uYcl))==0)//内存比较函数
    	{
    		return i+1;
    	}
    }
    return 255;//代表返回错误
}

//添加某个元素
bool AddElem(sarrayList *alist,ElemType e)
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
bool DelElem(sarrayList *alist,uint8 index)
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

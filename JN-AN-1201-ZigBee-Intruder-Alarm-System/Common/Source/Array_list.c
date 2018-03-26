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


sarrayList     arraylist;


//初始化顺序表：给出初始化长度
bool Array_init(ElemType * arrayhead , uint8 totalsize )
{

	arraylist->Array= arrayhead;
    arraylist->current_num=0;
    arraylist->size=totalsize;
    if(NULL==arraylist->Array)
        return FALSE;
    else
        return TRUE;
}
/*
//删除顺序表
void deleteArray(arrayList* arrLst)
{
    arrLst->length = 0;
    arrLst->size = 0;
    free(arrLst->Array);
    arrLst->Array = NULL;
}

//清空顺序表
void clearArray(arrayList *arrLst)
{
    arrLst->length = 0;
}

//判断是否为空
bool is_empty(arrayList *arrLst)
{
    if(0==arrLst->length)
    {
        printf("the arrayList is empty!\n");
        return true;
    }
    else
    {
        printf("the arrayList is not empty!\n");
        return false;
    }

}

//求有多少个元素
int arrayLength(arrayList *arrLst)
{
    return arrLst->length;
}
//取出某个元素
bool getElem(arrayList* arrLst,int index,ElemType *e)
{
    if(index<0||index>arrayLength(arrLst)-1)
    {
        printf("超出边界!\n");
        return false;
    }
    else
    {
        *e=arrLst->Array[index];
        return true;
    }
}

//遍历顺序表，并打印
void printfArray(arrayList *arrLst)
{
    printf("array elements are: ");
    for(int i=0;i<arrLst->length;i++)
    {
        printf("%d\t",arrayList->Array[i]);
    }
    printf("\n");
}

//判断某个元素的位置
int locateElem(arrayList *arrLst,ElemType e)
{
    for(int i=0;i<arrayLength(arrLst);i++)
    {
        if(e==arrLst->Array[i])
            return i;
    }
    return -1;
}

//求某个元素的前驱：如果没找到返回-2；如果是第一个元素。返回-1；否则返回前驱元素的下标
int preElement(arrayList *arrLst,ElemType e,ElemType *preElem)
{
    for(int i=0;i<arrayLength(arrLst);i++)
    {
        if(e==arrLst->Array[i])
        {
            if(i==0)
            {
                return -1;
            }
            else
            {
                preElem=arrLst->Array[i-1];
                return i-1;
            }
        }
    }
    return -2;
}


//求某个元素的后继：如果没找到，返回-2,如果是最后一个元素，返回-1；否则返回后继元素的下标
int nextElement(arrayList *arrLst,ElemType e,ElemType *nxtElem)
{
    for(int i = 0; i < arrayLength(arrLst); ++i)
    {
        if(e == arrLst->Array[i])
        {
            if(arrayLength(arrLst) -1 == i)
            {
                return -1;
            }
            else
            {
                *nxtElem = arrLst->Array[i+1];
                return i+1;
            }
        }
    }
    return -2;
}

//将元素插入到指定位置
bool insertElem(arrayList *arrLst,int index,ElemType e)
{
    //先判断插入位置是否合法
    if(index<0||index>arrayLength(arrLst)-1)
    {
        return false;
    }
     //如果顺序表存储空间已满，则需要重新分配内存
     if(arrLst->length==arrLst->size)
     {
        arrLst->Array=(ElemType*)reolloc(arrLst->Array,2*arrLst->size*sizeof(ElemType));
        if(NULL==arrLst->Array)
            return false;//分配空间失败
        else
        {
            arrLst->size*=2;
        }
     }
    for(int i = index; i < arrayLength(arrLst); ++i)
    {
        arrLst->Array[i+1]=arrLst->Array[i];
    }
    arrLst->Array[index]=e;
    ++arrLst->length;

    return true;
}

//删除某个元素
bool deleteElem(arrayList *arrLst,int index ,ElemType *e)
{
    //先判断插入位置是否合法
    if(index<0||index>arrayLength(arrLst)-1)
    {
        return false;
    }
    *e=array->Array[index];
    for(int i = index; i < arrayLength(arrLst); ++i)
    {
        arrLst->Array[i]=arrLst->Array[i+1];
    }

    --arrLst->length;

    return true;
}

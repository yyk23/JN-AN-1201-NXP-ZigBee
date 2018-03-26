/*
 * Array_list.h
 *
 *  Created on: 2018年3月23日
 *      Author: 1
 *
 */

#ifndef ARRAY_LIST_H_
#define ARRAY_LIST_H_


#include "app_CIE_uart.h"

//配置部分
typedef sEnddev_BasicInf ElemType;

typedef struct {
    ElemType *Array;//实际存放元素的数组
    uint8 current_num;//现有的元素个数
    uint8 size;      //数组的最大容量
}sarrayList;


#endif /* ARRAY_LIST_H_ */

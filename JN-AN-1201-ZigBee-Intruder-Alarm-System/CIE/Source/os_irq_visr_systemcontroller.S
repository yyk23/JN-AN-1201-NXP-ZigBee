#==============================================================================
#
#                 THIS IS A GENERATED FILE. DO NOT EDIT!
#
# MODULE:         OS
#
# COMPONENT:      os_irq_IllegalInstruction.s
#
# DATE:           Thu Mar 22 16:25:52 2018
#
# AUTHOR:         Jennic RTOS Configuration Tool
#
# DESCRIPTION:    Wrapper for uncontrolled interrupt 'vISR_SystemController'. Calls a 
#                 C Function to handle interrupts "SystemController".
#
#==============================================================================
#
# This software is owned by Jennic and/or its supplier and is protected
# under applicable copyright laws. All rights are reserved. We grant You,
# and any third parties, a license to use this software solely and
# exclusively on Jennic products. You, and any third parties must reproduce
# the copyright and warranty notice and any other legend of ownership on each
# copy or partial copy of the software.
#
# THIS SOFTWARE IS PROVIDED "AS IS". JENNIC MAKES NO WARRANTIES, WHETHER
# EXPRESS, IMPLIED OR STATUTORY, INCLUDING, BUT NOT LIMITED TO, IMPLIED
# WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE,
# ACCURACY OR LACK OF NEGLIGENCE. JENNIC SHALL NOT, IN ANY CIRCUMSTANCES,
# BE LIABLE FOR ANY DAMAGES, INCLUDING, BUT NOT LIMITED TO, SPECIAL,
# INCIDENTAL OR CONSEQUENTIAL DAMAGES FOR ANY REASON WHATSOEVER.
#
# Copyright Jennic Ltd. 2009 All rights reserved
#
#==============================================================================

    .globl IRQ_vISR_SystemController
    .extern os_ExternalException_R
    .extern os_CFunc_W
IRQ_vISR_SystemController:
    .extern os_vISR_SystemController
    b.addi r1,r1,-8
    b.sw   0(r1),r9
    b.sw   4(r1),r5
    b.ori  r5,r0,os_vvISR_SystemController
    b.jal  os_CFunc_W
    b.lwz  r9,0(r1)
    b.lwz  r5,4(r1)
    b.addi r1,r1,8
    b.ja   os_ExternalException_R

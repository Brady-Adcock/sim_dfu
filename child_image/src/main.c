/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

 #include <stdio.h>
 #include <zephyr/kernel.h>
 
 #define SLEEP_TIME_MS   2000
 
 int main(void)
 { 
     while (1) {
         printk("hello world from net core!\n");
         k_msleep(SLEEP_TIME_MS);
     }
     printk("goodbye world from net core!\n");
     return 0;
 }
 
/* 
 * File:   delay.c
 * Author: 164
 *
 * Created on July 25, 2019, 11:13 AM
 */

#include <stdio.h>
#include <stdlib.h>
#include "delay.h"

/*
 * 
 */
void delay(int n){ 
    n = n * 500;
    while(n>0) {n--;}
}


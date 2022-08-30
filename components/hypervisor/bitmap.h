/*
 * Copyright (c) 2006-2022, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2022-08-30     Suqier       the first version
 */

#ifndef __BITMAP_H__
#define __BITMAP_H__

unsigned char one[8]  = {0b00000001, 0b00000010, 0b00000100, 0b00001000,
                         0b00010000, 0b00100000, 0b01000000, 0b10000000};
unsigned char zero[8] = {0b11111110, 0b11111101, 0b11111011, 0b11110111,
                         0b11101111, 0b11011111, 0b10111111, 0b01111111};

void bitmap_init(unsigned char *bitmap, int size);
void bitmap_set_bit(unsigned char *bitmap, int size, int index);
void bitmap_clr_bit(unsigned char *bitmap, int size, int index);
int  bitmap_get_bit(unsigned char *bitmap, int size, int index);

#endif  /* __BITMAP_H__ */

#ifndef __WULIAO_H
#define __WULIAO_H

#include "stm32f10x.h"


extern int zhua1;
extern int zhua2;
extern int zhua3;
extern int fang2;
extern int fang1;
extern int fang3;

extern int zhua;
extern int fang;
extern int guiwei;

void wait(void);
void zhou(int angle);
void jiazi(int angle);

void wuliao3(void);
void wuliao2(void);
void wuliao1(void);

void fwuliao3(void);
void fwuliao2(void);
void fwuliao1(void);

void fnwuliao3(void);
void fnwuliao2(void);
void fnwuliao1(void);

void maduo3(void);
void maduo2(void);
void maduo_z(int WL);
void maduo1(void);

void dingweisheng(void);
void dingweijiang(void);

void zhuanpanhui(void);
void zhuanpan0(void);
void zhuazikai(void);

#endif

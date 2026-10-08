#ifndef RED_ALIGN_TEST_H
#define RED_ALIGN_TEST_H

/* 1: startup red alignment/down/hold test; 0: original task sequence. */
#define RED_ALIGN_TEST_ENABLE 0
#define RED_ALIGN_TEST_DOWN_PULSES 5400UL
#define RED_ALIGN_TEST_SPEED_RPM 500U
#define RED_ALIGN_TEST_ACCEL 200U

void RedAlignTest_Run(void);
int RedAlignTest_Display(void);

#endif

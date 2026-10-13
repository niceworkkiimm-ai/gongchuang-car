"""Compile production dir13 against deterministic motor/yaw mocks, not hardware."""
import argparse
from pathlib import Path
import re
import subprocess
import tempfile

PRE = r'''
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <setjmp.h>
static uint32_t now_ms, stage_ms;
static unsigned scenario, commands, stops, armed;
static float initial_yaw;
static jmp_buf halt;
static volatile uint32_t gyro_yaw_frame_count;
static volatile float anglea;
static volatile uint8_t wheel_reply_done_mask, wheel_reply_error_code;
struct Cmd {char kind;unsigned addr,dir,vel,acc,pulses,relative,sync;};
static struct Cmd command[8];
static uint32_t CameraDebug_NowMs(void){return now_ms;}
static void delay_ms(unsigned ms){now_ms+=ms;}
static void WheelReply_End(void){armed=0;}
static void WheelReply_Begin(void){wheel_reply_done_mask=wheel_reply_error_code=0;armed=0;stage_ms=now_ms;}
static void WheelReply_Arm(unsigned a){armed|=1U<<(a-1);}
static void Emm_V5_Stop_Now(unsigned a,unsigned sync){assert(a>=1&&a<=4&&!sync);++stops;}
static void put(char kind,unsigned a,unsigned d,unsigned v,unsigned acc,unsigned p,unsigned rel,unsigned sync){
 struct Cmd *c; assert(commands<8); c=&command[commands++];
 c->kind=kind;c->addr=a;c->dir=d;c->vel=v;c->acc=acc;c->pulses=p;c->relative=rel;c->sync=sync;
 if(commands>4){assert(armed&(1U<<(a-1)));assert(kind=='P'&&acc==0);}
}
static void Emm_V5_Pos_Control(unsigned a,unsigned d,unsigned v,unsigned acc,unsigned p,unsigned rel,unsigned sync){put('P',a,d,v,acc,p,rel,sync);}
static void Emm_V5_Vel_Control(unsigned a,unsigned d,unsigned v,unsigned acc,unsigned sync){put('V',a,d,v,acc,0,0,sync);}
static void Gyro_DisplayPoll(void);
'''

MOCK = r'''
static void Gyro_DisplayPoll(void){
 if(car13_phase==0xFF){assert(stops==4);longjmp(halt,1);}
 if(scenario!=1)++gyro_yaw_frame_count;
 if(car13_phase==1&&scenario!=1){
   anglea+=(scenario==2?1.0f:-1.0f);
   if(anglea>180)anglea-=360;
   if(anglea< -180)anglea+=360;
   /* An old inner-wheel FD event must not finish the combined segment. */
   wheel_reply_done_mask|=5;
 }
 if(car13_phase==2){
   assert(commands==8);wheel_reply_done_mask=7;
   if(scenario==4)wheel_reply_error_code=0xE2;
   if(scenario!=3&&(uint32_t)(now_ms-stage_ms)>=80)wheel_reply_done_mask=15;
 }
}
static void move(int dir,int speed,int location){
'''

POST = r'''
}
static void reset(unsigned sc,float yaw,uint32_t time){
 scenario=sc;initial_yaw=anglea=yaw;now_ms=time;commands=stops=armed=0;
 gyro_yaw_frame_count=1;wheel_reply_done_mask=wheel_reply_error_code=0;
 car13_phase=car13_error=0;
}
int main(void){
 unsigned i,sc;float delta;
 for(i=0;i<3;++i){
   reset(0,i==0?0:i==1? -150:170,i==2?UINT32_MAX-100:0);
   if(setjmp(halt))assert(!"unexpected fault");
   move(13,500,29002);
   assert(commands==8&&!stops&&!car13_error&&car13_phase==3&&!armed);
   delta=anglea-initial_yaw;if(delta>180)delta-=360;if(delta< -180)delta+=360;
   assert(delta<=-90&&delta>=-91);
   assert(command[0].kind=='P'&&command[0].addr==1&&command[0].pulses==37902);
   assert(command[1].kind=='V'&&command[1].addr==2);
   assert(command[2].kind=='P'&&command[2].addr==3&&command[2].pulses==37902);
   assert(command[3].kind=='V'&&command[3].addr==4);
   for(sc=0;sc<8;++sc){
     assert(command[sc].vel==500&&!command[sc].sync&&!command[sc].relative);
     assert(command[sc].dir==(command[sc].addr==1||command[sc].addr==3));
     assert(command[sc].acc==(sc<4?150:0));
     if(sc>=4)assert(command[sc].pulses==13004);
   }
   assert((uint32_t)(now_ms-stage_ms)>=80); /* final wheel cannot be skipped */
 }
 for(sc=1;sc<=4;++sc){
   reset(sc,0,0);
   if(!setjmp(halt)){move(13,500,29002);assert(!"fault ignored");}
   assert(car13_phase==255&&stops==4&&!armed);
   assert(car13_error==(sc==1?2:sc==2?3:sc==3?4:5));
   if(sc<=2)assert(commands==4);else assert(commands==8);
 }
 reset(0,0,0);gyro_yaw_frame_count=0;
 if(!setjmp(halt)){move(13,500,29002);assert(!"missing initial gyro accepted");}
 assert(car13_error==1&&!commands&&stops==4);
 reset(0,0,0);
 if(!setjmp(halt)){move(13,0,29002);assert(!"zero speed accepted");}
 assert(car13_error==1&&!commands&&stops==4);
 puts("PASS: continuous outer motion; yaw-triggered exit; no intermediate stop; wraparound; four-wheel final completion; gyro/timeout/motor errors");
 return 0;
}
'''


def main():
    p = argparse.ArgumentParser()
    p.add_argument('--source-root', type=Path, default=Path(__file__).resolve().parents[2]/'stm32')
    p.add_argument('--cc', default='gcc')
    args = p.parse_args()
    read = lambda name: (args.source_root/name).read_bytes().decode('gb18030')
    src, header = read('BSP/car.c'), read('BSP/car.h')
    constants = '\n'.join(re.findall(r'^#define CAR_(?:ARC|DIR13)[^\n]+', header, re.M))
    helper = src[src.index('volatile uint8_t car13_phase'):src.index('void car(')]
    block = src[src.index('    if (dir == 13)'):src.index('    car_debug_active = 0;')]
    with tempfile.TemporaryDirectory(prefix='dir13-') as temp:
        c, exe = Path(temp)/'check.c', Path(temp)/'check.exe'
        c.write_text(PRE+constants+'\n'+helper+MOCK+block+POST, encoding='utf-8')
        subprocess.run([args.cc, '-std=c99', '-Wall', '-Wextra', '-Werror', str(c), '-o', str(exe)], check=True)
        subprocess.run([str(exe)], check=True, timeout=15)
    task = read('BSP/task.c')
    body = task[task.index('void yuanliao()'):task.index('void cujiagong()')]
    calls = re.findall(r'car\((\d+),([^;]+)\);', re.sub(r'/\*.*?\*/|//[^\n]*', '', body, flags=re.S))
    i = next(i for i, (mode, _) in enumerate(calls) if mode == '13')
    assert calls[i+1][0] == '10'  # no old car(0) or duplicate exit straight
    print('PASS: only the first route pair uses dir13; old correction/exit straight not duplicated')


if __name__ == '__main__':
    main()

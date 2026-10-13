"""Production dir13: uniform velocity stages, measured distance, bounded errors.

Deterministic driver/yaw mocks verify code ordering and math, not real mechanics.
"""
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
#define false 0
#define S_CPOS 9
static uint32_t now_ms, stage_ms;
static unsigned scenario, commands, stops, turn_stops, armed, reads;
static float initial_yaw;
static int64_t model_position, origin_position;
static jmp_buf halt;
static volatile uint32_t gyro_yaw_frame_count;
static volatile float anglea;
static volatile uint8_t wheel_reply_done_mask, wheel_reply_error_code;
static volatile uint8_t rxCmd[64],rxCount,rxFrameFlag;
extern volatile uint8_t car13_phase;
struct Cmd {char kind;unsigned addr,dir,vel,acc,pulses,relative,sync;};
static struct Cmd command[12];
static uint32_t __get_PRIMASK(void){return 0;}
static void __disable_irq(void){}
static void __set_PRIMASK(uint32_t v){(void)v;}
static uint32_t CameraDebug_NowMs(void){return now_ms;}
static void delay_ms(unsigned ms){now_ms+=ms;}
static void WheelReply_End(void){armed=0;}
static void WheelReply_Begin(void){wheel_reply_done_mask=wheel_reply_error_code=0;armed=0;stage_ms=now_ms;}
static void WheelReply_Arm(unsigned a){armed|=1U<<(a-1);}
static void Emm_V5_Stop_Now(unsigned a,unsigned sync){assert(a>=1&&a<=4&&!sync);++stops;if(car13_phase==2)++turn_stops;}
static void put(char kind,unsigned a,unsigned d,unsigned v,unsigned acc,unsigned p,unsigned rel,unsigned sync){
 struct Cmd *c; assert(commands<12); c=&command[commands++];
 c->kind=kind;c->addr=a;c->dir=d;c->vel=v;c->acc=acc;c->pulses=p;c->relative=rel;c->sync=sync;
 if(kind=='P'){assert(car13_phase==3&&turn_stops==4);assert(armed&(1U<<(a-1)));}
 if(commands==5){int64_t delta=model_position-origin_position;
   if(delta<0)delta=-delta;
   assert((uint64_t)delta*3200>=29002ULL*65536);
   assert((uint64_t)delta*3200<29203ULL*65536);
 }
}
static void Emm_V5_Pos_Control(unsigned a,unsigned d,unsigned v,unsigned acc,unsigned p,unsigned rel,unsigned sync){put('P',a,d,v,acc,p,rel,sync);}
static void Emm_V5_Vel_Control(unsigned a,unsigned d,unsigned v,unsigned acc,unsigned sync){put('V',a,d,v,acc,0,0,sync);}
static void Emm_V5_Read_Sys_Params(unsigned a,unsigned kind){
 uint32_t value;unsigned i=4;
 assert(a==1&&kind==S_CPOS);++reads;
 if(scenario==6||(scenario==7&&reads>1))return;
 if(commands)model_position-=4096; /* 200 serial pulses between samples */
 value=(uint32_t)(model_position<0?-model_position:model_position);
 /* An unrelated FD frame preceding the 36 response must not mask it. */
 rxCmd[0]=4;rxCmd[1]=0xFD;rxCmd[2]=0x9F;rxCmd[3]=0x6B;
 rxCmd[i]=scenario==8?2:1;rxCmd[i+1]=0x36;rxCmd[i+2]=model_position<0;
 rxCmd[i+3]=(uint8_t)(value>>24);rxCmd[i+4]=(uint8_t)(value>>16);
 rxCmd[i+5]=(uint8_t)(value>>8);rxCmd[i+6]=(uint8_t)value;rxCmd[i+7]=scenario==9?0:0x6B;
 rxCount=12;rxFrameFlag=1;
}
static void Gyro_DisplayPoll(void);
'''

MOCK = r'''
static void Gyro_DisplayPoll(void){
 if(car13_phase==0xFF){assert(stops==4+turn_stops);longjmp(halt,1);}
 if(scenario!=1)++gyro_yaw_frame_count;
 if(car13_phase==2&&scenario!=1){
   anglea+=(scenario==2?1.0f:-1.0f);
   if(anglea>180)anglea-=360;
   if(anglea< -180)anglea+=360;
 }
 if(car13_phase==3){
   assert(commands==12);wheel_reply_done_mask=7;
   if(scenario==4)wheel_reply_error_code=0xE2;
   if(scenario!=3&&(uint32_t)(now_ms-stage_ms)>=80)wheel_reply_done_mask=15;
 }
}
static void move(int dir,int speed,int location){
'''

POST = r'''
}
static void reset(unsigned sc,float yaw,uint32_t time,int64_t position){
 scenario=sc;initial_yaw=anglea=yaw;now_ms=time;commands=stops=turn_stops=armed=reads=0;
 model_position=origin_position=position;rxCount=rxFrameFlag=0;
 gyro_yaw_frame_count=1;wheel_reply_done_mask=wheel_reply_error_code=0;
 car13_phase=car13_error=0;
}
int main(void){
 unsigned i,sc;float delta;
 for(i=0;i<3;++i){
   reset(0,i==0?0:i==1?-150:170,i==2?UINT32_MAX-100:0,i==0?10000:i==1?-10000:2147484000LL);
   if(setjmp(halt))assert(!"unexpected fault");
   move(13,500,29002);
   assert(commands==12&&stops==4&&!car13_error&&car13_phase==4&&!armed);
   delta=anglea-initial_yaw;if(delta>180)delta-=360;if(delta< -180)delta+=360;
   assert(delta<=-90&&delta>=-91);
   for(sc=0;sc<12;++sc){
     assert(!command[sc].sync&&!command[sc].relative);
     assert(command[sc].dir==(command[sc].addr==1||command[sc].addr==3));
     if(sc<8)assert(command[sc].kind=='V');else assert(command[sc].kind=='P');
     assert(command[sc].acc==(sc>=4&&sc<8?235:150));
     assert(command[sc].vel==(sc==4||sc==5?123:500));
     if(sc>=8)assert(command[sc].pulses==13004);
   }
   assert((uint32_t)(now_ms-stage_ms)>=80); /* last wheel must finish */
 }
 for(sc=1;sc<=9;++sc){
   if(sc==5)continue;
   reset(sc,0,0,10000);
   if(!setjmp(halt)){move(13,500,29002);assert(!"fault ignored");}
   assert(car13_phase==255&&stops==4+turn_stops&&!armed);
   assert(car13_error==(sc==1?2:sc==2?3:sc==3?4:sc==4?5:6));
   if(sc>=6&&sc!=7)assert(!commands);
   if(sc==7)assert(commands==4);
 }
 reset(0,0,0,10000);gyro_yaw_frame_count=0;
 if(!setjmp(halt)){move(13,500,29002);assert(!"missing gyro accepted");}
 assert(car13_error==1&&!commands&&stops==4);
 reset(0,0,0,10000);
 if(!setjmp(halt)){move(13,0,29002);assert(!"zero speed accepted");}
 assert(car13_error==1&&!commands&&stops==4);
 puts("PASS: four velocity wheels, measured straight incl. signed/large positions, turn ratio, wraparound, preserved stop, gentle final start, all-wheel completion and bounded failures");
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
    with tempfile.TemporaryDirectory(prefix='dir13-uniform-') as temp:
        c, exe = Path(temp)/'check.c', Path(temp)/'check.exe'
        c.write_text(PRE+constants+'\n'+helper+MOCK+block+POST, encoding='utf-8')
        subprocess.run([args.cc, '-std=c99', '-Wall', '-Wextra', '-Werror', str(c), '-o', str(exe)], check=True)
        subprocess.run([str(exe)], check=True, timeout=15)
    task = read('BSP/task.c')
    body = task[task.index('void yuanliao()'):task.index('void cujiagong()')]
    calls = re.findall(r'car\((\d+),([^;]+)\);', re.sub(r'/\*.*?\*/|//[^\n]*', '', body, flags=re.S))
    i = next(i for i, (mode, _) in enumerate(calls) if mode == '13')
    assert calls[i+1][0] == '10'
    print('PASS: no duplicate exit straight/correction in task')


if __name__ == '__main__':
    main()

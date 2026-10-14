"""Exercise the production car(0) controller with wheel lag and UART feedback."""
import argparse
from pathlib import Path
import re
import subprocess
import tempfile

HARNESS = r'''
#include <assert.h>
#include <stdint.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
volatile float global_angle, angular_velocity_z;
volatile uint32_t gyro_yaw_frame_count;
static uint32_t irq_mask;
static uint64_t sim_time;
static double physical_yaw, physical_rate, lag_ms;
static int wheel[4], feedback, plant_moves, freeze_at_ms, cmd_count, reversal_count;
static int last_direction, speed_seen[11];
static double rate_before_reversal, max_rate_before_reversal;
static unsigned stationary_frame_count;
static int noise_enabled;
static double recent_errors[256], recent_rates[256];
static uint64_t recent_times[256];
static unsigned recent_count;
static float target_for_history;
static double wrap(double x) { while(x>180)x-=360;while(x<-180)x+=360;return x; }
static uint32_t __get_PRIMASK(void){return irq_mask;}
static void __disable_irq(void){irq_mask=1;}
static void __set_PRIMASK(uint32_t m){irq_mask=m;}
static uint32_t CameraDebug_NowMs(void){return (uint32_t)sim_time;}
static void advance(unsigned ms){
 unsigned j;
 for(j=0;j<ms;++j){
  double desired=plant_moves?(wheel[0]+wheel[1]+wheel[2]+wheel[3]):0;
  physical_rate+=(desired-physical_rate)/lag_ms;
  physical_yaw=wrap(physical_yaw+physical_rate*.001);
  ++sim_time;
  if(sim_time%5==0 && feedback && (!freeze_at_ms || sim_time<(unsigned)freeze_at_ms)){
   double noise=noise_enabled?((sim_time/5)%2?.015:-.015):0;
   global_angle=(float)wrap(physical_yaw+noise);
   angular_velocity_z=(float)physical_rate;
   ++gyro_yaw_frame_count;++stationary_frame_count;
   recent_times[recent_count%256]=sim_time;
   recent_errors[recent_count%256]=wrap(target_for_history-global_angle);
   recent_rates[recent_count%256]=physical_rate;++recent_count;
  }
 }
}
static void delay_ms(int32_t ms){assert(ms>=0);advance((unsigned)ms);}
static void Gyro_DisplayPoll(void){}
#define GLOG_STOP_SENT 5
#define GLOG_VELOCITY_SENT 3
#define GLOG_HEADING_WINDOW 4
static void GyroLog_Mark(int e,int m,int d){(void)e;(void)m;(void)d;}
static void Emm_V5_Stop_Now(uint8_t addr,int sync){
 assert(addr>=1&&addr<=4&&!sync);advance(1);wheel[addr-1]=0;
}
static void Emm_V5_Vel_Control(uint8_t addr,uint8_t dir,uint16_t rpm,uint8_t acc,int sync){
 unsigned i;assert(addr>=1&&addr<=4&&dir<=1&&rpm<=10&&acc==10&&!sync);
 if(addr==1 && last_direction>=0 && dir!=last_direction){
  for(i=0;i<4;++i)assert(wheel[i]==0);
  rate_before_reversal=fabs(physical_rate);
  if(rate_before_reversal>max_rate_before_reversal)max_rate_before_reversal=rate_before_reversal;
  assert(rate_before_reversal<=1.5);++reversal_count;
 }
 if(addr==1)last_direction=dir;
 speed_seen[rpm]=1;++cmd_count;advance(1);wheel[addr-1]=(dir?1:-1)*rpm;
}
/* STATUS */
/* PRODUCTION */
static void reset(double yaw,double rate,double lag,uint64_t time){
 physical_yaw=yaw;physical_rate=rate;lag_ms=lag;sim_time=time;
 global_angle=(float)yaw;angular_velocity_z=(float)rate;gyro_yaw_frame_count=1;
 memset(wheel,0,sizeof wheel);memset(speed_seen,0,sizeof speed_seen);
 feedback=plant_moves=1;freeze_at_ms=cmd_count=reversal_count=noise_enabled=0;
 stationary_frame_count=recent_count=0;last_direction=-1;max_rate_before_reversal=0;
 irq_mask=0;car_heading_status=CAR_HEADING_IDLE;
}
static void success(double yaw,double target,double initial_rate,double lag,uint64_t time){
 unsigned i,count;uint64_t began;
 reset(yaw,initial_rate,lag,time);target_for_history=(float)target;
 noise_enabled=1;began=sim_time;
 assert(Car_AlignHeading((float)target)==1);
 assert(car_heading_status==CAR_HEADING_SUCCESS);
 assert(fabs(wrap(target-global_angle))<=CAR_HEADING_TOLERANCE_DEG+.001);
 assert(fabs(physical_rate)<=CAR_HEADING_STABLE_RATE_DPS+.01);
 for(i=0;i<4;++i)assert(wheel[i]==0);
 assert(sim_time-began<15000 && irq_mask==0);
 /* Final 120ms must contain live in-tolerance, low-rate samples. */
 count=recent_count<256?recent_count:256;
 for(i=0;i<count;++i) if(recent_times[i]+120>=sim_time){
  assert(fabs(recent_errors[i])<=CAR_HEADING_TOLERANCE_DEG+.001);
  assert(fabs(recent_rates[i])<=CAR_HEADING_STABLE_RATE_DPS+.01);
 }
 printf("PASS yaw %.3f -> %.0f lag %.0fms residual %.1f: %.3fdeg, %lums, cmds %d, reverse %d\n",
 yaw,target,lag,initial_rate,global_angle,(unsigned long)(sim_time-began),cmd_count,reversal_count);
}
int main(void){
 unsigned i;uint64_t began;
 success(52.3059,90,35,60,0);assert(speed_seen[10]&&speed_seen[3]&&speed_seen[1]);
 success(131.5887,90,-40,60,0);
 success(-137.1753,-180,-38,60,0);
 success(178.2257,180,0,60,0);
 success(179,-180,0,60,0);assert(!speed_seen[10]);
 success(-179,180,0,60,0);assert(!speed_seen[10]);
 success(.25,0,38,100,0);assert(cmd_count>0);
 success(-179.7,180,-30,100,0);
 success(121.3,90,-37,150,0);
 success(-90.36,-90,0,60,0);
 success(.39,0,0,60,0);assert(cmd_count>0);
 success(91.159,90,0,60,UINT32_MAX-150U);
 reset(0,0,60,0);feedback=0;target_for_history=90;began=sim_time;
 assert(!Car_AlignHeading(90)&&car_heading_status==CAR_HEADING_STALE);
 assert(sim_time-began<200&&cmd_count==0);
 reset(0,0,60,0);freeze_at_ms=400;target_for_history=90;
 assert(!Car_AlignHeading(90)&&car_heading_status==CAR_HEADING_STALE);
 for(i=0;i<4;++i)assert(wheel[i]==0);
 reset(0,0,60,0);plant_moves=0;target_for_history=90;began=sim_time;
 assert(!Car_AlignHeading(90)&&car_heading_status==CAR_HEADING_TIMEOUT);
 assert(sim_time-began>=15000&&sim_time-began<15100);
 for(i=0;i<4;++i)assert(wheel[i]==0);
 reset(0,0,60,0);global_angle=NAN;feedback=0;
 assert(!Car_AlignHeading(90)&&car_heading_status==CAR_HEADING_BAD_DATA);
 reset(0,0,60,0);feedback=0;
 assert(!Car_AlignHeading(181)&&car_heading_status==CAR_HEADING_BAD_DATA);
 puts("PASS: fresh stable arrival, inertia, shortest wrap, reversible fine correction, stale/timeout/bad-data stop");
 return 0;
}
'''

def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--source-root', type=Path, required=True)
    p.add_argument('--cc', default='gcc')
    args = p.parse_args()
    source = (args.source_root/'BSP/car.c').read_bytes().decode('latin1')
    header = (args.source_root/'BSP/car.h').read_bytes().decode('latin1')
    production = source[source.index('/* car(0) only:'):source.index('void car(')]
    statuses = '\n'.join(re.findall(r'^#define CAR_HEADING_\w+ \d+U\s*$', header, re.M))
    harness = HARNESS.replace('/* PRODUCTION */', production).replace('/* STATUS */', statuses)
    with tempfile.TemporaryDirectory(prefix='heading-settle-') as td:
        td = Path(td)
        c = td/'test.c'; exe = td/'test.exe'
        c.write_text(harness, encoding='ascii')
        subprocess.run([args.cc,'-std=c99','-O0','-Wall','-Wextra','-Werror',str(c),'-o',str(exe),'-lm'],check=True)
        subprocess.run([str(exe)],check=True,timeout=15)

if __name__ == '__main__':
    main()

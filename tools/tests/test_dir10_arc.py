"""Test production dir10 commands, feedback faults and route endpoint geometry.

Example: python tools/tests/test_dir10_arc.py --source-root <firmware> --cc <gcc>
The endpoint model assumes equal wheel scales, no slip and the existing 17800
pulse quarter-turn calibration; this is not a physical calibration test.
"""
import argparse
import math
from pathlib import Path
import re
import subprocess
import tempfile

PREAMBLE = r'''
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <setjmp.h>
static uint32_t clock_ms;
static jmp_buf fault_exit;
static unsigned scenario, launched, cmds, stops, polls[5], pre_polls[5];
struct Command { unsigned addr,dir,vel,acc,pulses,relative,sync; };
static struct Command sent[4];
static uint32_t __get_PRIMASK(void) { return 0; }
static void __disable_irq(void) { }
static void __set_PRIMASK(uint32_t x) { (void)x; }
static void delay_ms(unsigned ms) { clock_ms += ms; }
static uint32_t CameraDebug_NowMs(void) { return clock_ms; }
static void Gyro_DisplayPoll(void);
static void Emm_V5_Stop_Now(unsigned,unsigned);
static void Emm_V5_Read_Sys_Params(unsigned,unsigned);
static void Emm_V5_Pos_Control(unsigned,unsigned,unsigned,unsigned,unsigned,unsigned,unsigned);
static void Emm_V5_Synchronous_motion(unsigned);
#define S_FLAG 13
'''

HARNESS = r'''
static void Gyro_DisplayPoll(void)
{
    if (car_arc_fault) longjmp(fault_exit, 1);
}
static void Emm_V5_Stop_Now(unsigned addr, unsigned sync)
{
    assert(addr >= 1 && addr <= 4 && sync == 0);
    ++stops;
}
static void reply(unsigned addr,unsigned cmd,unsigned value)
{
    WheelStatus_Feed((uint8_t)addr); WheelStatus_Feed((uint8_t)cmd);
    WheelStatus_Feed((uint8_t)value); WheelStatus_Feed(0x6B);
}
static void Emm_V5_Read_Sys_Params(unsigned addr,unsigned kind)
{
    unsigned flags = 3;
    assert(kind == S_FLAG && addr >= 1 && addr <= 4);
    if ((scenario == 1 && !launched) || (scenario == 2 && launched)) return;
    if (!launched) {
        ++pre_polls[addr];
        if (scenario == 3) flags = 7; /* stalled */
        if (scenario == 4) flags = 2; /* disabled */
        if (scenario == 8 && pre_polls[addr] < 3) flags = 1;
    } else {
        ++polls[addr];
        if (scenario != 5 && polls[addr] < 4) flags = 1;
        if (scenario == 6 && addr == 4) flags = 1; /* last wheel never finishes */
        if (scenario == 7 && addr == 3) flags = 0x0D; /* stall protection */
    }
    /* FD arrival notifications and another address must not satisfy a query. */
    reply(addr, 0xFD, 0x9F);
    reply(addr == 4 ? 1 : 4, 0x3A, 3);
    assert(!wheel_status_ready);
    reply(addr, 0x3A, flags);
    assert(wheel_status_ready);
}
static void Emm_V5_Pos_Control(unsigned addr,unsigned dir,unsigned vel,unsigned acc,
                              unsigned pulses,unsigned relative,unsigned sync)
{
    struct Command *c;
    assert(cmds < 4 && !launched);
    if (scenario == 8) assert(pre_polls[addr] >= 3);
    c = &sent[cmds++];
    c->addr=addr; c->dir=dir; c->vel=vel; c->acc=acc;
    c->pulses=pulses; c->relative=relative; c->sync=sync;
}
static void Emm_V5_Synchronous_motion(unsigned addr)
{
    assert(addr == 0 && cmds == 4 && !launched);
    launched = 1;
}
static void reset(unsigned sc)
{
    scenario=sc; launched=cmds=stops=0; car_arc_fault=car_arc_fault_addr=0;
    memset(polls,0,sizeof polls); memset(pre_polls,0,sizeof pre_polls);
    WheelStatus_End(); clock_ms=0;
}
static void check_success(int speed)
{
    unsigned i, outer = (unsigned)(speed > 500 ? 500 : speed)/5*5;
    if (setjmp(fault_exit)) assert(!"unexpected arc fault");
    Car_Arc90(speed);
    assert(!car_arc_fault && cmds == 4 && launched && !stops);
    for (i=0; i<4; ++i) {
        unsigned inner = i == 0 || i == 2;
        struct Command c=sent[i];
        assert(c.addr == i+1 && c.dir == inner && c.sync == 1 && c.relative == 0);
        assert(c.pulses == (inner ? 8900U : 44500U));
        assert(c.vel == (inner ? outer/5 : outer));
        assert(c.acc == (inner ? 151U : 235U));
        assert(polls[i+1] >= 4);
    }
    /* Equal normalized travel time and equal acceleration/deceleration time. */
    assert(sent[0].pulses*sent[1].vel == sent[1].pulses*sent[0].vel);
    assert((256-sent[0].acc)*sent[0].vel == (256-sent[1].acc)*sent[1].vel);
}
int main(void)
{
    unsigned i;
    assert(CAR_ARC_90_TRIM_PULSES == 16998);
    /* Parser: incomplete, corrupt, wrong-address and back-to-back packets. */
    WheelStatus_Begin(2);
    reply(1,0x3A,3); reply(2,0xFD,0x9F); assert(!wheel_status_ready);
    WheelStatus_Feed(2); WheelStatus_Feed(0x3A); WheelStatus_Feed(3);
    assert(!wheel_status_ready); WheelStatus_Feed(0); assert(!wheel_status_ready);
    reply(2,0x3A,1); assert(wheel_status_ready && wheel_status_flags==1);
    reply(2,0x3A,3); assert(wheel_status_flags==1); /* first result frozen */
    WheelStatus_Begin(2); assert(!wheel_status_ready);
    reply(2,0x3A,0x6B); assert(wheel_status_ready && wheel_status_flags==0x6B);
    WheelStatus_End(); WheelStatus_Begin(0); reply(0,0x3A,3); assert(!wheel_status_ready);
    for (i=0; i<3; ++i) { reset(0); check_success(i==0 ? 500 : i==1 ? 498 : 1000); }
    reset(8); check_success(500); /* preceding straight must really finish */
    reset(0); clock_ms=UINT32_MAX-20; check_success(500); /* wrap-safe clock */
    for (i=1; i<=7; ++i) {
        reset(i);
        if (!setjmp(fault_exit)) { Car_Arc90(500); assert(!"fault was ignored"); }
        assert(stops==4 && car_arc_fault);
        assert(car_arc_fault == (i<=2 ? 1 : i<=4 || i==7 ? 2 : 3));
        if (i==1 || i==3 || i==4) assert(cmds==0 && !launched);
    }
    reset(0);
    if (!setjmp(fault_exit)) { Car_Arc90(0); assert(!"zero speed accepted"); }
    assert(car_arc_fault==4 && !cmds && stops==4);
    puts("PASS: production arc speed/direction/sync, four-wheel completion, status parser and faults");
    return 0;
}
'''


def function(text, name):
    m = re.search(r'void\s+'+name+r'\s*\([^)]*\)\s*\{', text)
    assert m, name
    depth, end = 1, m.end()
    while depth:
        depth += (text[end] == '{') - (text[end] == '}')
        end += 1
    return text[m.end():end-1]


def geometry(task):
    trim = 16998
    radius = 26700/(math.pi/2)
    specs = {'yuanliao':(2,6), 'zancunqu':(1,3), 'yuanliao2':(1,3),
             'zancunqu2':(1,3), 'qidian':(1,3), 'xingzou':(6,None)}
    max_error=0
    for name,(arc_count,limit) in specs.items():
        body=function(task,name)
        body=re.sub(r'/\*.*?\*/|//[^\n]*','',body,flags=re.S)
        calls=re.findall(r'car\((\d+),speed,([^;]+?)\);',body)
        if limit: calls=calls[:limit]
        assert sum(mode=='10' for mode,_ in calls)==arc_count,(name,calls)
        old=[]; new=[]
        for idx,(mode,expr) in enumerate(calls):
            mode=int(mode)
            assert re.fullmatch(r'[\d*\-A-Z_]+',expr)
            value=eval(expr.replace('CAR_ARC_90_TRIM_PULSES',str(trim)),{'__builtins__':{}})
            if mode==2:
                deductions=(idx>0 and calls[idx-1][0]=='10')+(idx+1<len(calls) and calls[idx+1][0]=='10')
                original=eval(expr.split('-')[0],{'__builtins__':{}})
                assert original-value==deductions*trim,(name,idx,expr)
                assert value>0
                old.append((mode,original)); new.append((mode,value))
            elif mode==10:
                assert idx>0 and idx+1<len(calls)
                assert calls[idx-1][0]==calls[idx+1][0]=='2'
                old.append((6,17800)); new.append((10,0))
            else:
                old.append((mode,value)); new.append((mode,value))
        def integrate(cmds):
            x=y=angle=0.0
            for mode,d in cmds:
                if mode in (1,2):
                    sign=1 if mode==2 else -1
                    x+=sign*d*math.cos(angle); y+=sign*d*math.sin(angle)
                elif mode==10:
                    x+=radius*(math.cos(angle)+math.sin(angle))
                    y+=radius*(math.sin(angle)-math.cos(angle))
                    angle-=math.pi/2
                elif mode in (5,6): angle+=(1 if mode==5 else -1)*math.pi/2
                # xingzou entry/exit diagonal calls are identical in both routes.
            return x,y,angle
        before,after=integrate(old),integrate(new)
        err=math.hypot(after[0]-before[0],after[1]-before[1])
        assert abs(after[2]-before[2])<1e-9 and err<2,(name,before,after)
        max_error=max(max_error,err)
        print(f'{name}: {arc_count} arcs, ideal endpoint rounding error {err:.6f} pulse-equivalent')
    print(f'PASS: route compensation, shared-straight double deduction; max error {max_error:.6f}')


def main():
    p=argparse.ArgumentParser()
    p.add_argument('--source-root',type=Path,default=Path(__file__).resolve().parents[2]/'stm32')
    p.add_argument('--cc',default='gcc')
    args=p.parse_args()
    def read(rel): return (args.source_root/rel).read_bytes().decode('gb18030')
    car,uart,header=read('BSP/car.c'),read('BSP/usart.c'),read('BSP/car.h')
    constants='\n'.join(re.findall(r'^#define CAR_ARC[^\n]+',header,re.M))
    parser=uart[uart.index('volatile uint8_t wheel_status_ready'):uart.index('void USART1_IRQHandler')]
    helper=car[car.index('volatile uint8_t car_arc_fault'):car.index('void car(')]
    with tempfile.TemporaryDirectory(prefix='dir10-test-') as temp:
        src=Path(temp)/'test.c'; exe=Path(temp)/'test.exe'
        src.write_text(PREAMBLE+constants+'\n'+parser+'\n'+helper+HARNESS,encoding='utf-8')
        subprocess.run([args.cc,'-std=c99','-O0','-Wall','-Wextra','-Werror',str(src),'-o',str(exe)],check=True)
        subprocess.run([str(exe)],check=True,timeout=15)
    geometry(read('BSP/task.c'))


if __name__=='__main__': main()

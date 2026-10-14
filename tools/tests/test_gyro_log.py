"""Host-test production logging, bounded retention and offline HEX/CSV decoding."""
import argparse
import importlib.util
import json
from pathlib import Path
import subprocess
import tempfile

STUB = r'''
#ifndef TEST_STM32_H
#define TEST_STM32_H
#include <stdint.h>
extern uint32_t SystemCoreClock, test_cycles, test_ctrl, test_mask;
struct TestCoreDebug { uint32_t DEMCR; };
extern struct TestCoreDebug test_core;
#define CoreDebug (&test_core)
#define CoreDebug_DEMCR_TRCENA_Msk (1UL << 24)
static uint32_t __get_PRIMASK(void) { return test_mask; }
static void __disable_irq(void) { test_mask = 1; }
static void __set_PRIMASK(uint32_t x) { test_mask = x; }
static void __DMB(void) {}
#endif
'''

HARNESS = r'''
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "stm32f10x.h"
uint32_t SystemCoreClock=72000000, test_cycles, test_ctrl, test_mask;
struct TestCoreDebug test_core;
volatile float global_angle, anglea, angular_velocity_z;
volatile uint32_t gyro_yaw_frame_count;
#define GLOG_DWT_CTRL test_ctrl
#define GLOG_DWT_CYCCNT test_cycles
#include "PRODUCTION"
#if GYRO_LOG_ENABLE
static void reset_all(uint32_t start) {
 memset((void*)&gyro_log,0,sizeof gyro_log);
 initialized=active=tail_pending=post_mask=completing=0;
 current_mode=-1;last_target=0;
 last_cycles=time_us=cycle_remainder=cycles_per_us=0;
 last_yaw_us=last_sample_us=end_us=complete_us=0;
 test_cycles=start;test_ctrl=0;test_mask=0;
 global_angle=anglea=angular_velocity_z=0;gyro_yaw_frame_count=0;
 GyroLog_Init();assert(test_cycles==start);assert(test_ctrl==1);
 assert(gyro_log.header.record_size==32);assert(gyro_log.header.header_size==64);
 assert(gyro_log.header.event_count==1);
}
static void advance(uint32_t us) { test_cycles += us*72U; }
static void snapshot(const char *path) {
 FILE *f=fopen(path,"wb");assert(f);
 assert(fwrite((const void*)&gyro_log,1,sizeof gyro_log,f)==sizeof gyro_log);
 assert(fclose(f)==0);
}
int main(int argc,char **argv) {
 unsigned i, count, next;uint32_t cycles,last;GyroLogRecord first;
 assert(argc==2);
 reset_all(0xFFFFFF00U);
 advance(5000);global_angle=12.345f;anglea=12.34f;angular_velocity_z=-0.75f;
 gyro_yaw_frame_count=1;GyroLog_OnYaw();
 assert(last_yaw_us==5000);assert(test_mask==0);
 GyroLog_CarBegin(0,90);
 advance(14999);GyroLog_Tick();assert(gyro_log.header.sample_count==0);
 advance(1);GyroLog_Tick();assert(gyro_log.header.sample_count==1);
 assert(gyro_log.samples[0].time_us==20000);
 assert(gyro_log.samples[0].last_yaw_us==5000);
 assert(gyro_log.samples[0].yaw_deg==global_angle);
 assert(gyro_log.samples[0].control_deg==anglea);
 assert(gyro_log.samples[0].rate_dps==angular_velocity_z);
 GyroLog_Mark(GLOG_HEADING_WINDOW,0,0);
 for(i=1;i<=4;i++){advance(10000);GyroLog_Mark(GLOG_STOP_SENT,(uint8_t)i,0);}
 GyroLog_CarEnd();assert(!active&&tail_pending);
 /* Starting the next translation must not manufacture an idle vehicle. */
 GyroLog_CarBegin(2,500);assert(last_target==90);
 advance(50000);GyroLog_Tick();assert(post_mask==1);
 assert(gyro_log.events[gyro_log.header.event_count-1].event==GLOG_POST_50MS);
 assert(gyro_log.events[gyro_log.header.event_count-1].mode==2);
 assert(gyro_log.events[gyro_log.header.event_count-1].direction&0x80U);
 advance(150000);GyroLog_Tick();assert(post_mask==3);
 advance(300000);GyroLog_Tick();assert(post_mask==7&&!tail_pending);
 count=gyro_log.header.sample_count;
 advance(199999);GyroLog_Tick();assert(gyro_log.header.sample_count==count);
 advance(1);GyroLog_Tick();assert(gyro_log.header.sample_count==count+1);
 /* Preserve a caller that already has interrupts masked. */
 test_mask=1;GyroLog_OnYaw();assert(test_mask==1);
 GyroLog_Mark(GLOG_STOP_SENT,4,0);assert(test_mask==1);
 GyroLog_CarBegin(0,-180);assert(test_mask==1);
 GyroLog_CarEnd();assert(test_mask==1);
 GyroLog_Tick();assert(test_mask==1);test_mask=0;
 /* Event overflow retains the entire beginning; sampling remains usable. */
 first=gyro_log.events[0];
 for(i=0;i<GYRO_LOG_EVENT_CAPACITY+8;i++)GyroLog_Mark(GLOG_STOP_SENT,1,1);
 assert(gyro_log.header.event_count==GYRO_LOG_EVENT_CAPACITY);
 assert(gyro_log.header.events_dropped>0);
 assert(!memcmp((const void*)&gyro_log.events[0],&first,sizeof first));
 reset_all(0);
 GyroLog_CarBegin(0,90);
 global_angle=89.98f;anglea=89.99f;gyro_yaw_frame_count=200;
 GyroLog_OnYaw();GyroLog_Mark(GLOG_HEADING_WINDOW,0,0);
 for(i=1;i<=4;i++) {advance(10000);GyroLog_Mark(GLOG_STOP_SENT,(uint8_t)i,0);}
 global_angle=90.12f;anglea=90.11f;GyroLog_CarEnd();
 assert(!(gyro_log.events[gyro_log.header.event_count-1].direction&0x80U));
 /* More than one complete ring: newest samples in chronological order. */
 GyroLog_CarBegin(0,-90);
 for(i=0;i<GYRO_LOG_SAMPLE_CAPACITY+19;i++){advance(20000);GyroLog_Tick();}
 assert(gyro_log.header.sample_count==GYRO_LOG_SAMPLE_CAPACITY);
 assert(gyro_log.header.sample_total==GYRO_LOG_SAMPLE_CAPACITY+19);
 next=gyro_log.header.sample_next;
 last=gyro_log.samples[next].time_us;
 for(i=1;i<GYRO_LOG_SAMPLE_CAPACITY;i++){
   uint32_t stamp=gyro_log.samples[(next+i)%GYRO_LOG_SAMPLE_CAPACITY].time_us;
   assert(stamp>last);last=stamp;
 }
 GyroLog_CarEnd();GyroLog_TaskComplete();
 count=gyro_log.header.event_count;GyroLog_TaskComplete();
 assert(gyro_log.header.event_count==count);
 advance(499999);GyroLog_Tick();assert(!gyro_log.header.frozen);
 advance(1);GyroLog_Tick();assert(gyro_log.header.frozen);
 assert(gyro_log.header.flags==1);
 snapshot(argv[1]);
 count=gyro_log.header.event_count;next=gyro_log.header.sample_next;cycles=last_cycles;
 advance(200000);global_angle=0;anglea=0;
 GyroLog_OnYaw();GyroLog_Tick();GyroLog_Mark(GLOG_STOP_SENT,1,0);
 GyroLog_CarBegin(0,0);GyroLog_CarEnd();GyroLog_TaskComplete();
 assert(gyro_log.header.event_count==count&&gyro_log.header.sample_next==next);
 assert(last_cycles==cycles);assert(test_mask==0);
 /* Microsecond counter wrap is separate from DWT counter wrap. */
 reset_all(0);time_us=0xFFFFF000U;last_sample_us=time_us;
 GyroLog_CarBegin(0,0);advance(20000);GyroLog_Tick();
 assert(gyro_log.header.sample_count==1);
 puts("PASS: production log clocks/wrap, raw floats, nested IRQ mask, retention/overflow, passive post-return samples, completion/freeze");
 return 0;
}
#else
int main(void) {
 int n=0;
 GyroLog_Init();GyroLog_OnYaw();GyroLog_Tick();
 GyroLog_CarBegin(++n,++n);GyroLog_Mark(++n,++n,++n);
 GyroLog_CarEnd();GyroLog_TaskComplete();assert(n==0);
 puts("PASS: disabled logging hooks compile away");return 0;
}
#endif
'''


def as_hex(blob, address=0x20000000):
    def row(address, kind, data):
        packet = bytes([len(data), address >> 8, address & 255, kind]) + data
        return ':' + (packet + bytes([-sum(packet) & 255])).hex().upper()
    lines = [row(0,4,bytes([address >> 24, (address >> 16) & 255]))]
    for i in range(0,len(blob),16):
        lines.append(row((address+i) & 65535,0,blob[i:i+16]))
    lines.append(row(0,1,b''))
    return '\n'.join(lines)+'\n'


def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--source-root',type=Path,required=True)
    p.add_argument('--cc',default='gcc')
    args=p.parse_args()
    root=Path(__file__).resolve().parents[2]
    spec=importlib.util.spec_from_file_location('gyro_decoder',root/'tools/gyro_log_decode.py')
    decoder=importlib.util.module_from_spec(spec);spec.loader.exec_module(decoder)
    with tempfile.TemporaryDirectory(prefix='gyro-log-test-') as tmp:
        tmp=Path(tmp);(tmp/'stm32f10x.h').write_text(STUB)
        c=tmp/'test.c'
        for name in ['gyro_log.c','gyro_log.h']:
            (tmp/name).write_bytes((args.source_root/'hardware'/name).read_bytes())
        c.write_text(HARNESS.replace('PRODUCTION','gyro_log.c'))
        dump=tmp/'snapshot.bin'
        for enabled in [1,0]:
            exe=tmp/f'test{enabled}.exe'
            subprocess.run([args.cc,'-std=c99','-O0','-Wall','-Wextra',
                '-Wno-unused-function','-Werror',f'-DGYRO_LOG_ENABLE={enabled}',
                '-I',str(tmp),str(c),'-o',str(exe)],check=True)
            subprocess.run([str(exe),str(dump)],check=True,timeout=10)
        blob=dump.read_bytes();assert len(blob)==49216
        hexfile=tmp/'snapshot.hex';hexfile.write_text(as_hex(blob))
        for path in [dump,hexfile]:
            meta,rows=decoder.export(path,tmp/(path.stem+path.suffix+'.csv'))
            assert meta['frozen']==1 and meta['samples_overwritten']>0
            assert all(a['time']<=b['time'] for a,b in zip(rows,rows[1:]))
            assert any(x['fields'][8]==4 for x in rows)
        damaged=as_hex(blob).replace(':020000042000DA',':020000042000DB')
        try:decoder.intel_hex(damaged)
        except ValueError:pass
        else:raise AssertionError('bad HEX checksum accepted')
        bad=tmp/'bad.bin';bad.write_bytes(blob[:128])
        try:decoder.read_snapshot(bad)
        except ValueError:pass
        else:raise AssertionError('truncated snapshot accepted')
    print('PASS: 49216-byte layout, circular order, binary/HEX decoding, CSV/summary, truncated/bad input rejection')


if __name__=='__main__':main()

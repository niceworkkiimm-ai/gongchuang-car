"""Check the primary project's actual TXE scan command and task-data preservation."""
import argparse
from pathlib import Path
import subprocess
import tempfile

PRE = r'''
#include <stdint.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>
#define ENABLE 1
#define DISABLE 0
#define USART_IT_TXE 0x80
static void *UART5;
static unsigned enabled,used;
static uint8_t wire[4096];
static void USART_ITConfig(void *port,unsigned kind,unsigned state) {
 assert(port==UART5&&kind==USART_IT_TXE);enabled=state;
}
static void USART_SendData(void *port,unsigned byte) {
 assert(port==UART5&&used<sizeof wire);wire[used++]=(uint8_t)byte;
}
'''

POST = r'''
static const char first[]="123+321+213+132",second[]="456+213+612+321";
static const uint8_t expected1[]="page0.t0.txt=\"123+321+\\r213+132\"\xFF\xFF\xFF";
static const uint8_t expected2[]="page0.t0.txt=\"456+213+\\r612+321\"\xFF\xFF\xFF";
static void reset_all(void) {
 memset(saoma_data,0,sizeof saoma_data);memset(scan_window,0,sizeof scan_window);
 scan_count=saoma_ready=0;hmi_pending=0;hmi_tx_index=HMI_FRAME_LENGTH;
 used=enabled=0;
}
static void feed(const char *s) {while(*s)Scanner_ReceiveByte((uint8_t)*s++);}
static void drain(void) {unsigned n=0;while(enabled){assert(++n<sizeof wire);HMI_TransmitByte();}}
int main(void) {
 unsigned offset;
 assert(HMI_FRAME_LENGTH==sizeof(expected1)-1);
 reset_all();feed(first);drain();
 assert(used==sizeof(expected1)-1&&!memcmp(wire,expected1,used));
 assert(saoma_ready&&strcmp((char*)saoma_data,first)==0);
 assert(!memchr(wire,'\r',used)&&!memchr(wire,'\n',used));
 /* A scan arriving during any active command must wait for its last FF. */
 for(offset=1;offset<=HMI_FRAME_LENGTH;offset++) {
   unsigned i;
   reset_all();feed(first);
   for(i=0;i<offset;i++)HMI_TransmitByte();
   feed(second);drain();
   assert(used==sizeof(expected1)+sizeof(expected2)-2);
   assert(!memcmp(wire,expected1,sizeof(expected1)-1));
   assert(!memcmp(wire+sizeof(expected1)-1,expected2,sizeof(expected2)-1));
   assert(strcmp((char*)saoma_data,first)==0);
 }
 puts("PASS: exact two-line command, literal backslash/r, buffer length, FFF tail, every active-command boundary, unchanged raw task scan");
 return 0;
}
'''


def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--source-root',type=Path,required=True)
    p.add_argument('--cc',default='gcc')
    args=p.parse_args()
    raw=(args.source_root/'hardware/uart_5.c').read_bytes()
    start=raw.index(b'volatile uint8_t saoma_ready')
    end=raw.index(b'volatile uint8_t uart5_RxData',start)
    screen=raw[start:end].decode('ascii')
    start=raw.index(b'static void Scanner_ReceiveByte(uint8_t data)')
    end=raw.index(b'\n}',start)+2
    parser=raw[start:end].decode('ascii')
    with tempfile.TemporaryDirectory(prefix='scan-newline-') as tmp:
        tmp=Path(tmp);src=tmp/'test.c';exe=tmp/'test.exe'
        src.write_text(PRE+screen+parser+POST,encoding='utf-8')
        subprocess.run([args.cc,'-std=c99','-Wall','-Wextra','-Werror',str(src),'-o',str(exe)],check=True)
        subprocess.run([str(exe)],check=True)


if __name__=='__main__':main()

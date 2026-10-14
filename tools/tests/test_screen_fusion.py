"""Compile actual screen/scanner code and inject requests at TX byte boundaries.

The test models TXE/RX interrupts, clock and task returns; it does not flash or
prove the physical HMI font/settings. Byte preservation can be checked against
the unchanged base project using --base-root.
"""
import argparse
from pathlib import Path
import re
import subprocess
import tempfile


def function(src, name):
    m = re.search(r'(?:static\s+)?(?:void|uint8_t|int|uint32_t)\s+' + name +
                  r'\s*\([^;]*?\)\s*\{', src)
    assert m, name
    end, depth = m.end(), 1
    while depth:
        depth += (src[end] == '{') - (src[end] == '}')
        end += 1
    return src[m.start():end]


PRE = r'''
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <setjmp.h>
static unsigned checks;
#define CHECK(c) do {++checks;if(!(c)){fprintf(stderr,"FAIL line %d: %s\n",__LINE__,#c);exit(2);}} while(0)
#define ENABLE 1
#define DISABLE 0
#define RESET 0
#define USART_IT_TXE 0x80
#define USART_FLAG_RXNE 0x20
#define USART_FLAG_ORE 8
#define USART_FLAG_FE 2
#define USART_FLAG_NE 4
#define USART_FLAG_PE 1
#define GPIO_Pin_2 4
#define CoreDebug_DEMCR_TRCENA_Msk 0x01000000U
#define GYRO_OLED_DEBUG_ENABLE 0
#define CAMERA_OLED_DEBUG_ENABLE 0
typedef struct {uint32_t SR, DR;} USART_TypeDef;
static USART_TypeDef test_uart;
#define UART5 (&test_uart)
static struct {uint32_t IDR;} test_gpio;
#define GPIOD (&test_gpio)
static struct {uint32_t ICSR;} test_scb;
#define SCB (&test_scb)
static struct {uint32_t DEMCR;} test_core;
#define CoreDebug (&test_core)
static uint32_t SystemCoreClock=72000000U;
static uint32_t test_cycles, test_dwt_ctrl, irq_mask;
static void (*inject_before_mask)(void);
static uint8_t wire[32768];
static unsigned wire_len, tx_enabled, tx_drains, display_calls, delay_calls;
static uint8_t scanner_oled_enabled;
static uint32_t scanner_oled_elapsed;
static uint32_t __get_PRIMASK(void){return irq_mask;}
static void __disable_irq(void){
 if(!irq_mask && inject_before_mask){void (*action)(void)=inject_before_mask;inject_before_mask=0;action();}
 irq_mask=1;
}
static void __set_PRIMASK(uint32_t saved){irq_mask=saved;}
static void USART_ITConfig(USART_TypeDef *u,unsigned kind,unsigned state){
 CHECK(u==UART5 && kind==USART_IT_TXE);tx_enabled=state;
}
static unsigned USART_GetITStatus(USART_TypeDef *u,unsigned kind){
 CHECK(u==UART5 && kind==USART_IT_TXE);return tx_enabled;
}
static void USART_SendData(USART_TypeDef *u,unsigned value){
 CHECK(u==UART5 && SCB->ICSR && !irq_mask && wire_len<sizeof(wire));
 wire[wire_len++]=(uint8_t)value;
}
static void Scanner_DisplayRefresh(void){++display_calls;}
void Gyro_DisplayPoll(void);
static void delay_ms(int ms){CHECK(!SCB->ICSR && ms>=0);++delay_calls;}
'''

TEST = r'''
static const uint8_t scan_a[]="123+321+213+132";
static const uint8_t scan_b[]="456+213+612+321";
static const uint8_t scan_c[]="654+123+456+231";
static void reset(uint32_t cycles){
 irq_mask=0;inject_before_mask=0;SCB->ICSR=0;test_uart.SR=test_uart.DR=0;test_gpio.IDR=GPIO_Pin_2;
 test_cycles=cycles;test_dwt_ctrl=0x10;tx_enabled=wire_len=delay_calls=display_calls=0;
 scanner_oled_enabled=0;test_hmi_init();
 CHECK(test_cycles==cycles && (test_dwt_ctrl&1));
 USART_ITConfig(UART5,USART_IT_TXE,ENABLE);
}
static void pump(void){
 CHECK(tx_enabled && !irq_mask);SCB->ICSR=69;UART5->SR=0;UART5_IRQHandler();SCB->ICSR=0;
 CHECK(!delay_calls);
}
static void drain(void){
 unsigned n=0;while(tx_enabled){pump();CHECK(++n<1000);}++tx_drains;
}
static void ready(void){reset(0xFFF00000U);drain();CHECK(wire_len==sizeof(expect_setup));
 CHECK(!memcmp(wire,expect_setup,sizeof(expect_setup)));wire_len=0;}
static void quiet_scan(const uint8_t *scan){
 unsigned i;uint32_t saved=irq_mask;irq_mask=1;
 for(i=0;i<15;++i){Scanner_ProcessByte(scan[i]);CHECK(irq_mask==1);}
 irq_mask=saved;
}
static void rx(unsigned byte,unsigned flags){
 UART5->SR=USART_FLAG_RXNE|flags;UART5->DR=byte;SCB->ICSR=69;
 UART5_IRQHandler();SCB->ICSR=0;UART5->SR=0;
}
static void assert_endings(unsigned commands){
 unsigned i=0,count=0;
 while(i<wire_len){
  CHECK(wire[i]!=13 && wire[i]!=10); /* only literal backslash+r */
  if(wire[i]==255){CHECK(i+2<wire_len && wire[i+1]==255 && wire[i+2]==255);++count;i+=3;}
  else ++i;
 }
 CHECK(count==commands && wire_len>=3 && wire[wire_len-1]==255);
}
static void expect_wire(const uint8_t *bytes,unsigned n,unsigned commands){
 CHECK(wire_len==n && !memcmp(wire,bytes,n));assert_endings(commands);
}
static void inject_scan(void){quiet_scan(scan_a);}
static void inject_complete(void){HMI_ShowTaskComplete();}
static void test_poll_races(void){
 ready();test_cycles+=SystemCoreClock/4U;inject_before_mask=inject_scan;HMI_PollStartup();
 CHECK(!inject_before_mask && !irq_mask && !hmi_startup_pending && !hmi_startup_retries && hmi_pending);
 drain();expect_wire(expect_a,sizeof(expect_a),4);
 ready();test_cycles+=SystemCoreClock/4U;inject_before_mask=inject_complete;HMI_PollStartup();
 CHECK(!inject_before_mask && !irq_mask && !hmi_startup_pending && !hmi_startup_retries && hmi_result_requested);
 drain();expect_wire(expect_result,sizeof(expect_result),4);
 ready();irq_mask=1;HMI_ShowTaskComplete();CHECK(irq_mask==1);irq_mask=0;drain();
 expect_wire(expect_result,sizeof(expect_result),4);
}
static void test_startup(void){
 unsigned i;uint32_t base;
 reset(0xFFF00000U);base=test_cycles;drain();expect_wire(expect_setup,sizeof(expect_setup),3);
 CHECK(hmi_startup_retries==12);wire_len=0;
 test_cycles=base+SystemCoreClock/4U-1;Gyro_DisplayPoll();CHECK(!tx_enabled && hmi_startup_retries==12);
 test_cycles=base+SystemCoreClock/4U;SCB->ICSR=16;Gyro_DisplayPoll();
 CHECK(!tx_enabled && hmi_startup_retries==12);SCB->ICSR=0;
 for(i=1;i<=12;++i){
  test_cycles=base+i*(SystemCoreClock/4U);
  Scanner_DisplayPoll(0);CHECK(hmi_startup_retries==12-i);
  Gyro_DisplayPoll();CHECK(hmi_startup_retries==12-i); /* no double request */
  drain();expect_wire(expect_setup,sizeof(expect_setup),3);wire_len=0;
 }
 test_cycles+=SystemCoreClock;Gyro_DisplayPoll();CHECK(!tx_enabled && !hmi_startup_retries);
 ready();test_cycles+=SystemCoreClock/4U;Gyro_DisplayPoll();CHECK(hmi_startup_pending);
 quiet_scan(scan_a);CHECK(!hmi_startup_pending && !hmi_startup_retries);
 drain();expect_wire(expect_a,sizeof(expect_a),4);
 test_cycles+=SystemCoreClock;Gyro_DisplayPoll();CHECK(!tx_enabled);
}
static void test_scan(void){
 unsigned i;uint8_t before[1000];
 ready();for(i=0;i<14;++i)Scanner_ProcessByte(scan_a[i]);
 CHECK(!hmi_pending && !saoma_ready);Scanner_ProcessByte(scan_a[14]);
 CHECK(saoma_ready && !memcmp(saoma_data,scan_a,16));
 memcpy(before,saoma_data,sizeof(before));drain();expect_wire(expect_a,sizeof(expect_a),4);
 wire_len=0;quiet_scan(scan_b);drain();expect_wire(expect_b,sizeof(expect_b),4);
 CHECK(!memcmp(before,saoma_data,sizeof(before)));
 ready();quiet_scan((const uint8_t*)"723+321+213+132");CHECK(!hmi_pending && !saoma_ready);
 ready();quiet_scan((const uint8_t*)"123+421+213+132");CHECK(!hmi_pending && !saoma_ready);
 ready();quiet_scan((const uint8_t*)"123-321+213+132");CHECK(!hmi_pending && !saoma_ready);
 /* Preserve the old distinction: duplicate colors can display, but cannot latch a task. */
 ready();quiet_scan((const uint8_t*)"111+321+213+132");CHECK(hmi_pending && !saoma_ready);
 ready();for(i=0;i<5;++i){rx(scan_a[i],0);}rx('9',USART_FLAG_ORE);
 CHECK(scanner_error_count==1 && scanner_last_error==USART_FLAG_ORE && !scan_count);
 for(i=0;i<15;++i){rx(scan_a[i],0);}
 drain();expect_wire(expect_a,sizeof(expect_a),4);
 CHECK(saoma_ready && !memcmp(saoma_data,scan_a,16));
 ready();for(i=0;i<15;++i){UART5->SR=USART_FLAG_RXNE;UART5->DR=scan_a[i];Scanner_PollReceive();CHECK(!irq_mask);UART5->SR=0;}
 drain();expect_wire(expect_a,sizeof(expect_a),4);CHECK(scanner_poll_rx_count==15 && !scanner_rx_count);
}
static void test_complete_boundaries(void){
 unsigned cut,i,n;uint8_t active[256],joined[256];
 for(cut=0;cut<=sizeof(expect_a);++cut){
  ready();quiet_scan(scan_a);for(i=0;i<cut;++i)pump();
  memcpy(active,hmi_tx_frame,sizeof(hmi_tx_frame));quiet_scan(scan_b);
  CHECK(!memcmp(active,hmi_tx_frame,sizeof(hmi_tx_frame)));n=wire_len;
  HMI_ShowTaskComplete();CHECK(wire_len==n && !hmi_pending && !hmi_startup_pending && !hmi_startup_retries);
  quiet_scan(scan_c);CHECK(!hmi_pending && !memcmp(saoma_data,scan_a,16));
  drain();
  if(!cut)expect_wire(expect_result,sizeof(expect_result),4);
  else{
   memcpy(joined,expect_a,sizeof(expect_a));memcpy(joined+sizeof(expect_a),expect_result,sizeof(expect_result));
   expect_wire(joined,sizeof(expect_a)+sizeof(expect_result),8);
  }
  n=wire_len;HMI_ShowTaskComplete();quiet_scan(scan_b);test_cycles+=SystemCoreClock;
  Gyro_DisplayPoll();CHECK(!tx_enabled && wire_len==n && !hmi_result_pending);
 }
 /* Result requests cannot split an active startup batch either. */
 for(cut=0;cut<=sizeof(expect_setup);++cut){
  reset(100);for(i=0;i<cut;++i)pump();quiet_scan(scan_a);HMI_ShowTaskComplete();drain();
  memcpy(joined,expect_setup,sizeof(expect_setup));memcpy(joined+sizeof(expect_setup),expect_result,sizeof(expect_result));
  expect_wire(joined,sizeof(expect_setup)+sizeof(expect_result),7);
 }
 /* Repeated requests and late scans cannot duplicate/overwrite an active result. */
 for(cut=0;cut<=sizeof(expect_result);++cut){
  ready();HMI_ShowTaskComplete();for(i=0;i<cut;++i)pump();
  HMI_ShowTaskComplete();quiet_scan(scan_b);HMI_ShowTaskComplete();drain();
  expect_wire(expect_result,sizeof(expect_result),4);
 }
}
static unsigned route_step,return_finished,block_return;
static jmp_buf return_wait;
static void yuanliao(void){CHECK(++route_step==1 && !hmi_result_requested);}
static void cujiagong(void){CHECK(++route_step==2 && !hmi_result_requested);}
static void zancunqu(void){CHECK(++route_step==3 && !hmi_result_requested);}
static void yuanliao2(void){CHECK(++route_step==4 && !hmi_result_requested);}
static void cujiagong2(void){CHECK(++route_step==5 && !hmi_result_requested);}
static void zancunqu2(void){CHECK(++route_step==6 && !hmi_result_requested);}
static void qidian(void){
 CHECK(++route_step==7 && !hmi_result_requested);if(block_return)longjmp(return_wait,1);return_finished=1;
}
static void run_original_task_calls(void){
'''

POST = r'''
}
int main(void){
 CHECK(sizeof(hmi_tx_frame)==sizeof(expect_a) && HMI_FRAME_LENGTH<256);
 test_startup();test_poll_races();test_scan();test_complete_boundaries();
 ready();route_step=return_finished=0;block_return=1;
 if(!setjmp(return_wait)){run_original_task_calls();CHECK(0);}
 CHECK(!return_finished && !hmi_result_requested);
 ready();route_step=return_finished=0;block_return=0;run_original_task_calls();
 CHECK(return_finished && route_step==7 && hmi_result_requested);drain();
 expect_wire(expect_result,sizeof(expect_result),4);
 printf("PASS: %u assertions; startup/retry/wrap, literal escapes/GB2312, raw scan/RX errors, every TX boundary, final lock, return timing\n",checks);
 return 0;
}
'''


def decode(path):
    raw = path.read_bytes()
    try:
        return raw.decode('utf-8-sig')
    except UnicodeDecodeError:
        return raw.decode('gb18030')


def expected_arrays():
    end = b'\xff' * 3
    setup = b'page page0' + end + b'vis t1,0' + end + b'vis t0,1' + end
    def scan(value):
        return setup + b'page0.t0.txt="' + value[:8] + b'\\r' + value[8:] + b'"' + end
    result = setup + b'page0.t0.txt="' + '正确放置\\r次数：6\\r正确抓取\\r次数：6'.encode('gb2312') + b'"' + end
    entries = {'setup': setup, 'a': scan(b'123+321+213+132'),
               'b': scan(b'456+213+612+321'), 'result': result}
    return '\n'.join('static const uint8_t expect_' + name + '[]={' +
                     ','.join(f'0x{x:02x}' for x in value) + '};' for name, value in entries.items())


def check_preservation(base, merged):
    changed = []
    for p in base.rglob('*'):
        if not p.is_file() or any(x.lower() in {'objects', 'listings', '.git'} for x in p.relative_to(base).parts):
            continue
        if '.uvguix.' in p.name:
            continue
        rel = p.relative_to(base)
        assert (merged/rel).exists(), rel
        if p.read_bytes() != (merged/rel).read_bytes():
            changed.append(rel.as_posix())
    assert set(changed) == {'APP/main.c', 'hardware/uart_5.c', 'hardware/uart_5.h'}, changed
    old, new = decode(base/'hardware/uart_5.c'), decode(merged/'hardware/uart_5.c')
    for name in ['Scanner_ReceiveByte', 'Scanner_ProcessByte', 'Scanner_PollReceive',
                 'Scanner_WaitForData', 'UART5_IRQHandler', 'Scanner_DisplayInit']:
        assert function(old, name) == function(new, name), name
    old_main, new_main = (base/'APP/main.c').read_bytes(), (merged/'APP/main.c').read_bytes()
    removed = re.sub(rb'(?m)^[ \t]*HMI_ShowTaskComplete\(\);[^\r\n]*\r?\n', b'', new_main)
    assert removed == old_main, 'main changed beyond the completion call'
    assert decode(merged/'hardware/uart_5.h').count('void HMI_ShowTaskComplete(void);') == 1
    # HMI owns only new clock reads/enabling; no assignment clears CYCCNT.
    assert not re.search(r'HMI_DWT_CYCCNT\s*=(?!=)', new)
    print('PASS: only three source files differ; task/params/RX unchanged; main adds only final callback')


def main():
    p = argparse.ArgumentParser()
    p.add_argument('--source-root', type=Path, required=True)
    p.add_argument('--base-root', type=Path)
    p.add_argument('--cc', default='gcc')
    args = p.parse_args()
    src = decode(args.source_root/'hardware/uart_5.c')
    a = src.index('volatile uint8_t saoma_ready')
    b = src.index('volatile uint8_t uart5_RxData', a)
    screen = src[a:b]
    screen = re.sub(r'^#define HMI_DWT_CTRL\s+.*$', '#define HMI_DWT_CTRL test_dwt_ctrl', screen, flags=re.M)
    screen = re.sub(r'^#define HMI_DWT_CYCCNT\s+.*$', '#define HMI_DWT_CYCCNT test_cycles', screen, flags=re.M)
    init = function(src, 'uart_init5')
    init = init[init.index('    saoma_ready = 0;'):init.index('    /* 开启UART5')]
    scanner = '\n'.join(function(src, name) for name in ['Scanner_ReceiveByte', 'Scanner_ProcessByte',
        'Scanner_WaitForData', 'UART5_IRQHandler', 'Scanner_PollReceive', 'Scanner_DisplayPoll', 'Gyro_DisplayPoll'])
    body = decode(args.source_root/'APP/main.c')
    start = re.search(r'(?m)^\s*yuanliao\(\);', body).start()
    finish = re.search(r'(?m)^\s*HMI_ShowTaskComplete\(\);[^\n]*', body).end()
    task_calls = body[start:finish]
    code = PRE + screen + '\nvolatile uint8_t uart5_RxData;\n' + scanner + '\n'
    code += 'static void test_hmi_init(void){\n' + init + '\n}\n'
    code += expected_arrays() + TEST + task_calls + POST
    with tempfile.TemporaryDirectory(prefix='screen-fusion-test-') as temp:
        c, exe = Path(temp)/'test.c', Path(temp)/'test.exe'
        c.write_text(code, encoding='utf-8')
        subprocess.run([args.cc, '-std=c99', '-O0', '-Wall', '-Wextra', '-Werror', str(c), '-o', str(exe)], check=True)
        subprocess.run([str(exe)], check=True, timeout=20)
    if args.base_root:
        check_preservation(args.base_root, args.source_root)


if __name__ == '__main__':
    main()

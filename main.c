#include <stdint.h>

// ------------------- Register Definitions -------------------
#define SYSCTL_RCGCGPIO_R   (*((volatile uint32_t *)0x400FE608))
#define SYSCTL_RCGCUART_R   (*((volatile uint32_t *)0x400FE618))
#define SYSCTL_RCGCADC_R    (*((volatile uint32_t *)0x400FE638))
#define SYSCTL_RCGCTIMER_R  (*((volatile uint32_t *)0x400FE604))
#define SYSCTL_RCGC2_R      (*((volatile uint32_t *)0x400FE108))

#define GPIO_PORTA_AFSEL_R  (*((volatile uint32_t *)0x40004420))
#define GPIO_PORTA_PCTL_R   (*((volatile uint32_t *)0x4000452C))
#define GPIO_PORTA_DEN_R    (*((volatile uint32_t *)0x4000451C))

#define GPIO_PORTE_AFSEL_R  (*((volatile uint32_t *)0x40024420))
#define GPIO_PORTE_AMSEL_R  (*((volatile uint32_t *)0x40024528))
#define GPIO_PORTE_DEN_R    (*((volatile uint32_t *)0x4002451C))

#define GPIO_PORTF_DIR_R    (*((volatile uint32_t *)0x40025400))
#define GPIO_PORTF_DEN_R    (*((volatile uint32_t *)0x4002551C))
#define GPIO_PORTF_DATA_R   (*((volatile uint32_t *)0x400253FC))
#define GPIO_PORTF_LOCK_R   (*((volatile uint32_t *)0x40025520))
#define GPIO_PORTF_CR_R     (*((volatile uint32_t *)0x40025524))

#define UART0_DR_R          (*((volatile uint32_t *)0x4000C000))
#define UART0_FR_R          (*((volatile uint32_t *)0x4000C018))
#define UART0_IBRD_R        (*((volatile uint32_t *)0x4000C024))
#define UART0_FBRD_R        (*((volatile uint32_t *)0x4000C028))
#define UART0_LCRH_R        (*((volatile uint32_t *)0x4000C02C))
#define UART0_CTL_R         (*((volatile uint32_t *)0x4000C030))

#define ADC0_ACTSS_R        (*((volatile uint32_t *)0x40038000))
#define ADC0_RIS_R          (*((volatile uint32_t *)0x40038004))
#define ADC0_ISC_R          (*((volatile uint32_t *)0x4003800C))
#define ADC0_EMUX_R         (*((volatile uint32_t *)0x40038014))
#define ADC0_SSMUX3_R       (*((volatile uint32_t *)0x400380A0))
#define ADC0_SSCTL3_R       (*((volatile uint32_t *)0x400380A4))
#define ADC0_SSFIFO3_R      (*((volatile uint32_t *)0x400380A8))
#define ADC0_PSSI_R         (*((volatile uint32_t *)0x40038028))

#define TIMER0_CFG_R        (*((volatile uint32_t *)0x40030000))
#define TIMER0_TAMR_R       (*((volatile uint32_t *)0x40030004))
#define TIMER0_TAILR_R      (*((volatile uint32_t *)0x40030028))
#define TIMER0_TAPR_R       (*((volatile uint32_t *)0x40030038))
#define TIMER0_CTL_R        (*((volatile uint32_t *)0x4003000C))
#define TIMER0_TAR_R        (*((volatile uint32_t *)0x40030048))

// ------------------- Constants -------------------
#define SYSCLK 16000000UL
#define MID    2048
#define HYS    80            // reduced for low-voltage signals
#define REPORT_INTERVAL_MS 1000   // 1 second

#define FREQ_NORMAL_MIN 49.5
#define FREQ_NORMAL_MAX 50.5
#define FREQ_WARNING_MIN 49.0
#define FREQ_WARNING_MAX 51.0

#define ROCOF_WARNING_HIGH   0.5
#define ROCOF_WARNING_LOW   -0.5
#define ROCOF_CRITICAL_HIGH  1.0
#define ROCOF_CRITICAL_LOW  -1.0

#define ROCOF_FILTER_SIZE 10
#define FREQ_FILTER_SIZE 5

// ------------------- Delay Functions -------------------
void delay_cycles(volatile uint32_t d){
    while(d--) __asm__("    nop");
}
void delay_ms(uint32_t ms){
    uint32_t i;
    while(ms--){
        for(i=0;i<2666;i++) __asm__("    nop");
    }
}

// ------------------- UART Functions -------------------
void UART_SendChar(char c){
    while(UART0_FR_R & (1<<5));
    UART0_DR_R = c;
}
void UART_SendString(const char *s){
    while(*s) UART_SendChar(*s++);
}
void UART_SendInt(int32_t v){
    char b[16];
    int i=0;
    if(v < 0){ UART_SendChar('-'); v=-v; }
    if(v == 0){ UART_SendChar('0'); return; }
    while(v){ b[i++] = (v%10)+'0'; v/=10; }
    while(i--) UART_SendChar(b[i]);
}
void UART_SendFloat(float v){
    if(v < 0){ UART_SendChar('-'); v = -v; }
    UART_SendInt((int)v);
    UART_SendChar('.');
    int d = (int)((v-(int)v)*100);
    if(d<10) UART_SendChar('0');
    UART_SendInt(d);
}

// UART INIT
void UART_Init(){
    SYSCTL_RCGCUART_R |= 1;
    SYSCTL_RCGCGPIO_R |= 1;
    GPIO_PORTA_AFSEL_R |= 0x03;
    GPIO_PORTA_PCTL_R = (GPIO_PORTA_PCTL_R & ~0xFF) | 0x11;
    GPIO_PORTA_DEN_R |= 0x03;
    UART0_CTL_R &= ~1;
    UART0_IBRD_R = 8;
    UART0_FBRD_R = 44;
    UART0_LCRH_R = 0x60;
    UART0_CTL_R = 0x301;
}

// LED
void LED_Init(){
    SYSCTL_RCGC2_R |= 0x20;
    GPIO_PORTF_LOCK_R = 0x4C4F434B;
    GPIO_PORTF_CR_R = 0x0E;
    GPIO_PORTF_DIR_R |= 0x0E;
    GPIO_PORTF_DEN_R |= 0x0E;
    GPIO_PORTF_DATA_R = 0x08;
}
void LED_SetStatus(float freq, float rocof){
    if(freq < FREQ_WARNING_MIN || freq > FREQ_WARNING_MAX ||
       rocof > ROCOF_CRITICAL_HIGH || rocof < ROCOF_CRITICAL_LOW)
        GPIO_PORTF_DATA_R = 0x02;
    else if(freq < FREQ_NORMAL_MIN || freq > FREQ_NORMAL_MAX ||
            rocof > ROCOF_WARNING_HIGH || rocof < ROCOF_WARNING_LOW)
        GPIO_PORTF_DATA_R = 0x04;
    else
        GPIO_PORTF_DATA_R = 0x08;
}

// ------------------- ADC -------------------
void ADC_Init(){
    SYSCTL_RCGCGPIO_R |= (1<<4);
    SYSCTL_RCGCADC_R |= 1;
    GPIO_PORTE_AFSEL_R |= (1<<3);
    GPIO_PORTE_DEN_R &= ~(1<<3);
    GPIO_PORTE_AMSEL_R |= (1<<3);
    ADC0_ACTSS_R &= ~8;
    ADC0_EMUX_R &= ~0xF000;
    ADC0_SSMUX3_R = 0;
    ADC0_SSCTL3_R = 0x06;
    ADC0_ACTSS_R |= 8;
}
uint16_t ADC_Read(){
    ADC0_PSSI_R = 8;
    while(!(ADC0_RIS_R & 8));
    uint16_t r = ADC0_SSFIFO3_R & 0xFFF;
    ADC0_ISC_R = 8;
    return r;
}

// -------------------- TIMER --------------------
void TIMER0_Init(){
    SYSCTL_RCGCTIMER_R |= 1;
    TIMER0_CTL_R &= ~1;
    TIMER0_CFG_R = 0;
    TIMER0_TAMR_R = 2;
    TIMER0_TAILR_R = 0xFFFFFFFF;
    TIMER0_CTL_R |= 1;
}

// ---------------------- MAIN -------------------------
int main(void){

    UART_Init();
    ADC_Init();
    TIMER0_Init();
    LED_Init();

    UART_SendString("\r\nROCOF + Freq Monitor (1s Interval, Any Voltage)\r\n");

    uint32_t prev_tick = 0;
    uint8_t have_prev = 0;
    int lastState = 0;

    float avg_f = 0;
    uint32_t avg_n = 0;

    float last_freq = 50.0;

    float rocof_buf[ROCOF_FILTER_SIZE];
    float rocof_sum = 0;
    int ri;
    for(ri=0; ri<ROCOF_FILTER_SIZE; ri++)
        rocof_buf[ri] = 0;

    int rocof_index = 0;
    float filtered_rocof = 0;

    float freq_buf[FREQ_FILTER_SIZE];
    float freq_sum = 0;
    int fi;
    for(fi=0; fi<FREQ_FILTER_SIZE; fi++){
        freq_buf[fi] = 50.0;
        freq_sum += 50.0;
    }

    int freq_index = 0;
    int alert_state = 0;

    uint32_t ms_count = 0;
    uint32_t measure_count = 0;

    while(1){

        uint16_t v = ADC_Read();

        int state = (v > MID + HYS) ? 1 :
                    (v < MID - HYS) ? 0 :
                     lastState;

        if(lastState == 0 && state == 1){
            uint32_t t = TIMER0_TAR_R;
            if(have_prev){
                uint32_t dt = (prev_tick >= t) ? (prev_tick - t) : (prev_tick + (0xFFFFFFFF - t));
                if(dt > 500){
                    float f = (float)SYSCLK / dt;
                    avg_f += f;
                    avg_n++;
                }
            } else have_prev = 1;
            prev_tick = t;
        }
        lastState = state;

        delay_ms(1);
        ms_count++;

        if(ms_count >= REPORT_INTERVAL_MS){

            ms_count = 0;
            measure_count++;

            float F_raw = (avg_n>0) ? avg_f/avg_n : last_freq;

            freq_sum -= freq_buf[freq_index];
            freq_buf[freq_index] = F_raw;
            freq_sum += F_raw;
            freq_index = (freq_index + 1) % FREQ_FILTER_SIZE;

            float F = freq_sum / FREQ_FILTER_SIZE;

            float rocof_raw = (F - last_freq) / 1.0f;
            last_freq = F;

            rocof_sum -= rocof_buf[rocof_index];
            rocof_buf[rocof_index] = rocof_raw;
            rocof_sum += rocof_raw;
            rocof_index = (rocof_index + 1) % ROCOF_FILTER_SIZE;

            filtered_rocof = rocof_sum / ROCOF_FILTER_SIZE;

            LED_SetStatus(F, filtered_rocof);

            if(filtered_rocof > ROCOF_CRITICAL_HIGH){
                if(alert_state != 1){
                    UART_SendString(">>> ALERT: ROCOF POSITIVE CRITICAL  ");
                    UART_SendFloat(filtered_rocof);
                    UART_SendString(" Hz/s\r\n");
                    alert_state = 1;
                }
            }
            else if(filtered_rocof < ROCOF_CRITICAL_LOW){
                if(alert_state != -1){
                    UART_SendString(">>> ALERT: ROCOF NEGATIVE CRITICAL  ");
                    UART_SendFloat(filtered_rocof);
                    UART_SendString(" Hz/s\r\n");
                    alert_state = -1;
                }
            }
            else {
                if(alert_state != 0){
                    UART_SendString(">>> ALERT CLEARED (ROCOF normal)\r\n");
                    alert_state = 0;
                }
            }

            UART_SendString("Measurement #");
            UART_SendInt(measure_count);
            UART_SendString(" | Freq: ");
            UART_SendFloat(F);
            UART_SendString(" Hz | ROCOF: ");
            UART_SendFloat(filtered_rocof);
            UART_SendString(" Hz/s | Status: ");

            if(F < FREQ_WARNING_MIN || F > FREQ_WARNING_MAX ||
                filtered_rocof > ROCOF_CRITICAL_HIGH || filtered_rocof < ROCOF_CRITICAL_LOW)
                UART_SendString("CRITICAL\r\n");
            else if(F < FREQ_NORMAL_MIN || F > FREQ_NORMAL_MAX ||
                    filtered_rocof > ROCOF_WARNING_HIGH || filtered_rocof < ROCOF_WARNING_LOW)
                UART_SendString("WARNING\r\n");
            else
                UART_SendString("NORMAL\r\n");

            avg_f = 0;
            avg_n = 0;
        }
    }
}

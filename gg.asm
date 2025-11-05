;========================================================
; Timer0 + Interrupt 跑馬燈（P0 Active-Low，含軟體上拉）
; 控制 LED0, LED1, LED2, LED7, LED11, LED15
; 晶振 22 MHz → 1 MC ≈ 0.545 µs
;========================================================

            ORG     0000H
            LJMP    START

            ORG     000BH
            LJMP    T0_ISR

;---------------- RAM 變數 -------------------------------
            ORG     0030H
CNT40:      DS      1
SEC:        DS      1
STATE:      DS      1
PAT:        DS      1          ; 儲存目前 LED 狀態（Active-Low）

;---------------- 主程式 -------------------------------
            ORG     0100H
START:
            MOV     SP,#60H
            MOV     CNT40,#00
            MOV     SEC,#00
            MOV     STATE,#00

            MOV     P0,#0FFH          ; 軟體上拉
            MOV     PAT,#0FFH         ; 全暗

            ; Timer0 Mode 1 (16-bit)
            ANL     TMOD,#0F0H
            ORL     TMOD,#01H
            MOV     TH0,#04CH
            MOV     TL0,#0D0H

            CLR     TF0
            SETB    ET0
            SETB    EA
            SETB    TR0

MAIN:       SJMP    MAIN

;---------------- Timer0 中斷 ---------------------------
T0_ISR:
            MOV     TH0,#04CH
            MOV     TL0,#0D0H

            INC     CNT40
            MOV     A,CNT40
            CJNE    A,#40,No1s
            MOV     CNT40,#00
            INC     SEC
No1s:

;---------------- 狀態機 -------------------------------
            MOV     A,STATE
            CJNE    A,#00,S1

;=== 狀態 0：LED0、LED15亮 ===
            MOV     A,#0FFH
            ANL     A,#10111110B      ; bit6(LED15)=0, bit0(LED0)=0
            MOV     PAT,A
            MOV     SEC,#00
            MOV     STATE,#01
            SJMP    Update

S1:         CJNE    A,#01,S2
;=== 狀態 1：5 秒後 LED15滅、LED11亮 ===
            MOV     A,SEC
            CJNE    A,#5,Update
            MOV     SEC,#00
            MOV     A,PAT
            ORL     A,#01000000B      ; bit6=1, LED15滅
            ANL     A,#11101111B      ; bit4=0, LED11亮
            MOV     PAT,A
            MOV     STATE,#02
            SJMP    Update

S2:         CJNE    A,#02,S3
;=== 狀態 2：3 秒後 LED11與LED0滅 → LED7、LED2亮 ===
            MOV     A,SEC
            CJNE    A,#3,Update
            MOV     SEC,#00
            MOV     A,PAT
            ORL     A,#00010001B      ; bit4(LED11)=1, bit0(LED0)=1 → 滅
            ANL     A,#01111011B      ; bit7(LED7)=0, bit2(LED2)=0 → 亮
            MOV     PAT,A
            MOV     STATE,#03
            SJMP    Update

S3:         CJNE    A,#03,S4
;=== 狀態 3：5 秒後 LED2滅、LED1亮 ===
            MOV     A,SEC
            CJNE    A,#5,Update
            MOV     SEC,#00
            MOV     A,PAT
            ORL     A,#00000100B      ; bit2=1 → LED2滅
            ANL     A,#11111101B      ; bit1=0 → LED1亮
            MOV     PAT,A
            MOV     STATE,#04
            SJMP    Update

S4:
;=== 狀態 4：3 秒後 LED1與LED7滅 → 回初始 ===
            MOV     A,SEC
            CJNE    A,#3,Update
            MOV     SEC,#00
            MOV     A,#0FFH
            MOV     PAT,A
            MOV     STATE,#00

Update:
            MOV     P0,#0FFH          ; 軟體上拉
            MOV     A,PAT
            MOV     P0,A
            RETI

            END

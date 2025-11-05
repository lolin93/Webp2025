;========================================================
; Timer0 + Interrupt 跑馬燈（P0 Active-Low，含軟體上拉）
; 加入：身障者穿越鍵（延長下一次綠燈時長）
; 控制 LED0, LED1, LED2, LED7, LED11, LED15
; 晶振 22 MHz → 1 MC ≈ 0.545 µs
;========================================================

            ORG     0000H
            LJMP    START

            ORG     000BH
            LJMP    T0_ISR

;---------------- 參數常數 -------------------------------
BASE_GRN    EQU     5          ; 基本綠燈秒數
EXT_ADD     EQU     3          ; 延長秒數（按鍵申請後）
DEB_THR     EQU     30         ; 去抖 30 ms（依 1 ms 中斷估算）
; 按鍵腳位：P3.2（低=按下）
; 若要改腳位，改下面的 JB/JNB P3.2 即可

;---------------- RAM 變數 -------------------------------
            ORG     0030H
CNT40:      DS      1          ; 1ms 計數 -> 1秒
SEC:        DS      1          ; 當前狀態累積秒數
STATE:      DS      1
PAT:        DS      1          ; 目前輸出（Active-Low）
GRN_DUR:    DS      1          ; 這一輪綠燈應等待秒數（可被延長）
EXT_REQ:    DS      1          ; 是否已申請延長（1=是、0=否）
BTN_CNT:    DS      1          ; 去抖累計 ms
BTN_LCK:    DS      1          ; 單次鎖定（避免長按重複觸發）

;---------------- 主程式 -------------------------------
            ORG     0100H
START:
            MOV     SP,#60H
            MOV     CNT40,#00
            MOV     SEC,#00
            MOV     STATE,#00
            MOV     GRN_DUR,#BASE_GRN
            MOV     EXT_REQ,#00
            MOV     BTN_CNT,#00
            MOV     BTN_LCK,#00

            MOV     P0,#0FFH          ; 軟體上拉
            MOV     PAT,#0FFH         ; 全暗

            ; Timer0 Mode 1 (16-bit)，約 1ms 中斷
            ANL     TMOD,#0F0H
            ORL     TMOD,#01H
            MOV     TH0,#04CH
            MOV     TL0,#0D0H

            CLR     TF0
            SETB    ET0
            SETB    EA
            SETB    TR0

MAIN:       SJMP    MAIN

;---------------- Timer0 中斷（約 1 ms） -----------------
T0_ISR:
            MOV     TH0,#04CH
            MOV     TL0,#0D0H

;--- 身障者穿越鍵去抖：P3.2 低=按下 ---
; 放開：計數清零、解除鎖定（可再次申請）
            JB      P3.2,Btn_Released          ; P3.2=1 → 未按
; 低=按下：若尚未鎖定，累計去抖
            MOV     A,BTN_LCK
            JNZ     Btn_Done                   ; 已鎖定就不再累計
            INC     BTN_CNT
            MOV     A,BTN_CNT
            CJNE    A,#DEB_THR,Btn_Done        ; 未達門檻
            MOV     EXT_REQ,#01                ; 申請延長下一次綠燈
            MOV     BTN_LCK,#01                ; 本次按壓已生效
            SJMP    Btn_Done
Btn_Released:
            MOV     BTN_CNT,#00
            MOV     BTN_LCK,#00
Btn_Done:

;--- 1秒時基 ---
            INC     CNT40
            MOV     A,CNT40
            CJNE    A,#40,No1s
            MOV     CNT40,#00
            INC     SEC
No1s:

;---------------- 黃燈閃爍（每秒翻轉） -----------------
; STATE=02（B 黃=LED11）與 STATE=04（A 黃=LED1）期間，
; 在 SEC<3 時每秒切換亮/滅（Active-Low → XRL 對應位元）
            MOV     A,STATE
            CJNE    A,#02,ChkS4
; --- S2：B 黃閃爍（LED11=P0.4） ---
            MOV     A,SEC
            CJNE    A,#3,B_BlinkDo
            SJMP    BlinkDone
B_BlinkDo:  MOV     A,PAT
            XRL     A,#00010000B       ; 反轉 bit4（LED11）
            MOV     PAT,A
            SJMP    BlinkDone

ChkS4:      CJNE    A,#04,BlinkDone
; --- S4：A 黃閃爍（LED1=P0.1） ---
            MOV     A,SEC
            CJNE    A,#3,A_BlinkDo
            SJMP    BlinkDone
A_BlinkDo:  MOV     A,PAT
            XRL     A,#00000010B       ; 反轉 bit1（LED1）
            MOV     PAT,A
BlinkDone:

;---------------- 狀態機 -------------------------------
            MOV     A,STATE
            CJNE    A,#00,S1

;=== 狀態 0：設定 A紅、B綠；並決定「下一段」綠燈秒數 ===
            MOV     A,#0FFH
            ANL     A,#10111110B       ; bit6(LED15)=0(B綠亮), bit0(LED0)=0(A紅亮)
            MOV     PAT,A

            ; 決定 S1（B 綠等待）要等多久：若有申請，延長
            MOV     A,EXT_REQ
            JZ      NoExt_S1
            MOV     GRN_DUR,#(BASE_GRN+EXT_ADD)
            MOV     EXT_REQ,#00
            SJMP    SetS1
NoExt_S1:   MOV     GRN_DUR,#BASE_GRN
SetS1:      MOV     SEC,#00
            MOV     STATE,#01
            SJMP    Update

S1:         CJNE    A,#01,S2
;=== 狀態 1：B 綠等待 GRN_DUR 秒 → B 黃亮（進入 S2：黃閃） ===
            MOV     A,SEC
            CJNE    A,GRN_DUR,Update
            MOV     SEC,#00
            MOV     A,PAT
            ORL     A,#01000000B       ; bit6=1, B綠滅（LED15滅）
            ANL     A,#11101111B       ; bit4=0, B黃亮（LED11亮）
            MOV     PAT,A
            MOV     STATE,#02
            SJMP    Update

S2:         CJNE    A,#02,S3
;=== 狀態 2（B 黃閃 3 秒）：結束後 → B紅、A綠 ===
            MOV     A,SEC
            CJNE    A,#3,Update
            MOV     SEC,#00
            MOV     A,PAT
            ORL     A,#00010001B       ; B黃滅(LED11=1)、A紅滅(LED0=1)
            ANL     A,#01111011B       ; B紅亮(LED7=0)、A綠亮(LED2=0)
            MOV     PAT,A

            ; 決定 S3（A 綠等待）要等多久：若有申請，延長
            MOV     A,EXT_REQ
            JZ      NoExt_S3
            MOV     GRN_DUR,#(BASE_GRN+EXT_ADD)
            MOV     EXT_REQ,#00
            SJMP    SetS3
NoExt_S3:   MOV     GRN_DUR,#BASE_GRN
SetS3:      MOV     STATE,#03
            SJMP    Update

S3:         CJNE    A,#03,S4
;=== 狀態 3：A 綠等待 GRN_DUR 秒 → A 黃亮（進入 S4：黃閃） ===
            MOV     A,SEC
            CJNE    A,GRN_DUR,Update
            MOV     SEC,#00
            MOV     A,PAT
            ORL     A,#00000100B       ; A綠滅(LED2=1)
            ANL     A,#11111101B       ; A黃亮(LED1=0)
            MOV     PAT,A
            MOV     STATE,#04
            SJMP    Update

S4:
;=== 狀態 4（A 黃閃 3 秒）：結束後 → 回到初始循環（A紅、B綠） ===
            MOV     A,SEC
            CJNE    A,#3,Update
            MOV     SEC,#00
            MOV     A,#0FFH
            MOV     PAT,A
            ; 回到起始：A紅、B綠（會在 S0 設定並重新決定下一段綠燈秒數）
            ANL     A,#10111110B
            MOV     PAT,A
            MOV     STATE,#01          ; 直接進入 S1（也可設 #00，再由 S0 重新進入）
            SJMP    Update

Update:
            MOV     P0,#0FFH           ; 軟體上拉
            MOV     A,PAT
            MOV     P0,A
            RETI

            END

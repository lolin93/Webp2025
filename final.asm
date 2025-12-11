; ---------------------------------------------------------
; Lab06 - LCD + Sound + Buttons Demo (最終修正版)
; ---------------------------------------------------------

              ORG     0000H
              AJMP    MAIN

;----------------------------------------------------------
; Timer0 中斷向量
;----------------------------------------------------------
              ORG     000BH
              AJMP    TIMER0_ISR

;----------------------------------------------------------
; 變數 / 位址定義
;----------------------------------------------------------
NOTE_TH     EQU     30H              ; Timer0 reload TH
NOTE_TL     EQU     31H              ; Timer0 reload TL
MODE_FLAG   EQU     020H             ; 模式旗標 (0/1)
EXIT_FLAG   EQU     021H             ; ★★ 立即退出播放旗標 ★★

;----------------------------------------------------------
; 主程式
;----------------------------------------------------------
              ORG     0100H

MAIN:
              MOV     SP, #60H

              ; Port 初始化
              MOV     P1, #0FFH      ; LCD 資料線預設為輸入/高
              MOV     P2, #0FFH      ; 蜂鳴器腳位預設高(靜音)
              MOV     P3, #0FFH      ; P3 作為輸入，按鍵 + LCD 控制

              ; Timer0 設定
              MOV     TMOD, #01H     ; Timer0 Mode 1
              CLR     TR0

              ; 中斷允許
              SETB    EA
              SETB    ET0

              ; 蜂鳴器預設靜音 (P2.7 = 1)
              SETB    P2.7

              ; 模式初始為 0
              CLR     MODE_FLAG

              ; LCD 初始化 + 顯示文字
              ACALL   LCD_INIT
              ACALL   LCD_SHOW_MERRY

;----------------------------------------------------------
; 主迴圈
;----------------------------------------------------------
MAIN_LOOP:
              ACALL   CHECK_KEYS

              JNB     MODE_FLAG, MODE0
              SJMP    MODE1

;----------------------------------------------------------
; 模式 0：We Wish You A Merry Christmas 播 3 次 + Happy New Year
;----------------------------------------------------------
MODE0:
              MOV     R7, #03        ; 播放三次 We Wish 主旋律
M0_LOOP:
              MOV     DPTR, #MELODY_WISH
              ACALL   PLAY_MELODY_NO_END_DELAY ; ★★ 使用無延遲版本 ★★
              DJNZ    R7, M0_LOOP

              ; 播一次 Happy New Year 段
              MOV     DPTR, #MELODY_HAPPY
              ACALL   PLAY_MELODY            ; 使用標準版本 (結尾有長延遲)

              SJMP    MAIN_LOOP

;----------------------------------------------------------
; 模式 1：三首歌以上 (We Wish / Song2 / Song3)
;----------------------------------------------------------
MODE1:
              MOV     DPTR, #MELODY_WISH
              ACALL   PLAY_MELODY

              MOV     DPTR, #MELODY_SONG2
              ACALL   PLAY_MELODY

              MOV     DPTR, #MELODY_SONG3
              ACALL   PLAY_MELODY

              SJMP    MAIN_LOOP

;----------------------------------------------------------
; 播放一首歌：PLAY_MELODY (標準版，結束有 LONG_DELAY)
;----------------------------------------------------------
PLAY_MELODY:
              CLR     EXIT_FLAG            ; 進入子程式先清除退出旗標

PLAY_LOOP:
              ; 檢查退出旗標：若按下按鍵，則立即退出
              JNB     EXIT_FLAG, CHECK_TH
              SJMP    END_SONG             ; 立即退出

CHECK_TH:
              ; 讀取 TH (使用 DPTR)
              CLR     A
              MOVC    A, @A+DPTR
              JZ      END_SONG             ; 讀到 00H → 結束
              MOV     NOTE_TH, A
              INC     DPTR

              ; 讀取 TL
              CLR     A
              MOVC    A, @A+DPTR
              MOV     NOTE_TL, A
              INC     DPTR

              ; 讀取 Duration
              CLR     A
              MOVC    A, @A+DPTR
              MOV     R2, A
              INC     DPTR

              ; 設定 Timer0 初值
              MOV     TH0, NOTE_TH
              MOV     TL0, NOTE_TL

              ; 播音
              SETB    TR0
              ACALL   DELAY_NOTE
              CLR     TR0
              SETB    P2.7

              ; 音與音之間停頓 (★★ 修正：縮短間隔，讓音樂連貫 ★★)
              MOV     R2, #10
              ACALL   DELAY_NOTE

              ; 讓 LCD 動一下
              ACALL   LCD_SCROLL_ONCE

              ; 檢查按鍵 (若按下，會設定 EXIT_FLAG)
              ACALL   CHECK_KEYS

              SJMP    PLAY_LOOP

END_SONG:
              ; 每首歌結束時稍微停一下
              ACALL   LONG_DELAY
              RET

;----------------------------------------------------------
; 播放一首歌 (無結尾長延遲版本)
;----------------------------------------------------------
PLAY_MELODY_NO_END_DELAY:
              CLR     EXIT_FLAG

PMLND_LOOP:
              ; 檢查退出旗標：若按下按鍵，則立即退出
              JNB     EXIT_FLAG, PMLND_CHECK_TH
              SJMP    PMLND_END_SONG       ; 立即退出 (無長延遲)

PMLND_CHECK_TH:
              ; 讀取 TH
              CLR     A
              MOVC    A, @A+DPTR
              JZ      PMLND_END_SONG       ; 讀到 00H → 結束 (無長延遲)
              MOV     NOTE_TH, A
              INC     DPTR

              ; 讀取 TL
              CLR     A
              MOVC    A, @A+DPTR
              MOV     NOTE_TL, A
              INC     DPTR

              ; 讀取 Duration
              CLR     A
              MOVC    A, @A+DPTR
              MOV     R2, A
              INC     DPTR

              ; 設定 Timer0 初值
              MOV     TH0, NOTE_TH
              MOV     TL0, NOTE_TL

              ; 播音
              SETB    TR0
              ACALL   DELAY_NOTE
              CLR     TR0
              SETB    P2.7

              ; 音與音之間停頓 (★★ 修正：縮短間隔到 #10 ★★)
              MOV     R2, #10
              ACALL   DELAY_NOTE

              ; 讓 LCD 動一下
              ACALL   LCD_SCROLL_ONCE

              ; 檢查按鍵
              ACALL   CHECK_KEYS

              SJMP    PMLND_LOOP

PMLND_END_SONG:
              RET                          ; 直接返回，無 LONG_DELAY

;----------------------------------------------------------
; Timer0 ISR: 產生頻率方波 / Reload (保持不變)
;----------------------------------------------------------
TIMER0_ISR:
              PUSH    ACC
              PUSH    PSW

              MOV     TH0, NOTE_TH
              MOV     TL0, NOTE_TL
              CPL     P2.7                 ; 反轉 P2.7 蜂鳴器腳位

              POP     PSW
              POP     ACC
              RETI

;----------------------------------------------------------
; DELAY_NOTE / LONG_DELAY / KEY_DELAY (保持不變)
;----------------------------------------------------------
DELAY_NOTE:
D1:           MOV     R3, #20
D2:           MOV     R4, #248
D3:           DJNZ    R4, D3
              DJNZ    R3, D2
              DJNZ    R2, D1
              RET

LONG_DELAY:
              MOV     R2, #250
              ACALL   DELAY_NOTE
              RET

KEY_DELAY:
              MOV     R6, #200
KD1:          DJNZ    R6, KD1
              RET

;----------------------------------------------------------
; CHECK_KEYS: 按鍵檢查邏輯 (切換模式後設置 EXIT_FLAG)
;----------------------------------------------------------
CHECK_KEYS:
              ;--- 檢查 S1 (P3.2) ---
              JB      P3.2, CK_S2
              ACALL   KEY_DELAY
              JNB     P3.2, $
              CLR     MODE_FLAG
              SETB    EXIT_FLAG            ; ★★ 設置退出旗標 ★★
              RET

CK_S2:
              ;--- 檢查 S2 (P3.3) ---
              JB      P3.3, CK_S3
              ACALL   KEY_DELAY
              JNB     P3.3, $
              SETB    MODE_FLAG
              SETB    EXIT_FLAG            ; ★★ 設置退出旗標 ★★
              RET

CK_S3:
              ;--- 檢查 S3 (P3.4) ---
              JB      P3.4, CK_DONE
              ACALL   KEY_DELAY
              JNB     P3.4, $
              CPL     MODE_FLAG
              SETB    EXIT_FLAG            ; ★★ 設置退出旗標 ★★
CK_DONE:
              RET

;----------------------------------------------------------
; LCD 子程式 (保持不變)
;----------------------------------------------------------
LCD_DELAY_SHORT:
              MOV     R5, #50
LDS1:         DJNZ    R5, LDS1
              RET

LCD_DELAY_LONG:
              MOV     R5, #200
LDL1:         DJNZ    R5, LDL1
              RET

LCD_PULSE_E:
              SETB    P3.1
              ACALL   LCD_DELAY_SHORT
              CLR     P3.1
              RET

LCD_CMD:
              MOV     P1, A
              CLR     P3.0
              ACALL   LCD_PULSE_E
              ACALL   LCD_DELAY_LONG
              RET

LCD_WRITE_DATA:
              MOV     P1, A
              SETB    P3.0
              ACALL   LCD_PULSE_E
              ACALL   LCD_DELAY_LONG
              RET

LCD_INIT:
              ACALL   LCD_DELAY_LONG

              MOV     A, #038H
              ACALL   LCD_CMD

              MOV     A, #00CH
              ACALL   LCD_CMD

              MOV     A, #001H
              ACALL   LCD_CMD

              MOV     A, #006H
              ACALL   LCD_CMD

              RET

; 顯示 "MERRY CHRISTMAS" (已修正 DPTR 讀取)
LCD_SHOW_MERRY:
              MOV     A, #080H
              ACALL   LCD_CMD

              MOV     DPTR, #LCD_TEXT
LPRINT:
              CLR     A
              MOVC    A, @A+DPTR
              JZ      LPRINT_END
              ACALL   LCD_WRITE_DATA
              INC     DPTR
              SJMP    LPRINT

LPRINT_END:
              RET

LCD_SCROLL_ONCE:
              MOV     A, #018H
              ACALL   LCD_CMD
              RET

;----------------------------------------------------------
; LCD 顯示字串
;----------------------------------------------------------
LCD_TEXT:
              DB  'MERRY CHRISTMAS',0

;----------------------------------------------------------
; 樂譜表 (已修正 MELODY_HAPPY 的連貫性)
;----------------------------------------------------------
MELODY_WISH:
              DB  0F6H,009H,30
              DB  0F8H,08CH,30
              DB  0F8H,08CH,15
              DB  0F9H,05BH,15

              DB  0F8H,08CH,15
              DB  0F8H,018H,15
              DB  0F7H,020H,30
              DB  0F7H,020H,30

              DB  0F7H,020H,30
              DB  0F9H,05BH,30
              DB  0F9H,05BH,15
              DB  0FAH,015H,15

              DB  0F9H,05BH,15
              DB  0F8H,08CH,15
              DB  0F8H,018H,30
              DB  0F6H,009H,30

              DB  00H

MELODY_HAPPY: ; ★★ 修正版 ★★
              DB  0F6H,009H,30      ; G3  (And)
              DB  0F8H,08CH,30      ; C4  (a)
              DB  0F9H,05BH,30      ; D4  (Happy)
              DB  0FAH,015H,30      ; E4  (New)
              DB  0F8H,08CH,60      ; C4  (Year!)
              DB  00H

MELODY_SONG2:
              DB  0F8H,08CH,20
              DB  0F8H,08CH,20
              DB  0F8H,08CH,40
              DB  0F6H,009H,20

              DB  0F8H,08CH,20
              DB  0F9H,05BH,20
              DB  0FAH,015H,40
              DB  0F8H,08CH,20

              DB  0F8H,08CH,20
              DB  0FAH,015H,20
              DB  0FAH,015H,40
              DB  0F9H,05BH,20

              DB  0F9H,05BH,20
              DB  0FAH,015H,20
              DB  0F8H,08CH,60
              DB  0F6H,009H,30

              DB  00H

MELODY_SONG3:
              DB  0F7H,020H,40
              DB  0F7H,020H,40
              DB  0F8H,018H,40
              DB  0F8H,08CH,40

              DB  0F8H,08CH,40
              DB  0F8H,018H,40
              DB  0F7H,020H,80

              DB  0F8H,08CH,40
              DB  0F8H,08CH,40
              DB  0F9H,05BH,40
              DB  0FAH,015H,40

              DB  0FAH,015H,40
              DB  0F9H,05BH,40
              DB  0F8H,08CH,80

              DB  00H

              END

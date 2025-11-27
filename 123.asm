; ---------------------------------------------------------
; 8051 Melody Player: We Wish You A Merry Christmas
; Output Pin: P2.7
; Clock: 12MHz (Assumed)
; ---------------------------------------------------------

    ORG 0000H
    AJMP MAIN           ; 程式開始，跳至 MAIN

    ORG 000BH           ; Timer 0 中斷向量
    AJMP TIMER0_ISR

; --- 變數定義 ---
NOTE_TH EQU 30H         ; 存放目前音符的 Timer High 值
NOTE_TL EQU 31H         ; 存放目前音符的 Timer Low 值
DURATION EQU 32H        ; 音符持續時間計數

    ORG 0100H

MAIN:
    MOV SP, #60H        ; 設定堆疊指標
    MOV TMOD, #01H      ; Timer 0 設定為 Mode 1 (16-bit)
    SETB EA             ; 開啟總中斷
    SETB ET0            ; 開啟 Timer 0 中斷
    CLR TR0             ; 先關閉 Timer

NEXT_NOTE:
    MOV DPTR, #MELODY   ; 指向旋律表
    MOV R0, #00H        ; 旋律索引 (Offset)

PLAY_LOOP:
    ; 讀取音符 (TH)
    MOV A, R0
    MOVC A, @A+DPTR
    JZ END_SONG         ; 如果讀到 00H，表示歌曲結束
    MOV NOTE_TH, A      ; 存入 TH

    ; 讀取音符 (TL)
    INC R0
    MOV A, R0
    MOVC A, @A+DPTR
    MOV NOTE_TL, A      ; 存入 TL

    ; 讀取節拍 (Duration)
    INC R0
    MOV A, R0
    MOVC A, @A+DPTR
    MOV R2, A           ; R2 用於延遲迴圈次數

    ; 準備下一個音符的索引
    INC R0

    ; --- 播放音符 ---
    SETB TR0            ; 啟動 Timer (開始發聲)
    ACALL DELAY_NOTE    ; 持續發聲一段時間
    CLR TR0             ; 停止 Timer (停止發聲)
    MOV P2.7, #1        ; 確保輸出為高電位 (靜音)
    
    ; --- 音符間的短暫停頓 (斷音) ---
    MOV R2, #10         
    ACALL DELAY_NOTE
    
    SJMP PLAY_LOOP      ; 繼續下一個音符

END_SONG:
    ACALL LONG_DELAY    ; 播完後休息一下
    SJMP NEXT_NOTE      ; 重播

; --- Timer 0 中斷服務程式 (產生頻率) ---
TIMER0_ISR:
    PUSH ACC
    PUSH PSW
    
    ; 重載 Timer 數值
    MOV TH0, NOTE_TH
    MOV TL0, NOTE_TL
    
    ; 反轉 P2.7 產生方波
    CPL P2.7
    
    POP PSW
    POP ACC
    RETI

; --- 延遲副程式 ---
DELAY_NOTE:
    ; 根據 R2 的值進行延遲
    ; R2 * 10ms (大約)
D1: MOV R3, #20
D2: MOV R4, #248
D3: DJNZ R4, D3
    DJNZ R3, D2
    DJNZ R2, D1
    RET

LONG_DELAY:
    MOV R2, #200
    ACALL DELAY_NOTE
    RET

; --- 旋律資料表 (TH, TL, Duration) ---
; 頻率對應 (12MHz):
; Low G (Sol): F9 1F
; C (Do):      F8 8C
; D (Re):      F9 5B
; E (Mi):      FA 15
; F (Fa):      FA 67
; G (Sol):     FB 04
; A (La):      FB 90
; B (Si):      FC 0C
; High C:      FC 44

MELODY:
    ; We (Low G)
    DB 0F9H, 01FH, 20
    ; Wish (C)
    DB 0F8H, 08CH, 20
    ; You (C)
    DB 0F8H, 08CH, 10
    ; A (D)
    DB 0F9H, 05BH, 10
    ; Mer (C)
    DB 0F8H, 08CH, 10
    ; ry (B)
    DB 0FCH, 00CH, 10
    ; Christ (A)
    DB 0FBH, 090H, 20
    ; mas (A)
    DB 0FBH, 090H, 20

    ; We (A)
    DB 0FBH, 090H, 20
    ; Wish (D)
    DB 0F9H, 05BH, 20
    ; You (D)
    DB 0F9H, 05BH, 10
    ; A (E)
    DB 0FAH, 015H, 10
    ; Mer (D)
    DB 0F9H, 05BH, 10
    ; ry (C)
    DB 0F8H, 08CH, 10
    ; Christ (B)
    DB 0FCH, 00CH, 20
    ; mas (Low G)
    DB 0F9H, 01FH, 20
    
    ; 結束標記
    DB 00H
    
END

; ---------------------------------------------------------
; Lab06 - Sound Control Demo (Long Pause Version)
; Melody Player: We Wish You A Merry Christmas
; ---------------------------------------------------------

            ORG     0000H            ; 程式起始位址 (Reset Vector)
            AJMP    MAIN             ; CPU 啟動後，無條件跳轉到 MAIN

;----------------------------------------------------------
; Timer0 中斷向量 (Interrupt Service Routine, ISR)
;----------------------------------------------------------
            ORG     000BH            ; Timer0 中斷服務常式的固定位址
            AJMP    TIMER0_ISR       ; 跳轉到 TIMER0_ISR 標籤處執行中斷程式

;----------------------------------------------------------
; 變數定義 (Internal RAM)
;----------------------------------------------------------
NOTE_TH     EQU     30H              ; 定義內部 RAM 30H 存放 Timer0 重載值 (高位元)
NOTE_TL     EQU     31H              ; 定義內部 RAM 31H 存放 Timer0 重載值 (低位元)

;----------------------------------------------------------
; 主程式 (Main Program)
;----------------------------------------------------------
            ORG     0100H            ; 主程式碼從 0100H 位址開始

MAIN:
            MOV     SP, #60H         ; 設定堆疊指標 (Stack Pointer)，用於子程式呼叫/中斷
            MOV     TMOD, #01H       ; 設定 Timer0 為 Mode 1 (16-bit Timer)
            CLR     TR0              ; 清除 Timer0 啟動位元，暫時關閉 Timer0 計數
            SETB    EA               ; 開啟總中斷 (Enable All Interrupts)
            SETB    ET0              ; 開啟 Timer0 溢位中斷 (Enable Timer0 Interrupt)
            SETB    P2.7             ; 蜂鳴器腳位預設為高電位 (靜音狀態)

NEXT_SONG:                           ; 歌曲開始標籤
            MOV     DPTR, #MELODY    ; DPTR (資料指標) 指向樂譜表 MELODY 的起始位址
            MOV     R0, #00H         ; R0 作為樂譜的 byte 索引，從 0 開始

PLAY_LOOP:                           ; 播放單顆音符的迴圈
    ; ===== 1. 讀取 TH (頻率高位元) =====
            MOV     A, R0            ; A = 索引 R0
            MOVC    A, @A+DPTR       ; 讀取程式記憶體 (樂譜表) 內容到累加器 A
            JZ      END_SONG         ; 若 A=00H (遇到樂譜結束標記)，則跳轉到歌曲結束
            MOV     NOTE_TH, A       ; 存入變數 NOTE_TH (給 ISR 使用)

    ; ===== 2. 讀取 TL (頻率低位元) =====
            INC     R0               ; 索引 R0 + 1 (指向 TL 的位置)
            MOV     A, R0
            MOVC    A, @A+DPTR       ; 讀取 TL
            MOV     NOTE_TL, A       ; 存入變數 NOTE_TL

    ; ===== 3. 讀取 Duration (音長) =====
            INC     R0               ; 索引 R0 + 1 (指向 Duration 的位置)
            MOV     A, R0
            MOVC    A, @A+DPTR       ; 讀取 Duration
            MOV     R2, A            ; 存入 R2，作為 DELAY_NOTE 的迴圈次數

    ; ===== 4. 準備下個音的指標 =====
            INC     R0               ; 索引 R0 + 1 (準備指向下一組 TH)

    ; ===== 5. 設定 Timer 初值 (為了讓 ISR 啟動時立刻重載) =====
            MOV     TH0, NOTE_TH     ; 將頻率高位元載入 TH0
            MOV     TL0, NOTE_TL     ; 將頻率低位元載入 TL0

    ; ===== 6. 播放聲音 (Sound On) =====
            SETB    TR0              ; 啟動 Timer0 計數 (開始發聲, Bonus 1)
            ACALL   DELAY_NOTE       ; 執行延遲，聲音持續 R2 決定的時間
            CLR     TR0              ; 停止 Timer0 計數 (停止發聲)
            SETB    P2.7             ; 確保蜂鳴器腳位固定為高 (靜音)

    ; ===== 7. 音與音之間的停頓 (Silence / Bonus 2 斷奏效果) =====
            MOV     R2, #25          ; R2 設定為 25，控制停頓時間 (數值大，停頓久)
            ACALL   DELAY_NOTE       ; 執行一段時間的靜音停頓

            SJMP    PLAY_LOOP        ; 跳回迴圈頂端，播放下一顆音

;----------------------------------------------------------
; 歌曲結束
;----------------------------------------------------------
END_SONG:
            ACALL   LONG_DELAY       ; 呼叫較長延遲
            SJMP    NEXT_SONG        ; 跳回歌曲開始，重播

;----------------------------------------------------------
; Timer0 ISR: 產生指定頻率方波 (Bonus 1 核心)
;----------------------------------------------------------
TIMER0_ISR:
            PUSH    ACC              ; 保護 ACC 暫存器 (中斷標準動作)
            PUSH    PSW              ; 保護 PSW 暫存器 (中斷標準動作)
            
            MOV     TH0, NOTE_TH     ; 重載 TH0 (設定下次溢位時間/音高)
            MOV     TL0, NOTE_TL     ; 重載 TL0
            CPL     P2.7             ; 反轉蜂鳴器腳位，產生方波 (發聲核心)
            
            POP     PSW              ; 恢復 PSW
            POP     ACC              ; 恢復 ACC
            RETI                     ; 中斷返回 (CPU 回到被打斷的位置繼續執行)

;----------------------------------------------------------
; DELAY_NOTE：延遲副程式 (軟體耗時迴圈)
;----------------------------------------------------------
DELAY_NOTE:
D1:         MOV     R3, #20          ; R3 內層迴圈計數
D2:         MOV     R4, #248         ; R4 最內層迴圈計數
D3:         DJNZ    R4, D3           ; R4 減 1，非零跳回 D3 (跑 248 次)
            DJNZ    R3, D2           ; R3 減 1，非零跳回 D2 (跑 20 次)
            DJNZ    R2, D1           ; R2 減 1，非零跳回 D1 (總延遲長度由 R2 決定)
            RET                      ; 副程式返回

;----------------------------------------------------------
; LONG_DELAY：整首歌播完後的較長暫停
;----------------------------------------------------------
LONG_DELAY:
            MOV     R2, #250         ; 設定較長的延遲時間
            ACALL   DELAY_NOTE       ; 呼叫延遲副程式
            RET                      ; 副程式返回

;----------------------------------------------------------
; MELODY 樂譜表 (數據定義)
; 每三位元組代表一個音符: DB  TH, TL, Duration
;----------------------------------------------------------
MELODY:
; --- 數據部分略，僅為資料定義 ---
; ... (樂譜數據) ...

            DB  00H                 ; 結束標記

            END                     ; 程式結束

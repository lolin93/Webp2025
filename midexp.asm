;========================================================                       ; 說明：整體功能與硬體假設（P0 是 Active-Low，0=亮）
; Timer0 + Interrupt 跑馬燈（P0 Active-Low，含軟體上拉）                         ; 用 Timer0 中斷驅動狀態機
; 加入：身障者穿越鍵（延長下一次綠燈時長）                                       ; 按鍵可讓「下一段」綠燈延長
; 控制 LED0, LED1, LED2, LED7, LED11, LED15                                      ; A紅/黃/綠 與 B紅/黃/綠的對應腳位
; 晶振 22 MHz → 1 MC ≈ 0.545 µs                                                  ; 晶振與機器週期資訊（實際本程式以40次中斷≈1秒）
;========================================================

            ORG     0000H                                                   ; 0000H 為重置向量
            LJMP    START                                                   ; 上電後跳到 START 主程式

            ORG     000BH                                                   ; 000BH 是 Timer0 中斷向量位址
            LJMP    T0_ISR                                                  ; 發生 Timer0 中斷時跳到 T0_ISR

;---------------- 參數常數 -------------------------------
BASE_GRN    EQU     5                                                       ; 基本綠燈秒數 = 5 秒
EXT_ADD     EQU     3                                                       ; 延長秒數 = 3 秒（收到申請時加上）
DEB_THR     EQU     30                                                      ; 去抖閾值（以中斷次數計，30 次）
; 按鍵腳位：P3.2（低=按下）                                                     ; 指定鍵接點與極性
; 若要改腳位，改下面的 JB/JNB P3.2 即可                                         ; 換腳位只需改相應位測指令

;---------------- RAM 變數 -------------------------------
            ORG     0030H                                                   ; 變數從內部 RAM 0x30 開始配置
CNT40:      DS      1                                                       ; 中斷次數計數器（40≈1秒）
SEC:        DS      1                                                       ; 當前狀態已累積秒數
STATE:      DS      1                                                       ; 狀態碼（00~04）
PAT:        DS      1                                                       ; 目前 LED 輸出圖樣（Active-Low，0=亮）
GRN_DUR:    DS      1                                                       ; 本段綠燈目標秒數（會因申請被延長）
EXT_REQ:    DS      1                                                       ; 是否有延長申請（1=有，進綠燈時消耗）
BTN_CNT:    DS      1                                                       ; 去抖累積（連續按下計次）
BTN_LCK:    DS      1                                                       ; 單次鎖定（長按不重複觸發）

;---------------- 主程式 -------------------------------
            ORG     0100H                                                   ; 程式碼段（主邏輯）起點
START:
            MOV     SP,#60H                                                 ; 設定堆疊指標位置到 0x60
            MOV     CNT40,#00                                               ; 清 40 次計數器
            MOV     SEC,#00                                                 ; 清狀態秒數
            MOV     STATE,#00                                               ; 從狀態 S0 開始
            MOV     GRN_DUR,#BASE_GRN                                       ; 預設綠燈秒數 = 基本值
            MOV     EXT_REQ,#00                                             ; 尚無延長申請
            MOV     BTN_CNT,#00                                             ; 去抖計數清零
            MOV     BTN_LCK,#00                                             ; 鎖定標誌清零（可觸發）

            MOV     P0,#0FFH                                                ; 將 P0 全部寫 1（Active-Low 下為全滅/上拉）
            MOV     PAT,#0FFH                                               ; PAT 也設為全滅，保持一致

            ; Timer0 Mode 1 (16-bit)，約 1ms 中斷（實際載入值對應約25ms）       ; 此註解為說明，實際 4C/D0 對應約25ms
            ANL     TMOD,#0F0H                                              ; 清除 T0 的模式位（保留 T1 設定）
            ORL     TMOD,#01H                                               ; 設定 T0 為 Mode 1（16 位）
            MOV     TH0,#04CH                                               ; 計時器高位重載值（與 TL0 搭配得約25ms）
            MOV     TL0,#0D0H                                               ; 計時器低位重載值

            CLR     TF0                                                     ; 清 Timer0 溢出旗標
            SETB    ET0                                                     ; 允許 Timer0 中斷
            SETB    EA                                                      ; 開啟全域中斷
            SETB    TR0                                                     ; 啟動 Timer0 計時

MAIN:       SJMP    MAIN                                                    ; 主迴圈空轉，一切交由中斷處理

;---------------- Timer0 中斷（約 25 ms） -----------------
T0_ISR:
            MOV     TH0,#04CH                                               ; 重裝 Timer0 高位（維持固定節拍）
            MOV     TL0,#0D0H                                               ; 重裝 Timer0 低位

;--- 身障者穿越鍵去抖：P3.2 低=按下 ---
; 放開：計數清零、解除鎖定（可再次申請）
            JB      P3.2,Btn_Released                                       ; 若 P3.2=1（未按）→ 跳至釋放處理
; 低=按下：若尚未鎖定，累計去抖
            MOV     A,BTN_LCK                                               ; 讀鎖定旗標（長按時避免重複）
            JNZ     Btn_Done                                                ; 已鎖定就不再累計，直接結束按鍵處理
            INC     BTN_CNT                                                 ; 未鎖定且為按下 → 去抖累計 +1
            MOV     A,BTN_CNT                                               ; A = 去抖累積
            CJNE    A,#DEB_THR,Btn_Done                                     ; 還沒達門檻就先不觸發
            MOV     EXT_REQ,#01                                             ; 去抖通過 → 標記「下一段綠燈」需延長
            MOV     BTN_LCK,#01                                             ; 立刻上鎖（本次長按不會重複觸發）
            SJMP    Btn_Done                                                ; 跳到按鍵處理結束

Btn_Released:
            MOV     BTN_CNT,#00                                             ; 釋放時清去抖累積（重新計）
            MOV     BTN_LCK,#00                                             ; 釋放時解除鎖定（允許下次觸發）
Btn_Done:

;--- 1秒時基 ---
            INC     CNT40                                                   ; 每次中斷 +1（約25ms）
            MOV     A,CNT40                                                 ; A = 當前累計次數
            CJNE    A,#40,No1s                                              ; 未達 40 次（約 1 秒）就略過
            MOV     CNT40,#00                                               ; 達 40 次 → 歸零
            INC     SEC                                                     ; 秒數 +1（此狀態經過秒數）
No1s:

;---------------- 黃燈閃爍（每秒翻轉） -----------------
; STATE=02（B 黃=LED11）與 STATE=04（A 黃=LED1）期間，
; 在 SEC<3 時每秒切換亮/滅（Active-Low → XRL 對應位元）
            MOV     A,STATE                                                 ; A = 目前狀態
            CJNE    A,#02,ChkS4                                             ; 若非 S2 → 檢查 S4

; --- S2：B 黃閃爍（LED11=P0.4） ---
            MOV     A,SEC                                                   ; A = 此狀態已過秒數
            CJNE    A,#3,B_BlinkDo                                          ; 若 <3 秒 → 進行閃爍翻轉
            SJMP    BlinkDone                                               ; 若已滿 3 秒 → 不再閃爍

B_BlinkDo:  MOV     A,PAT                                                   ; 取目前輸出圖樣
            XRL     A,#00010000B                                           ; 反轉 bit4（P0.4，B 黃），達到閃爍
            MOV     PAT,A                                                   ; 更新 PAT
            SJMP    BlinkDone                                               ; 跳至閃爍段落結束

ChkS4:      CJNE    A,#04,BlinkDone                                        ; 若非 S4 也非 S2 → 不用閃爍

; --- S4：A 黃閃爍（LED1=P0.1） ---
            MOV     A,SEC                                                   ; A = 此狀態已過秒數
            CJNE    A,#3,A_BlinkDo                                          ; 若 <3 秒 → 進行閃爍翻轉
            SJMP    BlinkDone                                               ; 已滿 3 秒 → 不再閃爍

A_BlinkDo:  MOV     A,PAT                                                   ; 取目前輸出圖樣
            XRL     A,#00000010B                                           ; 反轉 bit1（P0.1，A 黃），達到閃爍
            MOV     PAT,A                                                   ; 更新 PAT
BlinkDone:                                                                 ; 閃爍段落結束（落點標籤）

;---------------- 狀態機 -------------------------------
            MOV     A,STATE                                                 ; A = 目前狀態
            CJNE    A,#00,S1                                                ; 若不是 S0 → 直接按狀態流程往下

;=== 狀態 0：設定 A紅、B綠；並決定「下一段」綠燈秒數 ===
            MOV     A,#0FFH                                                 ; A = 全 1（全滅基底）
            ANL     A,#10111110B                                           ; 清 bit0(A紅) 與 bit6(B綠) → 兩燈亮
            MOV     PAT,A                                                   ; 寫入當前輸出圖樣

            ; 決定 S1（B 綠等待）要等多久：若有申請，延長
            MOV     A,EXT_REQ                                               ; 讀是否有延長申請
            JZ      NoExt_S1                                                ; 沒申請 → 用基本秒數
            MOV     GRN_DUR,#(BASE_GRN+EXT_ADD)                             ; 有申請 → 綠燈 = 基本 + 延長
            MOV     EXT_REQ,#00                                             ; 申請已消耗 → 清零
            SJMP    SetS1                                                   ; 進入 S1 前的收尾

NoExt_S1:   MOV     GRN_DUR,#BASE_GRN                                       ; 無申請 → 用基本秒數

SetS1:      MOV     SEC,#00                                                 ; 切入新狀態 → 秒數歸零
            MOV     STATE,#01                                               ; 進入 S1（B 綠等待）
            SJMP    Update                                                  ; 輸出更新並結束中斷

S1:         CJNE    A,#01,S2                                                ; 若不是 S1 → 檢查 S2
;=== 狀態 1：B 綠等待 GRN_DUR 秒 → B 黃亮（進入 S2：黃閃） ===
            MOV     A,SEC                                                   ; A = 已過秒數
            CJNE    A,GRN_DUR,Update                                        ; 未到目標秒數 → 持續等
            MOV     SEC,#00                                                 ; 到時間 → 歸零
            MOV     A,PAT                                                   ; 取目前輸出
            ORL     A,#01000000B                                           ; 設 bit6=1 → B 綠熄
            ANL     A,#11101111B                                           ; 清 bit4=0 → B 黃亮
            MOV     PAT,A                                                   ; 寫回輸出圖樣
            MOV     STATE,#02                                               ; 進入 S2（B 黃閃）
            SJMP    Update                                                  ; 更新輸出後離開中斷

S2:         CJNE    A,#02,S3                                                ; 若不是 S2 → 檢查 S3
;=== 狀態 2（B 黃閃 3 秒）：結束後 → B紅、A綠 ===
            MOV     A,SEC                                                   ; A = 已過秒數
            CJNE    A,#3,Update                                             ; <3 秒 → 繼續閃爍
            MOV     SEC,#00                                                 ; =3 秒 → 切換燈色
            MOV     A,PAT                                                   ; 取目前輸出
            ORL     A,#00010001B                                           ; 設 bit4=1(B黃熄)、bit0=1(A紅熄)
            ANL     A,#01111011B                                           ; 清 bit7=0(B紅亮)、bit2=0(A綠亮)
            MOV     PAT,A                                                   ; 寫回輸出

            ; 決定 S3（A 綠等待）要等多久：若有申請，延長
            MOV     A,EXT_REQ                                               ; 讀延長申請旗標
            JZ      NoExt_S3                                                ; 無申請 → 基本秒數
            MOV     GRN_DUR,#(BASE_GRN+EXT_ADD)                             ; 有申請 → 基本 + 延長
            MOV     EXT_REQ,#00                                             ; 申請已用 → 清零
            SJMP    SetS3                                                   ; 進入 S3 前收尾

NoExt_S3:   MOV     GRN_DUR,#BASE_GRN                                       ; 無申請 → 用基本值
SetS3:      MOV     STATE,#03                                               ; 進入 S3（A 綠等待）
            SJMP    Update                                                  ; 更新輸出後離開中斷

S3:         CJNE    A,#03,S4                                                ; 若不是 S3 → 檢查 S4
;=== 狀態 3：A 綠等待 GRN_DUR 秒 → A 黃亮（進入 S4：黃閃） ===
            MOV     A,SEC                                                   ; A = 已過秒數
            CJNE    A,GRN_DUR,Update                                        ; 未到目標 → 繼續
            MOV     SEC,#00                                                 ; 到目標 → 歸零
            MOV     A,PAT                                                   ; 取目前輸出
            ORL     A,#00000100B                                           ; 設 bit2=1 → A 綠熄
            ANL     A,#11111101B                                           ; 清 bit1=0 → A 黃亮
            MOV     PAT,A                                                   ; 寫回輸出
            MOV     STATE,#04                                               ; 進入 S4（A 黃閃）
            SJMP    Update                                                  ; 更新輸出

S4:                                                                         ; 落點：目前狀態為 S4
;=== 狀態 4（A 黃閃 3 秒）：結束後 → 回到初始循環（A紅、B綠） ===
            MOV     A,SEC                                                   ; A = 已過秒數
            CJNE    A,#3,Update                                             ; <3 秒 → 繼續閃爍
            MOV     SEC,#00                                                 ; =3 秒 → 切回起始燈態
            MOV     A,#0FFH                                                 ; 基底全滅
            MOV     PAT,A                                                   ; PAT 也先全滅
            ; 回到起始：A紅、B綠（會在 S0 設定並重新決定下一段綠燈秒數）
            ANL     A,#10111110B                                           ; 清 bit0/bit6 → A紅亮、B綠亮
            MOV     PAT,A                                                   ; 寫回輸出圖樣
            MOV     STATE,#01                                              ; 直接進入 S1（B 綠等待），等同回循環
            SJMP    Update                                                  ; 更新輸出後結束本次中斷

Update:
            MOV     P0,#0FFH                                               ; 先將 P0 全部寫 1（軟體上拉，避免殘影）
            MOV     A,PAT                                                  ; A = 目前要輸出的圖樣
            MOV     P0,A                                                   ; 輸出到 P0（Active-Low，0=亮）
            RETI                                                           ; 中斷返回，恢復主程式（此程式主迴圈空轉）

            END                                                            ; 組譯結束（連結器識別終點）

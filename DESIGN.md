# DESIGN.md — In-Memory KV Store (類 Redis)

> 這是一份「活的」設計文件,不是規格書。
> 它記錄:已拍板的決定 + 為什麼 / 還沒決定、留待自己 defend 的問題。
> 每次推進就更新「進度日誌」。換聊天室時把這份貼給 Claude,他就能接上。

---

## 專案定位

一個從零手寫的 in-memory key-value store(本質是「網路化的 hash map」/ 類 Redis)。
目的:補足工程實作經驗,支撐 Google Taiwan new grad 面試的系統設計與行為問題。

**核心心智模型:KV store = 一個能被網路存取、能扛並發、崩潰後能復原的 hash map server。**
核心資料結構(hash map)簡單是特徵不是缺點——腦力全放在下面四層真實系統的難點上。

### 這個專案真正的價值(面試要 defend 的四層)
1. **崩潰 / 持久化** — 斷電後,已回報成功的資料還算不算數?(WAL / fsync / recovery)
2. **並發下的正確性** — 同時操作同一 key 的順序如何定義、client 能否預期?(consistency)
3. **資源有限** — 記憶體滿了踢誰?(LRU/LFU 淘汰)連線暴增撐不撐得住?
4. **時間** — TTL 過期由誰來盯?(lazy vs active)

---

## 鐵律
- **不放任何 AI 代寫的 code。** Claude 只當 reviewer / 出題 / 追問設計決定。每一行自己寫。
- 每個設計決定都要能 defend 到底(面試官會追問到 fsync 時機那種深度)。
- 深度 > 廣度:功能少沒關係,每個功能都要能被追問到底。

---

## 技術棧
- 語言:**C++** — 最熟、系統底層在 Google 加分、半年內不宜同時學新語言
- 單機(主從複製列為 optional stretch,沒時間不做)

---

## 里程碑(真正工期:現在 → 8 月底;9 月後進世界總決賽模式只收尾)

| 里程碑 | 時間 | 內容 | 目標 |
|--------|------|------|------|
| **M1** | 7 月 | 核心引擎 + 網路層 | 端到端跑通 GET/SET/DEL(先求通,不求漂亮) |
| **M2** | 7 月底–8 月初 | TTL 過期 + 並發 | |
| **M3** | 8 月中 | 持久化(WAL → snapshot → crash recovery) | 面試含金量最高,review 最嚴 |
| **M4** | 8 月底 | benchmark + 設計文件 + README | QPS / p99、trade-off 寫清楚 |
| 收尾 | 9 月後 | 只維護,不加大功能 | World Finals: 11/15–11/20 Dubai |

---

## 已拍板的設計決定

- **語言:C++** — 理由見技術棧。
- **wire protocol:** M1 先自訂簡單協議(先理解,不急著相容 RESP)。之後再考慮相容 RESP,好處是能直接用 redis-benchmark / memtier_benchmark 壓測。
- **並發模型:單執行緒 event loop(epoll),非多執行緒 + 鎖。**
  - 理由(要能這樣講):**I/O 層要並行,計算層不必並行。**
  - 動 hash map 快到奈秒級,為它加鎖,鎖的開銷 + 複雜度 > 省下的時間,不划算 → 計算層單執行緒,從根本消滅 data race。
  - 連線多、每條 I/O 慢 → 用 epoll event loop 在單一 thread 上「非阻塞地多路複用」所有連線,不會因某條連線慢就卡住其他人 → I/O 層照樣高度並行。
  - 一句話:**I/O 並行靠 epoll,計算串行靠單執行緒避開鎖。**
  - 埋伏筆:日後若 benchmark 顯示 I/O 系統呼叫本身成瓶頸,再把 I/O 拆多執行緒,但 hash map 存取永遠單執行緒。
  - ⚠️ 常見誤解(自己踩過):單執行緒 ≠ 放棄平行化。它是「用不需要鎖的方式(epoll)平行」。

---

## OPEN — 待在對應里程碑自己 defend 的設計題

- **TTL 刪除策略(M2):** lazy(存取時才檢查過期)vs active(定時掃描)vs 兩者混用。各自代價?(Redis 兩個都用,想清楚為什麼)
- **並發正確性語義(M2):** 同一 key 併發操作的順序如何定義並讓 client 可預期?
- **持久化策略(M3):** WAL 與 snapshot 的取捨;fsync 時機;partial write 怎麼辦;recovery 如何 replay。
- **記憶體淘汰(未排,資源那層):** 滿了踢誰?LRU vs LFU,為什麼。

---

## 測試策略(client 就是程式,不是真人)
- **並發壓測(情境三):** redis-benchmark / memtier_benchmark 灌大量並發連線 → 看崩不崩、延遲多少 → 順便當 M4 benchmark 數據。
- **並發正確性(情境二):** 多 thread 對同一 key 各做 N 次 INCR,結束後應恰為 threads×N,少了就有 race。
- **crash-recovery(情境一):** 灌 1000 筆 SET → `kill -9` server(模擬斷電)→ 重啟 → 檢查資料還在不在。**主動製造崩潰。**
- **TTL(情境四):** SET 設 2 秒過期 → GET 有 → sleep 3 → GET 應為空。

---

## 進度日誌
(每次推進追加一行:日期 + 做了什麼 + 遇到什麼設計選擇)

- 2026-07-01 — 確立專案定位與四大難點;拍板 C++、單執行緒 event loop 並發模型。M1 待開工。

- 2026-07-26 — M2 用 epoll

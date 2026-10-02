# KVstore

以 C++ 實作的 Linux TCP 記憶體型鍵值儲存系統。使用 `std::unordered_map` 儲存資料，提供 `SET`、`GET`、`DEL` 指令，並以單執行緒 `epoll` 事件迴圈及非阻塞客戶端 socket 處理多條連線。

專案重點包括 TCP 串流解析、每條連線的狀態管理、部分送出處理，以及連線中斷時的資源清理。

## 已實作功能

- **鍵值操作**：新增、覆寫、查詢與刪除字串鍵值。
- **多連線服務**：以 `epoll` 監聽事件，交錯處理各客戶端的 I/O。
- **分段與批次命令**：保留尚未完整接收的命令，並依序處理同一批資料中的多條命令。
- **非阻塞送出**：保存尚未送出的回覆，透過 `EPOLLOUT` 繼續送出，送完後取消可寫事件。
- **連線狀態管理**：集中管理未完成命令、待送回覆及關閉標記。
- **斷線防護**：使用 `MSG_NOSIGNAL` 避免送出時的 `SIGPIPE` 終止服務程序。
- **命令列客戶端**：輸入指令並顯示伺服器回覆。

## 編譯與執行

### 環境

- Linux，或 Windows 下的 WSL Linux 環境。
- 支援 C++17 的 GNU g++。
- 使用 Linux socket、`epoll` 與 `fcntl` API，無額外第三方函式庫依賴。

在專案根目錄編譯：

```bash
mkdir -p build
g++ -std=c++17 -O2 -Wall -Wextra server.cpp -o build/server
g++ -std=c++17 -O2 -Wall -Wextra client.cpp -o build/client
```

啟動伺服器：

```bash
./build/server
```

再開啟另一個終端機，啟動客戶端：

```bash
./build/client
```

伺服器預設監聽 `0.0.0.0:8080`，客戶端預設連線至 `127.0.0.1:8080`。兩者目前皆使用程式內固定的位址與埠號；可開啟多個客戶端連至同一個伺服器。結束程序時使用 `Ctrl+C`。

## 指令與通訊格式

每條命令以 LF（`\n`）結尾，指令與參數之間以單一空白分隔；指令名稱使用大寫。命令列客戶端會自動在輸入後補上 LF。

| 指令 | 行為 | 回覆 |
| --- | --- | --- |
| `SET key value` | 新增鍵值；key 已存在時覆寫 value | `SET successfully` |
| `GET key` | 查詢指定 key | 對應的 value |
| `DEL key` | 刪除指定 key | `DEL successfully` |

`GET` 或 `DEL` 的 key 不存在時，回覆 `* KEY doesn't exist`。未知指令或參數數量不符時，回覆 `fail`。每個回覆同樣以 LF 結尾。

以下是指令與對應回覆，`>` 表示輸入的命令：

```text
> SET language cpp
SET successfully
> GET language
cpp
> SET language cplusplus
SET successfully
> GET language
cplusplus
> DEL language
DEL successfully
> GET language
* KEY doesn't exist
```

協定使用不含空白、換行或 NUL 的文字鍵值，不支援引號、跳脫字元與二進位資料。請使用 LF，不以 CRLF 作為命令結尾。伺服器也可接收同一條連線連續送入的多條命令，依接收後的解析順序執行並產生回覆。

## 系統架構

```text
TCP 客戶端
    │
    ▼
epoll 事件迴圈
    │
    ├─ 新連線：accept → 設定非阻塞 → 註冊 EPOLLIN
    │
    ├─ 可讀：recv → 拼接片段 → 依 LF 切分命令
    │                          │
    │                          ▼
    │                    解析與參數檢查
    │                          │
    │                          ▼
    │                  unordered_map 鍵值操作
    │                          │
    │                          ▼
    │                    產生並嘗試送出回覆
    │
    └─ 可寫：續送剩餘回覆 → 送完取消 EPOLLOUT
```

鍵值資料由單一執行緒存取，不需要以鎖保護資料表。各連線的 I/O 由事件迴圈交錯處理，不為每個客戶端建立獨立執行緒。

### 接收與命令邊界

TCP 是位元組串流，單次 `recv` 可能只有半條命令，也可能包含多條命令。接收流程以 LF 辨識完整命令，保留尚未結束的片段，等待後續資料補齊。讀取遇到 `EAGAIN` 時，回到事件迴圈等待下一次就緒通知。

### 部分送出

回覆產生後先嘗試立即送出。若 `send` 只接受部分資料，就從實際送出位置繼續；遇到 `EAGAIN` 時，保留殘餘內容並註冊 `EPOLLOUT`。待 socket 可寫後再續送，清空待送資料時取消可寫事件。

### 連線狀態

每條連線以 `Connection` 保存三項狀態，並透過檔案描述元索引：

| 狀態 | 用途 |
| --- | --- |
| `left_command` | 尚未接收完整的命令片段 |
| `wait_reply` | 已產生但尚未送完的回覆 |
| `can_remove` | 接收端已結束，待回覆送完後關閉的標記 |

接收端讀到 EOF 時，會先處理已取得的完整命令，並在待送回覆送完後關閉連線。關閉流程清除緩衝、移除 `epoll` 註冊並釋放 socket。送出使用 `MSG_NOSIGNAL`，使斷線時的寫入錯誤交由程式處理。

### 緩衝限制

- 未完成命令累積超過 **1000 bytes** 時結束接收。此檢查目前針對未完成命令，完整長命令仍可能繞過限制。
- `send` 遇到 `EAGAIN` 後，若剩餘待送資料超過 **6500 bytes**，便關閉該連線。
- 待送殘餘上限不限制命令批次處理時產生的所有暫存資料，因此不等同整個服務的總記憶體上限。

## 目前範圍

資料只保存在記憶體中，重啟後不保留；目前尚未實作 TTL、WAL、快照及崩潰復原，也不相容 Redis RESP。

專案仍在改善輸入邊界與連線生命週期：NUL 輸入、EOF 與待送回覆交互、檔案描述元耗盡，以及關閉後同一事件的後續處理仍有待修正。命令列客戶端提供基本互動操作，尚未以完整行緩衝重組接收到的回覆。

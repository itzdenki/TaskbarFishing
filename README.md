# Taskbar Fishing — prototype 0.1

Game desktop nhỏ cho Windows 10/11, viết bằng **C++20 + CMake + raylib 5.5**.
Mở game là nhân vật tự thả câu. Sprite được đóng gói sẵn; không cần tài khoản hay kết nối mạng khi chơi.

## Chạy ngay

Bản đã build trong workspace:

- **`out/TaskbarFishing.exe` — bản một file, chỉ cần sao chép file này để chơi.**
- `out/TaskbarFishing/TaskbarFishing.exe` — mở trực tiếp.
- `out/TaskbarFishing-Windows-x64.zip` — giải nén rồi mở executable.
- `build/TaskbarFishing.exe` — executable trong thư mục build.

Bản Windows đã nhúng toàn bộ artwork, 24 nền theo giờ, icon, cấu hình Discord mặc định
và giấy phép thư viện vào `.exe`. Game đọc ảnh trực tiếp từ bộ nhớ, không giải nén assets ra ổ đĩa.
Không cần thư mục `sprite/`, file cấu hình hay DLL đi kèm. Save vẫn ghi riêng tại
`%LOCALAPPDATA%/TaskbarFishing/save.txt`, nên thay executable giữ nguyên tiến trình trên máy.

### Nếu máy khác không mở được

Bản Windows mặc định dùng **OpenGL 1.1** cho game 2D và không yêu cầu MSAA 4×,
thay cho OpenGL 3.3 của bản cũ. Cần Windows 10/11 x64 và driver đồ họa có OpenGL;
không đảm bảo chạy được khi máy chỉ có Microsoft Basic Display Adapter hoặc trong mọi máy ảo.
Bản build vẫn chỉ dùng DLL hệ thống Windows, không cần bộ DLL của MSYS2.

Mỗi lần mở, game ghi `%LOCALAPPDATA%/TaskbarFishing/startup.log`, gồm bước khởi động,
thông tin renderer và lỗi. Khi có lỗi C++ sẽ hiện hộp thoại cùng đường dẫn log;
lỗi native có thể ghi mã ngoại lệ và module trước khi Windows Error Reporting tiếp quản.
Nếu vẫn không mở được, lấy log ngay sau lần lỗi, kèm **Faulting module name** và
**Exception code** trong Event Viewer → Windows Logs → Application.
Không phải mọi lỗi đều ghi được log (ví dụ Windows loader không vào được chương trình).

Source raylib 5.5 được vá có kiểm tra qua `cmake/PatchRaylibStartup.cmake`:
không truy vấn monitor với cửa sổ null và không khởi tạo renderer khi platform thất bại.

Cửa sổ borderless mặc định **512 × 176**: cảnh hồ **512 × 144 (32:9)** và thanh Last catch cao 32 px.
Bấm **^** ở góc trên bên phải khung câu cá để mở **Fish Box**, **Collection**, thông tin cá và
nút mở cây Stats. Bảng bung lên phía trên (cửa sổ **512 × 428**); bấm **v** hoặc **Esc** để thu gọn.
Bấm biểu tượng **bánh răng** để mở/đóng **Settings** (cửa sổ **512 × 548**, năm tab General / Window / Display /
Fishing / Hotkeys): Discord, khởi động cùng Windows, tooltip, đồng hồ 12h, always-on-top, khóa/nhớ vị trí, Dock và
khoảng cách taskbar, độ mờ (kèm độ mờ khi rảnh), tỉ lệ cửa sổ 75–150%, FPS/FPS nền/VSync, HUD và hiệu ứng,
auto sell, tự khóa cá Special/Rare, hành vi khi thùng đầy, tạm dừng khi mất focus, bật/tắt phím tắt.
Chi tiết từng mục ở [assets/SETTINGS.md](assets/SETTINGS.md). Settings và bảng Fish Box/nâng cấp mở luân phiên.
Kéo vùng khung câu cá (trừ hai nút và thanh Last) để di chuyển. **Dock** đưa game về phía dưới màn hình hiện tại.
Mỗi lần khởi động, game trở về chế độ chỉ hiện câu cá.
Game vẫn cập nhật khi mất focus hoặc thu nhỏ (giảm rendering xuống 15 FPS lúc thu nhỏ).
Chưa nhúng vào taskbar và chưa có tiến trình câu khi game đã đóng hoặc máy sleep.

Game thật chọn `assets/background/hour_HH.png` theo **giờ địa phương và múi giờ hiện tại trên Windows**,
kể cả giờ mùa hè. Đồng hồ và cấu hình múi giờ được đọc lại mỗi 0,25 giây; thay đổi giờ/múi giờ
trên Windows cập nhật cả nền và xác suất trong cùng lần đọc, không cần mở lại game.
Góc trái hiện `HH:mm` cùng tỷ lệ và mức câu của giờ đó; múi giờ đầy đủ nằm trong Settings và Stats. Nền đổi mượt trong 0,8 giây.
Ví dụ khi chưa nâng Accuracy: **18h = nền 18h + 88% thành công**, **13h = nền 13h + 28%**.
Việc chuyển giờ dùng API múi giờ của Windows, xem [Microsoft: thông tin múi giờ](https://learn.microsoft.com/en-us/windows/win32/api/timezoneapi/nf-timezoneapi-gettimezoneinformation).

## Demo nền theo giờ

Mở `scripts/BackgroundDemo.cmd`, hoặc chạy:

```powershell
./build/TaskbarFishing.exe --background-demo
```

Cửa sổ demo **1024 × 288 (32:9)** dùng 24 sprite sheet `assets/background/hour_00.png` đến `hour_23.png`.
Bắt đầu ở **00h**, cứ **15 giây đổi một giờ**, chuyển ảnh mượt trong 0,8 giây;
hết 23h quay về 00h, một vòng kéo dài **6 phút**. Giữ tỉ lệ cảnh khi thay đổi kích thước cửa sổ.
Ảnh được đọc từ tài nguyên nhúng trong executable Windows; build không có tài nguyên nhúng
(ví dụ UI test) vẫn có thể đọc từ thư mục cạnh executable.

| Phím | Thao tác |
| --- | --- |
| Space | Tạm dừng / tiếp tục |
| ← / → | Xem giờ trước / sau, đặt lại đếm ngược 15 giây |
| T | Chuyển giữa demo tăng tốc và nền theo giờ địa phương trên máy |
| R | Chạy lại từ 00h |
| H | Ẩn / hiện nhãn giờ và hướng dẫn để xem trọn ảnh |
| Esc | Đóng demo |

Đây là cửa sổ xem trước cảnh nền, chạy độc lập với vòng chơi, save và Discord Presence.
Game thật cũng dùng bộ nền này, với thời gian thực; tốc độ 15 giây/giờ chỉ dùng trong demo.
Demo giữ tối đa ba texture (ảnh trước, hiện tại và tiếp theo), không nạp cả bộ 24 ảnh vào GPU cùng lúc.
Kiểm tra tự động: `background_clock` kiểm tra mốc 15 giây/tạm dừng/vòng 24 giờ;
`background_demo_smoke` nạp và render đủ 24 ảnh, kiểm tra 23h→00h, xuất ảnh ở `build/captures/background/`.

## Discord Rich Presence

Game có tích hợp Rich Presence qua [Discord RPC/IPC](https://docs.discord.com/developers/topics/rpc).
Hiện trạng thái câu cá, cấp cần, số cá/sức chứa Fish Box và thời gian của phiên chơi.
Khi thùng đầy và cá đang chờ chỗ, presence báo tạm nghỉ. Không cần bot, token hay Client Secret.

Application ID **1547528683689087106** được nhúng trực tiếp trong `src/Game.cpp`.
Game không đọc hay cần file `discord_app_id.txt`.
Mở Discord desktop rồi khởi động game. Bật chia sẻ hoạt động trong cài đặt Discord
nếu presence chưa xuất hiện. Tên game lấy từ application trên Developer Portal.

Presence cập nhật tối đa một lần mỗi 15 giây, nên các animation ngắn có thể không xuất hiện.
Thời gian chơi giữ nguyên khi kết nối lại. Nếu Discord chưa mở hoặc bị đóng, game vẫn chơi bình thường
và thử kết nối lại sau khoảng 5 giây. Khi thoát, game gửi lệnh xóa presence và đóng kết nối.
Có thể tắt riêng một phiên bằng `--no-discord`.
Smoke test luôn tắt Discord; test IPC sử dụng pipe riêng, không đổi hoạt động trên tài khoản thật.
Game hỗ trợ avatar qua `assets.large_image` và chú thích `large_text`.
Application ID của bản này là **1547528683689087106**; file `discord_presence.json`
cạnh executable hiện dùng URL CDN của **App Icon đã tải lên Discord**.
Không cần tải lại vào Rich Presence Art Assets cho cấu hình này.
Nếu đổi App Icon lần nữa, cập nhật URL ảnh mới trong cấu hình.
Ảnh cá cũ và hướng dẫn dùng asset key thay thế nằm trong [sprite/discord/README.md](sprite/discord/README.md).
Có thể đổi key hoặc dùng URL ảnh HTTPS công khai trong cấu hình; khởi động lại game sau khi sửa.
Xem [Discord: cấu hình ảnh Rich Presence](https://docs.discord.com/developers/discord-social-sdk/development-guides/setting-rich-presence).

Khi phát hành, người chơi chỉ cần mở Discord và game; Application ID đã nằm trong mã game.
Build nhúng cấu hình ảnh từ `build/discord_presence.json`; không cần mang theo file này.
File `discord_presence.json` cạnh executable có thể ghi đè cấu hình ảnh nhúng.
Build lại giữ cấu hình ảnh đã điền. Thư viện JSON được link vào executable, không cần thêm DLL.

Icon Windows dùng ảnh người dùng cung cấp ở `sprite/app-icon.png`, chuyển thành ICO với
7 kích thước 16–256 px và nhúng trực tiếp vào `.exe`. Explorer, taskbar và cửa sổ demo
đều dùng cùng icon; không cần file ảnh rời để icon hoạt động. Khi đổi ảnh nguồn, chạy
`python scripts/MakeIcon.py` (cần Pillow) rồi build lại. Build thông thường dùng ICO có sẵn.

## Cách chơi

1. Chờ cá cắn câu (5–12 giây ở Rod Lv.1).
2. Nhân vật tự kéo cá trong 1 giây. Cá có thể **sổng** (xem xác suất bên dưới); khi đó
   panel hiện **IT GOT AWAY!** khoảng 1,6 giây rồi thả câu lại.
3. Cá kéo lên thành công hiện khoảng 2 giây rồi **tự vào Fish Box** — không cần bấm gì.
4. Bấm **^** mở **Fish Box** để xem, bán từng con, bán tất cả hoặc khóa cá muốn giữ.
5. Bấm **Stats** hoặc **U** để dùng tiền nâng cần, thùng và các chỉ số câu cá.
6. Mở **Collection** để xem FishDex và trọng lượng kỷ lục từng loài.

### Fish Box (inventory dạng lưới)

Fish Box là lưới ô kiểu inventory: mỗi con cá là một ô có icon và viền màu theo rarity
(Common trắng, Uncommon xanh lá, Rare xanh dương). Ô ngoài sức chứa hiện khóa mờ;
nâng Fish Box mở thêm ô. Cá to hơn trong cùng loài vẽ icon lớn hơn.

- **Rê chuột** vào ô: tooltip tên, rarity, cân nặng, giá bán.
- **Click trái**: chọn ô; cột bên phải hiện chi tiết cùng nút **Sell** và **Lock/Unlock**.
- **Click phải**: bán ngay con cá trong ô.
- **Alt + click trái**: khóa/mở khóa. Cá khóa có chấm vàng ở góc, không bán được bằng
  click phải, nút Sell hay **Sell All**.
- **Sell All**: bán mọi cá chưa khóa, nút hiện tổng tiền thu được.

Fish Box bắt đầu với **30 ô** (3 hàng, mỗi hàng 10). Khi nâng Fish Box, thêm hàng phía dưới; lăn chuột trên lưới để xem. Khi đầy, con cá vừa câu treo trên móc và game **tạm dừng**
cho đến khi bạn bán bớt (panel báo FISH BOX FULL). FishDex ghi nhận ngay khi bắt được cá;
bán cá không xóa collection hay kỷ lục.

**Auto Sell Common** trong **Settings (bánh răng)** mặc định OFF. Khi ON, Bluegill, Carp và Old Boot bán ngay khi kéo lên
thay vì chiếm ô; Uncommon/Rare vẫn vào Fish Box. Loài mới và kỷ lục vẫn được ghi nhận khi tự bán.

UI dùng tông xanh đêm, điểm nhấn bạc hà/vàng ấm, bảng và nút bo góc, tab đang chọn nổi bật.
Cảnh câu có bờ cây, cầu gỗ, mặt trời/trăng và phản chiếu thay đổi theo giờ. Fish Box có cột chi tiết riêng;
Collection hiện số loài đã khám phá và kỷ lục theo từng hàng.
Chữ dùng Segoe UI có sẵn trong thư mục Fonts của Windows (không đóng gói font hệ thống);
nếu không tìm thấy, tự dùng font raylib. Bộ ảnh và metadata hiện dùng trong game nằm ở `assets/`.
Mỗi nền có 64 frame, 125 ms/frame, vòng lặp 8 giây; đồng hồ nền độc lập với trạng thái câu.
Nhân vật dùng 7 animation; cần câu, phao, hiệu ứng nước và UI đọc frame/anchor từ `assets/assets.json`.
Cá vẫn dùng hình vẽ hiện có vì bộ asset chưa có sprite từng loài. Game nạp ảnh từ thư mục cạnh executable,
không phụ thuộc thư mục đang chạy. Bản Windows ưu tiên tài nguyên nhúng; build không có
tài nguyên nhúng dùng ảnh rời, nếu thiếu ảnh thì giữ nền trước đó hoặc vẽ cảnh dự phòng.
Muốn thay artwork của bản Windows, sửa ảnh nguồn rồi build lại; ảnh gốc được giữ nguyên.

| Phím (khi game có focus) | Thao tác |
| --- | --- |
| S / L | Bán / khóa con cá đang chọn trong Fish Box |
| B / C | Mở Fish Box / Collection; bấm lại để xem Catch trong bảng mở rộng |
| R / F | Nâng Rod / Fish Box nếu đủ tiền |
| U | Mở cây Stats / trở về hồ |
| A | Bật/tắt Auto Sell Common |
| T / D | Bật/tắt always-on-top / Dock |
| Esc | Rời Stats về bảng trước đó; ngoài Stats thì thu gọn bảng phụ |
| Shift+F12 / Shift+F11 | Đưa cửa sổ về màn hình chính / về tỉ lệ 100% và độ mờ 100% (luôn hoạt động) |
| Alt+F4 | Lưu và thoát |

Các phím một chữ ở trên có thể tắt trong Settings → Hotkeys; Esc và Shift+F11/F12 luôn hoạt động.

## Cây Stats

**Stats** mở cửa sổ **960 × 640**, gồm cây bên trái và chi tiết nâng cấp bên phải.
Có **28 ô, 68 lần mua**, chia bốn nhánh màu: thiết bị (vàng), tốc độ (xanh),
độ chính xác (cam), may mắn và giá bán (tím). Chọn ô để xem hiệu ứng hiện tại → kế tiếp,
điều kiện và giá; bấm **Upgrade** mới trừ tiền. Các ô chưa mở khóa hoặc thiếu tiền không mua được.
Ô đã đạt tối đa có dải cấp tô kín; chấm xanh chỉ ô đủ tiền mua.

Kéo cây để di chuyển, lăn chuột để phóng to/thu nhỏ. **U**, **Esc** hoặc **Back to lake**
trở về bảng và vị trí cửa sổ trước đó. Game vẫn câu và cập nhật giờ/múi giờ khi xem Stats;
nếu thùng đầy, thanh dưới báo cần trở về hồ bán cá. Có thể chạy `TaskbarFishing.exe --stats`
để mở thẳng bảng này. Phím **R/F** vẫn nâng thiết bị qua cùng hệ thống mua.

| Chuỗi | Số ô × cấp | Giá cơ sở từng ô | Hiệu ứng mỗi cấp | Điều kiện ô đầu |
| --- | --- | --- | --- | --- |
| Rod Lv.2–5 | 4 × 1 | 100 / 225 / 450 / 800 | Xem bảng cần bên dưới | Không |
| Fish Box Lv.2–5 | 4 × 1 | 75 / 175 / 350 / 600 | 40 / 50 / 60 / 70 ô cá | Không |
| Patient rhythm I–III | 3 × 3 | 150 / 650 / 2000 | Giảm 2% thời gian chờ | Rod Lv.2 |
| Quick reel I–III | 3 × 3 | 100 / 450 / 1500 | Giảm 5% thời gian kéo | Rod Lv.2 |
| Steady hands I–VI | 6 × 3 | 150 / 300 / 600 / 1000 / 1600 / 2400 | Giảm phần thất bại 2% ở I–II, 3% ở III–VI | Rod Lv.2 |
| Lucky waters I–IV | 4 × 3 | 150 / 450 / 1100 / 2400 | +5% trọng số cá Rare | Rod Lv.3 |
| Good business I–IV | 4 × 3 | 125 / 350 / 900 / 1800 | +2,5% giá bán | Fish Box Lv.2 |

Ô tiếp theo yêu cầu ô trước trong cùng chuỗi đạt cấp tối đa. Với ô ba cấp, giá mỗi lần mua
là **1× / 2× / 4× giá cơ sở**. Có thể hoàn thành mọi nhánh; nâng cấp vĩnh viễn, không hoàn điểm.
Tổng chi phí cây là **$130.000**. Mục tiêu tiến trình khoảng 10–20 giờ, cần tiếp tục cân bằng
qua chơi thực tế vì phụ thuộc giờ chơi, thứ tự mua và tần suất bán cá.

Khi nâng tối đa:

- Chờ giảm thêm **18%**, nhân với hệ số của cần; Rod Lv.5 chờ trung bình **4,182 giây**.
- Kéo cá còn **0,55 giây**. Nâng tốc độ có tác dụng ở lượt Waiting/Reeling tiếp theo,
  không thay đổi bộ đếm của giai đoạn đang chạy.
- Xác suất cuối = `1 - (1 - xác suất giờ) × (1 - giảm thất bại)`; giảm thất bại tối đa **48%**.
  Ví dụ 13h từ 28% lên **62,56%**, 18h từ 88% lên **93,76%**. Tính tại thời điểm kéo xong;
  thông báo cá sổng giữ đúng xác suất của lần thử đó.
- Trọng số Rare tăng thêm **60%**, nhân cùng bonus cần và chuẩn hóa với các loài khác.
- Giá bán tăng **30%**, áp dụng tại lúc bán cho cả cá đã giữ từ trước, bán từng con,
  Sell All và Auto Sell. Giá hiển thị dùng cùng công thức, làm tròn nửa đơn vị lên;
  giá gốc của cá vẫn được giữ để kiểm tra save.

## Cân bằng prototype

| Rod | Khoảng chờ | Chờ trung bình | Giá lên level kế |
| --- | --- | --- | --- |
| Lv.1 | 5–12 giây | 8,50 giây | $100 |
| Lv.2 | 4,5–10,8 giây | 7,65 giây | $225 |
| Lv.3 | 4–9,6 giây | 6,80 giây | $450 |
| Lv.4 | 3,5–8,4 giây | 5,95 giây | $800 |
| Lv.5 | 3–7,2 giây | 5,10 giây | MAX |

Thời gian trung bình trên chỉ tính Waiting, chưa cộng animation và 2 giây hiện cá.
Nâng Rod áp dụng từ lần Waiting kế tiếp. Trọng số cá Rare tăng 12% mỗi level so với trọng số gốc;
xác suất cuối cùng được chuẩn hóa cùng các loài còn lại.

**Xác suất câu thành công theo giờ địa phương:** áp dụng đúng một lần khi kết thúc Reeling.
Nếu giờ đổi lúc đang kéo, dùng giờ tại thời điểm xác định kết quả. Cá đã bắt thành công hoặc
đang treo chờ chỗ không bị roll lại khi đổi giờ hay tải save. Cá sổng không vào FishDex/records.
Bảng dưới là xác suất cơ sở khi chưa nâng Accuracy. Cần và rarity không cộng/trừ xác suất này;
Accuracy giảm phần thất bại theo công thức ở mục Stats.

| Giờ | Thành công | Mức |
| --- | --- | --- |
| 00:00 | 42% | Trung bình |
| 01:00 | 38% | Trung bình |
| 02:00 | 34% | Thấp |
| 03:00 | 32% | Thấp |
| 04:00 | 40% | Trung bình |
| 05:00 | 58% | Cao |
| 06:00 | 78% | Rất cao |
| 07:00 | 85% | Cao nhất |
| 08:00 | 72% | Rất cao |
| 09:00 | 60% | Cao |
| 10:00 | 48% | Trung bình |
| 11:00 | 38% | Thấp |
| 12:00 | 30% | Rất thấp |
| 13:00 | 28% | Thấp nhất |
| 14:00 | 32% | Thấp |
| 15:00 | 42% | Trung bình |
| 16:00 | 58% | Cao |
| 17:00 | 72% | Rất cao |
| 18:00 | 88% | Cao nhất |
| 19:00 | 82% | Rất cao |
| 20:00 | 68% | Cao |
| 21:00 | 55% | Cao |
| 22:00 | 48% | Trung bình |
| 23:00 | 44% | Trung bình |

| Fish Box | Sức chứa | Giá lên level kế |
| --- | --- | --- |
| Lv.1 | 30 | $75 |
| Lv.2 | 40 | $175 |
| Lv.3 | 50 | $350 |
| Lv.4 | 60 | $600 |
| Lv.5 | 70 | MAX |

| Loài | Rarity | Trọng lượng (kg) | Giá cơ sở/kg | Trọng số bắt ở Lv.1 |
| --- | --- | --- | --- | --- |
| Bluegill | Common | 0,15–1,20 | $8 | 32 |
| Carp | Common | 1,00–5,00 | $7 | 28 |
| Bass | Uncommon | 0,80–4,50 | $12 | 17 |
| Catfish | Uncommon | 1,50–8,00 | $10 | 13 |
| Golden Carp | Rare | 1,00–4,00 | $60 | 3 |
| Old Boot | Common (junk) | 0,30–1,50 | $2 | 7 |

Weight làm tròn 0,01 kg; giá = `max(1, round(basePrice * weight))`.
Money bắt đầu $0; giới hạn kỹ thuật $9.999.999.999. Chỉ có một currency.

## Save/load

Mặc định: **`%LOCALAPPDATA%/TaskbarFishing/save.txt`**.
Dùng text có version, không thêm thư viện JSON. Save độc lập vị trí executable.

Lưu money, hai level thiết bị, tên người chơi, 20 cấp Stats theo ID cố định, cá trong Fish Box (kèm cờ khóa), FishDex, records, cá đang treo trên
móc chưa vào thùng, Auto Sell Common và trạng thái PIN. Tự lưu khi bắt được cá, cá vào thùng/tự bán,
bán hoặc khóa cá trong thùng, mua upgrade, đổi setting và thoát bình thường.
Đóng game khi cá còn treo trên móc rồi mở lại thì con cá đó vào thùng ngay khi có chỗ.
Lần đầu mở game (chưa có `save.txt`) hiện màn chào: nhập tên, rồi chọn vài settings trước khi bắt đầu câu.
Định dạng hiện tại là **version 6** (`name <số byte> <utf8>` sau preferences, xem `assets/SETTINGS.md`).
Save version 1–5 tự chuyển khi lưu tiếp: giữ tiền, thiết bị, cá và collection, các Stats mới bắt đầu cấp 0,
các tùy chọn chưa có dùng giá trị mặc định, save cũ không có tên thì tên để trống. Cấp Stats và điều kiện
mở khóa được kiểm tra khi đọc; dữ liệu không hợp lệ dùng cơ chế phục hồi backup bên dưới.

Ghi vào `save.txt.tmp`, kiểm tra dữ liệu rồi thay file chính; `save.txt.bak` giữ snapshot hợp lệ trước đó.
Nếu file chính hỏng, game thử đọc backup. Nếu cả hai không đọc được, game báo lỗi và
tắt ghi save trong phiên đó để giữ nguyên file cũ. Đóng game, sao lưu/đổi tên hai file lỗi rồi
mở lại để bắt đầu save mới. Nếu ghi thất bại do ổ đĩa/quyền thư mục, UI hiện **SAVE FAILED**.

Chỉ chạy một phiên game cho cùng file save. Có thể dùng file khác để test:

```powershell
./build/TaskbarFishing.exe --save-path D:/taskbar_fishing/build/manual-test.save
```

Các test tự động không đọc/ghi save chơi thật.

## Build từ source

Yêu cầu CMake 3.24+, compiler C++20; bản Windows mặc định dùng OpenGL 1.1.
Với build cache cũ, configure thêm `'-DOPENGL_VERSION=1.1'` để chuyển backend.
Có thể chọn lại `'-DOPENGL_VERSION=3.3'` nếu muốn dùng renderer OpenGL 3.3.
Lần configure đầu cần mạng để tải source raylib 5.5 và nlohmann/json 3.12.0 bằng FetchContent; archive được kiểm tra SHA-256.
Sau đó build có thể dùng dependency đã cache. Không cần cài raylib riêng.

**MSYS2 UCRT64 + Ninja**, PowerShell tại thư mục project:

```powershell
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_C_COMPILER=C:/msys64/ucrt64/bin/gcc.exe -DCMAKE_CXX_COMPILER=C:/msys64/ucrt64/bin/g++.exe
cmake --build build --parallel 4
ctest --test-dir build --output-on-failure
./build/TaskbarFishing.exe
```

Điều chỉnh hai đường dẫn compiler nếu MSYS2 nằm ở nơi khác.
Bản MinGW link tĩnh runtime C++, không cần copy DLL của MSYS2; dùng các DLL hệ thống Windows.
Windows executable dùng GUI subsystem nên không mở cửa sổ console kèm theo.

**Visual Studio 2022**, cài workload Desktop development with C++:

```powershell
cmake -S . -B build-vs -G "Visual Studio 17 2022" -A x64
cmake --build build-vs --config Release
ctest --test-dir build-vs -C Release --output-on-failure
./build-vs/Release/TaskbarFishing.exe
```

Build đã được kiểm tra bằng GCC 16.2 / Ninja trên máy Windows hiện tại.
Lệnh Visual Studio được cung cấp để build bằng toolchain đó, chưa được chạy trên máy này.
CMake mới có thể báo deprecation từ cấu hình CMake của raylib 5.5; đó không phải lỗi build.

Đóng game trước khi build lại cùng executable. Đóng gói executable, sprite và tài liệu:

```powershell
cmake --install build --prefix out/TaskbarFishing --component Game
Compress-Archive -Path out/TaskbarFishing/* -DestinationPath out/TaskbarFishing-Windows-x64.zip -Force
```

Xuất bản một file (Windows):

```powershell
cmake --install build --prefix out --component Standalone
./out/TaskbarFishing.exe
```

Component `Game` vẫn cung cấp gói có tài liệu và artwork rời để chỉnh sửa;
component `Standalone` chỉ xuất `.exe`. Giấy phép raylib, GLFW và nlohmann/json
đều được nhúng dưới tên tài nguyên tương ứng trong `licenses/`.

## Kiến trúc

```text
CMakeLists.txt
src/
  main.cpp                    Parse tùy chọn, entry point
  Game.h / Game.cpp           Vòng lặp, điều phối input/gameplay/save, RAII window
  fishing/FishingSystem.*     State machine, timer, RNG seed một lần
  fish/Fish.h                 FishSpecies, FishInstance, rarity
  fish/FishDatabase.*         Dữ liệu 6 loài, weighted random, weight/price
  player/Player.*             Money, Fish Box, upgrades, collection và records
  player/Upgrades.h           28 định nghĩa nâng cấp, ID cố định và hiệu ứng Stats
  save/SaveSystem.*           Text save có version, validation và backup
  ui/UI.*                     Primitive drawing, button input, panels
  ui/StatsUI.cpp              Cây Stats, kéo/zoom và xem trước nâng cấp
  platform/DesktopWindow.*   Win32 work area và vị trí con trỏ
  platform/DiscordPresence.* Discord IPC ở luồng riêng, presence và reconnect
tests/
  CoreTests.cpp               Gameplay và persistence, không cần raylib
  StatsTests.cpp              Tiến trình, hiệu ứng, giá bán và migration v1/v2/v3
  UITests.cpp                 Button/panel integration bằng cửa sổ raylib ẩn
scripts/
  Smoke.ps1                  Chạy game ẩn có log và timeout
```

`fishing_core` không phụ thuộc raylib. UI chỉ phát action; Game điều phối thay đổi Player/FishingSystem.
State machine: `Idle → Casting → Waiting → FishBiting → Reeling → Caught | Escaped → Waiting`.
Reeling roll xác suất sổng; Escaped tự về Waiting. Caught hiện 2 giây rồi Game đưa cá vào Fish Box
(hoặc auto sell Common); nếu thùng đầy, Caught giữ nguyên tới khi có chỗ.
Timer dùng delta time, chuyển tiếp giữ phần thời gian dư.
RNG `std::mt19937` không reseed mỗi frame; test dùng seed cố định.

## Kiểm tra đã thực hiện

Cả 8 phase đều đã build và chạy cửa sổ raylib 120 frame thành công trước khi chuyển phase.

| Phase | Kết quả |
| --- | --- |
| 1 | CMake/raylib, cửa sổ 900×220 borderless, đặt trên taskbar |
| 2 | State machine và animation placeholder theo state |
| 3 | Database 6 loài, random weight và price |
| 4 | SELL/KEEP, Fish Box, bán từng cá đã giữ |
| 5 | Money, hai upgrade Lv.1–5; thêm test gameplay |
| 6 | FishDex, record, thông báo; test collection |
| 7 | Save/load, backup, pending catch; test file hỏng |
| 8 | UI, drag, PIN, Dock, minimize, auto sell, hotkeys; test UI và vòng chơi |

CTest có 9 nhóm trên Windows:

- `graphics_startup_failure`: ép GLFW khởi tạo thất bại, xác nhận trả về an toàn
  và lần khởi tạo tiếp theo vẫn render được.

- `standalone_smoke`: chỉ sao chép `.exe` vào thư mục riêng, chạy game và render đủ
  24 nền từ một thư mục làm việc khác, không có sprite/cấu hình bên cạnh executable.
  Đường dẫn test có tiếng Việt, PATH chỉ chứa Windows, và log ghi vào profile test riêng.
- `stats_progression`: 28 ô/68 lần mua/$130.000, điều kiện và trừ tiền, timer đang chạy,
  240.000 lần thử xác suất theo giờ/cần, trọng số Rare, làm tròn và khóa cá khi bán,
  save v3/migration v1–2/dữ liệu Stats hỏng/phục hồi backup.
- `background_clock` và `background_demo_smoke`: bộ đếm 15 giây, 24 ảnh nền và vòng 23h→00h như mục demo ở trên.
- `discord_presence`: ID validation, trạng thái thùng đầy/auto sell, IPC handshake, JSON escaping,
  frame bị chia nhỏ, ping/pong, giới hạn cập nhật, reconnect giữ timer, clear và thoát nhanh.
  Server giả lập dùng pipe riêng, không truy cập Discord thật.
- `core_gameplay`: state transitions, delta time, tỷ lệ thành công 24 giờ (192.000 lần kéo ở cần Lv.1/Lv.5), 10.000 random catches,
  box đầy, khóa/Sell All, tiền/nâng cấp, collection/record, auto sell, save roundtrip (v1 và v2),
  file thiếu/hỏng và backup.
- `ui_integration`: cửa sổ thật nhưng ẩn; mặc định 512×176, cảnh 32:9, đổi giờ/múi giờ và qua nửa đêm,
  mở/đóng và chuyển bảng bằng mũi tên / bánh răng,
  phím tắt mở bảng, hit test các nút ẩn, panel Caught/Escaped không có nút bán/giữ, lưới Fish Box
  (chọn, click phải bán, Alt+click khóa, Sell/Lock/Sell All, lưới 10 cột/30 ô, lăn chuột), disabled buttons, settings,
  cây Stats (khóa/thiếu tiền/mua/cấp tối đa/kéo/zoom), tiếp tục cập nhật giờ khi mở Stats.
  Ảnh render xuất ở `build/captures/`, gồm `stats-progress.png` và `stats-complete.png`.
- `game_smoke`: chạy Game thật 120 frame, mô phỏng 60 giây bằng delta time test; cá tự vào thùng,
  script click mở Fish Box → chọn ô → Sell qua UI thật, mở Stats 960×640 rồi phục hồi
  kích thước/vị trí widget; không lưu vào profile.

Chạy smoke riêng với log:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File scripts/Smoke.ps1 -Label phase8
```

Prototype giữ đúng core loop; đã có sprite môi trường, chưa có audio, sprite nhân vật, offline rewards hay các hệ thống ngoài phạm vi.
raylib và GLFW giữ bản quyền/giấy phép riêng, xem `licenses/`.

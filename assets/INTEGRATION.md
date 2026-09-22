# Phân tích và tích hợp sprite

Game đọc `assets/assets.json`; các PNG runtime được đóng gói cùng game và nhúng vào executable Windows. `source/` và `previews/` là tài liệu mỹ thuật, không dùng làm texture gameplay.

| Nhóm | Cấu trúc | Cách sử dụng trong game |
| --- | --- | --- |
| Nền | 24 sheet 4096 × 1152; mỗi sheet 8 × 8 frame 512 × 144 | Chọn giờ địa phương, phát 8 FPS, lặp 8 giây; giữ đồng hồ khi đổi giờ hoặc mở Stats |
| Nhân vật | Frame 48 × 48; anchor (22,32) | Đặt ghế tại (180,96), đọc rod_grip và góc cần riêng cho từng frame |
| Cần câu | 3 trạng thái tĩnh 64 × 16 | Relaxed khi chờ, tension lúc cắn, strong_tension khi kéo; quay cả tip cùng anchor để nối dây |
| Hiệu ứng | 9 animation có timing và anchor riêng | Vệt ném, va nước, phao, gợn sóng, cá cắn, cảnh báo, vệt kéo, nước bắn và giọt nước |
| UI | 11 bộ thành phần và font mẫu | Chọn trạng thái tĩnh, nine-slice, lọc point; chữ vẫn dùng font hệ thống để giữ đầy đủ ký tự |

## Hoạt ảnh nhân vật

| Animation | Frame | Tổng thời gian | Kết thúc |
| --- | --- | --- | --- |
| idle | 4 | 1400 ms | Lặp |
| casting | 6 | 680 ms | Về idle; đồng bộ với thời gian ném của gameplay |
| bite | 2 | 240 ms | Lặp đến lúc kéo |
| reeling | 4 | 500 ms | Lặp đến khi có kết quả |
| caught | 4 | 840 ms | Giữ frame cuối |
| escaped | 3 | 600 ms | Về idle |
| box_full | 2 | 1400 ms | Lặp khi chờ chỗ trống |

## Kết quả xem ảnh

- Nền là pixel art 32:9 có cầu ở bên trái và vùng câu bên phải. Game dùng đúng 512 × 144; cửa sổ thu gọn 512 × 176, mở rộng 512 × 428.
- Nhân vật có chân buông xuống mép cầu và tay thay đổi theo thao tác. Cần được vẽ riêng, không nằm sẵn trong sprite nhân vật.
- Phao cắn đã nằm trong `fx_bobber_bite`; renderer không vẽ chồng phao idle trong trạng thái này.
- Nền đêm giảm độ sáng; cần và hiệu ứng dùng PNG đã nhuộm màu theo giờ. Nhân vật được áp công thức lighting từ JSON, giữ nguyên alpha; không tint thêm ảnh đã nhuộm sẵn.
- Chưa có sprite cá từng loài. Các hình cá hiện tại được giữ cho inventory và cá vừa bắt.

## Kiểm tra

`py scripts/InspectAssets.py` kiểm tra 293 PNG được tham chiếu, vùng cắt và tổng timing, đồng thời xuất bảng ảnh vào `build/captures/asset-review/`.

`ctest --test-dir build --output-on-failure` kiểm tra gameplay, UI, bản độc lập và demo 24 giờ. UI test kiểm tra biên thời gian frame, one-shot ẩn đúng lúc, đổi giờ giữ clock, pixel nền/nhân vật thực sự thay đổi và đủ 7 trạng thái nhân vật. Ảnh gameplay nằm ở `build/captures/sprite-*.png`.

Các phần tích hợp chính: `src/ui/AssetCatalog.cpp`, `HourlyBackground.cpp`, `FishingSprites.cpp`, `UISkin.h` và `UI.cpp`.

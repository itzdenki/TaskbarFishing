# Special catches — 5 gói đồ

| ID | Tên | Độ hiếm tương đối |
|---|---|---|
| `driftwood_parcel` | Driftwood Parcel | Thường |
| `fishermans_gift` | Fisherman's Gift | Khá hiếm |
| `lucky_tackle_box` | Lucky Tackle Box | Hiếm |
| `golden_parcel` | Golden Parcel | Rất hiếm |
| `moonlit_chest` | Moonlit Chest | Cực hiếm |

## File dùng trong game

- `icons/<id>.png`: icon **24 × 24** trong suốt, không có viền rarity.
- `animations/<id>_float.png`: 4 frame **32 × 32**, strip **128 × 32**; 220/180/220/180 ms, tổng 800 ms. Lặp chuyển động nổi nhẹ.
- `animations/<id>_caught.png`: 8 frame **32 × 32**, strip **256 × 32**; 120/100/100/120/120/160/160/240 ms, tổng 1120 ms. Gói nhấc nhẹ, nhỏ nước, có điểm sáng tiết chế theo độ hiếm rồi giữ tư thế cuối.
- `source/<id>.aseprite`: 12 frame, hai tag `float`, `caught`; layer `package`, `ribbon_handle_lock`, `catch_fx`.
- `special.json`: đường dẫn, độ hiếm tương đối, source rectangle, duration và anchor.
- `special_icons_atlas.png`: 5 icon trên một hàng, cell 24 × 24.
- `special_catalog.*`: bảng xem thử; không dùng làm sprite sheet gameplay.

## C++ / raylib

Vùng source mỗi frame được ghi trong JSON; chỉ số bắt đầu từ 0. Ở strip 32 × 32: source.x = index × 32, source.y = 0.

Chọn frame theo tổng tích lũy duration_ms; `float` lấy elapsed modulo 800, `caught` giữ frame cuối sau 1120 ms. Timing này chỉ là nhịp mỹ thuật, không thay thế timer kéo/thời gian nhận thưởng của gameplay.

`anchor` mặc định `(16,26)` là điểm đặt gói khi trình bày/nổi. Khi treo gói trên dây, dùng **hook_anchor từng frame**, vì gói dịch nhẹ trong animation. Với scale 1 và không quay: góc trên trái = đầu dây trừ hook_anchor. Không vẽ thêm một gói tĩnh chồng lên animation đã có gói.

Nền trong suốt thật; dùng point/nearest-neighbor filtering. Có thể áp cấu hình ánh sáng trong nhóm `lighting` của assets.json chung để vật phẩm khớp giờ trong ngày. File trong bộ này là màu vật liệu gốc, chưa nhuộm 24 biến thể giờ.

`relative_rarity.rank` 1–5 chỉ là thứ tự độ hiếm special theo yêu cầu. Đây là hệ special riêng, không tự ánh xạ sang 6 rarity cá, không đặt drop rate, giá hoặc phần thưởng bên trong. Gói/rương đang đóng; chưa có animation mở hay nội dung thưởng.

Game đọc `special/special.json` và coi năm special là các loài cá bổ sung (ID 6–10), có cân nặng, giá bán, Collection, khóa bảo vệ và save. Auto Sell Common không bán special. Save cũ có sáu loài vẫn được nạp.

Tổng xác suất gặp special là **0,1% mỗi lần cá cắn**, độc lập với nâng cấp cần/luck. Sau đó vẫn áp dụng xác suất kéo lên thành công của gameplay. Phân bố trong nhóm: Driftwood Parcel 50%, Fisherman's Gift 25%, Lucky Tackle Box 15%, Golden Parcel 8%, Moonlit Chest 2%. Như vậy Moonlit Chest xuất hiện khoảng 1/50.000 lần cá cắn, trước khi xét thành công.

Icon dùng trong kho và chân dung UI v3; animation float dùng khi kéo, caught dùng khi nhấc lên. Renderer lấy hook_anchor từng frame. Không có mở rương hay phần thưởng ngẫu nhiên bên trong.

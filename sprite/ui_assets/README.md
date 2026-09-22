# Taskbar Fishing — UI kit

Bộ UI dựng bằng Aseprite, dùng màu navy/teal, chữ kem, mint cho hover và vàng cho lựa chọn. Game đã tích hợp các PNG thành phần qua `src/ui/UISkin.h`: panel, button, tab, slot, overlay, toggle, progress, icon, badge và cá chưa khám phá. Các nút dùng dữ liệu và thao tác thật trong game.

## File

- 11 bộ thành phần: panel, thanh trạng thái, button, tab, slot, slot overlay, toggle, progress, 18 icon, 5 badge, cá chưa khám phá.
- Mỗi bộ có `.aseprite` với tag một frame cho mỗi trạng thái, và PNG strip ngang. **Các frame là trạng thái tĩnh, không phát thành animation.**
- `ui_font_5x7`: font mẫu tự dựng, glyph rộng 5 cao 7 trong cell 6 × 8; advance 6 px, khoảng trắng advance 6 px. Tag `u0041` tương ứng mã Unicode U+0041. Chỉ có chữ hoa Latin cơ bản, số và dấu đã xuất; không đủ bộ dấu tiếng Việt. Với UI tiếng Việt, dùng font có hỗ trợ tiếng Việt trong renderer.
- `ui_assets.json`: tọa độ từng trạng thái và quy cách nine-slice.
- `ui_mockups.aseprite`: bốn frame tĩnh Fish Box, Collection, Catch, Settings; lớp UI và lớp nền tham chiếu riêng.
- `mockup_*.png`: ảnh minh họa có chữ và dữ liệu mẫu. Cá chỉ là hình giữ chỗ, chưa phải bộ sprite loài cá. Không dùng ảnh mockup làm toàn bộ UI tương tác.
- `mockup_compact.png`: cửa sổ thu gọn 512 × 176.
- `ui_kit_overview.png`: bảng xem nhanh các thành phần.

## Bố cục

Bản mở rộng: **512 × 428** = bảng UI 252 + hồ 144 + Last catch 32.
Bản thu gọn: **512 × 176** = hồ 144 + Last catch 32.
Đây là bố cục đề xuất khớp bộ hồ 32:9 đã dựng, không phải xác nhận kích thước hiện tại trong source game.

Fish Box: 25 ô, 9 cột; cell 26 × 26, bước 28 px, gốc (16,103), hàng cuối có 7 ô. Khung chọn cá bên phải bắt đầu x=278. Hình cá mẫu đặt trong ô, viền rarity Common xám xanh / Uncommon mint / Rare xanh dương. Golden Carp vẫn dùng viền Rare.

## Cách ghép

1. Giải nén vào thư mục assets của dự án, nạp PNG + JSON; file Aseprite dùng để sửa mỹ thuật.
2. Chọn vùng ảnh theo tên trạng thái. Không trim cell hoặc dùng bản mockup làm asset nút.
3. Panel/button/tab/status dùng nine-slice inset 4 px mỗi cạnh. Giữ nguyên góc, kéo/lặp vùng giữa để viền không bị dày lên.
4. Progress dùng cap trái/phải 3 px, top/bottom 1 px. Vẽ track, cắt fill theo phần trăm; không kéo co toàn hình để biểu diễn lượng đầy.
5. Slot giữ 26 × 26; vẽ thứ tự slot rarity → cá → hover/selected → protected. Viền chọn nằm phía trong để vẫn thấy màu rarity ở ngoài. Dấu bảo vệ vàng ở góc phải.
6. Khi disabled, chọn nền disabled, đổi chữ/icon sang muted và chặn tương tác bằng code; ảnh không tự vô hiệu hóa nút.
7. Nhãn, giá, tên cá, cân nặng, số xu, trạng thái toggle, tooltip, hitbox và focus bàn phím đều do renderer/game quản lý. Đặt chữ/icon giữa nút sau khi vẽ nền, dịch xuống 1 px lúc pressed.
8. Scale bằng nearest-neighbor/point filtering. Có thể tăng vùng bấm vô hình quanh icon 16 × 16, không cần phóng nét icon.

## Màu

Nền `#0F1A23`; panel `#172731`; raised `#1F333D`; viền `#30464F`; chữ `#E7EFE9`; chữ phụ `#91AAB0`; mint `#87D8BD`; Rare `#85B8F8`; vàng `#EFC87B`; cảnh báo `#F29A81`.

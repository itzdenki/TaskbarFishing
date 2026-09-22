# Taskbar Fishing — 30 loài cá / 6 rarity

Đủ 30 loài theo danh sách đã yêu cầu, mỗi rarity 5 loài. Đây là thiết kế pixel art cách điệu; Epic, Legendary và Mythic là tạo hình giả tưởng theo mô tả, không mô phỏng loài sinh học.

## Assets

- `icons/<id>.png`: icon inventory **24 × 24**, nền trong suốt; chừa padding để đặt trong ô 26 × 26.
- `swim/<id>.png`: strip **384 × 32**, gồm 8 frame **48 × 32**; mỗi frame 120 ms, vòng lặp 960 ms. Cá quay phải, thân và đầu giữ vị trí; đuôi, vây/râu chuyển động.
- `source/<id>.aseprite`: 3 layer `fins_tail`, `body_pattern`, `face_and_pectoral`, tag `swim`.
- `fish.json`: tên, id, rarity, đường dẫn, source rectangle, timing, anchor và điểm móc câu riêng cho từng loài.
- `fish_icons_atlas.png`: atlas **120 × 144**, 5 cột × 6 hàng, theo thứ tự rarity từ Common tới Mythic; mỗi cell 24 × 24.
- `rarity_slots.png` và `.aseprite`: 6 ô tĩnh **26 × 26**, thứ tự Common, Uncommon, Rare, Epic, Legendary, Mythic. Không chạy thành animation.
- `fish_catalog.png`, `.gif`, `.aseprite`: bảng xem cả 30 loài, không phải sprite sheet gameplay.

## C++ / raylib

Chỉ số frame bắt đầu từ 0. Đọc `source` để tạo Rectangle; đặt point filtering.

Frame bơi: `floor(elapsed_ms / 120) % 8`.
Source tự tính nếu cần: `{ frame * 48, 0, 48, 32 }`.

Đặt icon 24 × 24 tại `(slot_x + 1, slot_y + 1)` trong ô 26 × 26. Icon không có viền rarity; vẽ rarity slot trước rồi mới vẽ cá.

Khi cá đang treo trên dây: lấy `hook_anchor` làm điểm neo, không dùng tâm sprite. Ở scale 1, góc quay 0: vị trí góc trên trái = điểm đầu dây trừ `hook_anchor`. Nếu quay cá, quay quanh điểm móc này để dây không lệch khỏi miệng. `anchor` (24,16) chỉ dùng khi cần đặt theo tâm canvas.

Ghost Catfish có alpha bán trong suốt thật. Các cá khác dùng alpha 0/255. Giữ alpha khi tint theo ánh sáng giờ; không xóa vùng màu tối của Void Catfish hoặc Eclipse Carp thành trong suốt.

Rarity chỉ là nhóm mỹ thuật/metadata. Bộ này không tự đặt tỷ lệ rơi, giá, cân nặng hay timer gameplay. Màu thân Golden Carp vẫn vàng nhưng rarity là Rare; viền UI phải lấy từ rarity, không suy ra từ thân.

## Folder chung

Game đọc trực tiếp `assets/fish/fish.json`. Icon được dùng trong inventory, collection và Last catch; sprite bơi được dùng khi cá treo trên dây, neo theo `hook_anchor` và nhuộm màu theo giờ.

Các loài đang có được ghép theo tên: Bluegill, Carp → Common Carp, Bass → Largemouth Bass, Catfish và Golden Carp. Old Boot giữ hình riêng. Metadata cả 30 loài đã được nạp; 25 loài còn lại chưa được thêm vào bảng rơi, giá và cân nặng gameplay. ID save và thông số các loài hiện có được giữ nguyên.

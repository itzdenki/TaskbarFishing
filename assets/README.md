# Taskbar Fishing — assets cho C++ / raylib

Đọc **assets.json** ở thư mục gốc. JSON mô tả PNG, frame, timing, anchor, UI state, nine-slice và màu theo giờ. Hình ảnh vẫn là file PNG; JSON không thay thế dữ liệu ảnh.

```text
taskbar_fishing_assets/
  assets.json
  background/          24 sprite sheet nền động theo giờ
  player/              7 sprite sheet nhân vật trong suốt
  ui/                  khung, nút, tab, slot, icon, font mẫu
  effects/             hiệu ứng trong suốt màu trung tính
    hourly/hour_00/    hiệu ứng nhuộm màu sẵn cho 00h
    ... hour_23/
  equipment/           cần câu, 3 mức uốn tĩnh
    hourly/            cần đổi màu theo giờ
  source/              file .aseprite để chỉnh mỹ thuật
  previews/            ảnh và GIF minh họa, không phải gameplay
  README.md
  validation.txt
```

## Quy ước JSON

- Tất cả đường dẫn `texture` tương đối với **thư mục chứa assets.json**, không phụ thuộc working directory của chương trình.
- `index` bắt đầu từ **0**. `source: {x,y,width,height}` là vùng cắt trong texture, đơn vị pixel; có thể chuyển sang `Rectangle` của raylib.
- `duration_ms`, `start_ms` và tổng `duration_ms` dùng **mili giây**. Đồng hồ giây của game cần đổi sang ms trước khi chọn frame.
- `anchor` nằm trong canvas của một frame, không phải tọa độ trên cả sprite sheet.
- PNG là straight RGBA, dùng point filtering để giữ nét pixel. Không trim lại frame hoặc dùng bản GIF preview làm texture.
- JSON được chia nhóm `background`, `player`, `ui`, `effects`, `equipment`, `lighting`, `integration`.

## Dùng trong raylib

1. Dùng thư viện JSON của project để parse `assets.json` (raylib không tự đọc schema metadata này).
2. Ghép thư mục assets với đường dẫn `texture`, rồi nạp PNG bằng `LoadTexture`. Đặt `TEXTURE_FILTER_POINT` bằng `SetTextureFilter`.
3. Chọn frame/state trong JSON, dùng vùng `source` khi vẽ bằng `DrawTexturePro`. Tính vị trí vẽ từ anchor; ví dụ player vẽ tại `seat - anchor` ở scale 1.
4. Với scale lớn hơn 1, nhân cả kích thước đích lẫn khoảng cách anchor theo cùng scale. Không nhân tọa độ source rectangle.
5. Khi giải phóng tài nguyên, gọi `UnloadTexture` cho các texture đã nạp. Cache theo đường dẫn để tránh nạp lại mỗi frame. Nền chỉ cần giữ giờ hiện tại và giờ kế tiếp nếu đang chuyển cảnh.

## Nền realtime và hoạt ảnh độc lập

- Giờ địa phương 0–23 chọn `background.hours[hour]` và `lighting.hours[hour]`.
- Frame nền: `floor(background_elapsed_ms / 125) % 64`. Mỗi vòng 8 giây, có sẵn chim/sóng. **Không reset đồng hồ nền khi đổi giờ hay đổi trạng thái câu.**
- Player: chọn animation bằng `id` (`idle`, `casting`, `bite`, `reeling`, `caught`, `escaped`, `box_full`). Tra frame theo tổng thời lượng kể từ lúc vào trạng thái.
- Hiệu ứng: đồng hồ bắt đầu ở sự kiện riêng như phao chạm nước/cá cắn/cá rời mặt hồ. Dùng `loop` và `on_complete` để lặp, ẩn hoặc giữ theo gameplay.
- Khi đổi giờ chỉ thay texture/màu. Giữ nguyên tiến trình lượt câu, vị trí phao/cá và các đồng hồ.

## Hiệu ứng và ánh sáng

Mỗi effect có `texture` màu gốc và `hourly_textures` màu theo giờ. Chọn một cách: dùng màu gốc + tint theo `lighting`, hoặc dùng PNG nhuộm màu sẵn. **Không tint thêm PNG đã có `lighting_baked: true`.** Công thức tint được ghi trong JSON, giữ nguyên alpha.

`contains_bobber: true` nghĩa là frame đã chứa phao. Khi dùng `fx_bobber_bite`, ẩn phao idle để không vẽ hai phao.

`equipment.rod` là 3 trạng thái tĩnh, `animated: false`. Quay quanh `anchor`, biến đổi `tip` cùng góc để nối dây. `player.animations[].frames[].rod_grip` là điểm gắn cần, và `rod_angle_deg` là góc gợi ý theo hệ x sang phải, y xuống dưới. Dây câu do renderer vẽ theo hai đầu đang di chuyển.

## UI

Các `states` của UI là ảnh tĩnh; chọn theo tương tác, không chạy như animation. Nhãn, giá, số lượng, hitbox và hành vi nút do code xử lý.

`nine_slice` có inset trái/phải/trên/dưới. Giữ nguyên góc và độ dày viền khi co giãn; không phóng to cả ảnh để tạo một panel lớn. Font mẫu chỉ có chữ hoa Latin cơ bản, số và các dấu đã xuất; chưa hỗ trợ dấu tiếng Việt.

## Phạm vi

Gói này gom bộ nền, nhân vật đã sửa chân, UI, cần cơ bản và hiệu ứng đã làm. Chưa có bộ cá hoàn thiện, logic gameplay hoặc mask va chạm. Cá trong mockup/preview là hình giữ chỗ. **Đã tích hợp vào source C++ của game**, xem [ghi chú tích hợp](INTEGRATION.md).

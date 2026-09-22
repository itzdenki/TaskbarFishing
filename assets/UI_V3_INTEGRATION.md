# UI v3 trong game

- Chân dung `player/shoulder_portrait.png` được hiển thị ở tỷ lệ 2x, point filtering.
- Bảng chính đặt chân dung ở giữa, bốn ô trưng bày special và Fish Box phía dưới. Ô special liên kết với vật phẩm đang giữ; bấm để chọn trong kho.
- Craft mở bên trái, Stats mở bên phải, có thể mở đồng thời. Bảng Craft dùng các nâng cấp trang bị hiện có bằng xu; bộ mockup chưa cung cấp công thức vật liệu.
- Stats hiện xác suất câu, thời gian kéo, bonus cá hiếm, sức chứa và nâng cấp cần. Nút Progression tree mở cây nâng cấp đầy đủ.
- Collection và Catch truy cập bằng hai icon bên trái chân dung. Collection cuộn để xem cả 11 loài/vật phẩm. Bán, Sell All và khóa bảo vệ dùng logic hiện có.
- Giữ thanh Last catch 32 px, nên cửa sổ chính mở rộng là 512 x 536; một bảng bên là 808 x 536 và hai bảng là 1104 x 536. Cảnh hồ vẫn 512 x 144.
- Sức chứa tối đa hiện tại vẫn 25; ảnh `ui_v3_scroll_example_40.png` là mẫu mỹ thuật, chưa thêm cấp nâng kho 40 ô.

Ảnh kiểm tra: `build/captures/ui-v3-expanded.png`, `ui-v3-crafting.png`, `ui-v3-stats.png`, `ui-v3-both.png`, `ui-v3-special.png`.

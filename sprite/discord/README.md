# Discord avatar

Application ID: `1547528683689087106` (Taskbar Fishing).

Hiện tại game dùng URL CDN của App Icon mà người dùng đã tải lên Discord,
được cấu hình trong `discord_presence.json`. Không cần tải thêm Rich Presence Asset.
Icon Windows dùng `../app-icon.png` và `../app-icon.ico` (ảnh người dùng cung cấp).

Ảnh cá tạo trước đó: `taskbar_fishing.png` trong thư mục này, giữ lại làm lựa chọn thay thế.
Nếu muốn chuyển sang ảnh cá này bằng asset key:

1. Mở https://discord.com/developers/applications/1547528683689087106/information
   và tải ảnh vào **App Icon**, sau đó **Save Changes**.
2. Trong application đó, mở **Rich Presence → Art Assets**, tải cùng ảnh lên,
   đặt tên asset chính xác **taskbar_fishing**, lưu thay đổi.
3. Mở Discord desktop và khởi động lại game. Discord có thể cần vài phút để cập nhật ảnh.

Sau khi tải ảnh cá lên, đổi `large_image` trong cấu hình thành `taskbar_fishing`.
App Icon và Rich Presence Asset là hai cấu hình riêng trên Discord;
cấu hình hiện tại dùng trực tiếp URL HTTPS của App Icon thay vì asset key.
Không cần token, Client Secret hay cài bot.

Có thể thay `large_image` trong `discord_presence.json` cạnh executable bằng asset key khác
hoặc URL ảnh HTTPS công khai. `large_text` là chú thích khi rê chuột lên ảnh.
Để `large_image` trống để dùng Presence chỉ có chữ. Thay cấu hình rồi khởi động lại game.

Ảnh được tạo bằng công cụ imagegen tích hợp. Prompt:

> Create a square Discord application avatar for a cozy desktop fishing game called Taskbar Fishing.
> Polished crisp pixel art: one large golden carp facing right with mint highlights, a tiny fishing hook
> curving above it, deep navy teal water background with two simple horizontal ripples. Friendly calm mood.
> Strong bold silhouette legible at 64 pixels; centered emblem occupying 75 percent of square, generous
> safe margin for circular cropping. Limited palette of dark navy, teal, warm golden yellow and pale mint.
> Hard pixel edges, no blur, no photography, no gradients, no lettering, no text, no watermark.
> Output one square PNG icon.

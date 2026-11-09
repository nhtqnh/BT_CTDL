# Mô phỏng hàng đợi khám bệnh

**Tài liệu thiết kế và quản lý dự án C++17**  
Phiên bản: 1.0 · Phạm vi: Đồ án môn Cấu trúc dữ liệu và Giải thuật

## 1. Giới thiệu

Phần mềm mô phỏng quy trình tiếp nhận và gọi bệnh nhân tại nhiều phòng khám. Mỗi phòng khám quản lý hai hàng đợi: hàng đợi thường bằng **danh sách liên kết đơn** và hàng đợi ưu tiên bằng **heap nhị phân trên mảng**. Hệ thống mô phỏng thời gian theo phút, ghi nhận thời gian chờ, thống kê kết quả và so sánh các kịch bản vận hành.

Đây là bộ tài liệu thiết kế/khởi tạo dự án, không phải bản chương trình nghiệp vụ đã hoàn thiện. Cây thư mục trong `docs/PROJECT_DESIGN.md` là kiến trúc mục tiêu để nhóm triển khai.

## 2. Phạm vi và ràng buộc chính

- Ngôn ngữ C++, chuẩn C++17.
- Chạy Console là phương án mặc định để tập trung vào cấu trúc dữ liệu và thuật toán.
- Tự cài đặt các cấu trúc dữ liệu lõi: DSLK đơn và heap nhị phân trên mảng.
- Không dùng `vector`, `list`, `stack`, `queue`, `map`, `set`, `unordered_map`, `priority_queue`, `sort`, `find` của STL cho phần lõi.
- Dùng `string`, `iostream`, `fstream` theo yêu cầu đề bài; các thư viện phụ trợ khác chỉ dùng khi giảng viên cho phép.
- Đọc dữ liệu từ tệp khi khởi động và ghi dữ liệu/kết quả khi kết thúc.
- Có bộ dữ liệu mẫu tối thiểu 30 bản ghi và bộ dữ liệu lớn từ 10.000 bản ghi để benchmark.

## 3. Tính năng

### Theo yêu cầu đề bài

- Tiếp nhận bệnh nhân, kiểm tra mã trùng và dữ liệu đầu vào, cấp số thứ tự.
- Phân tuyến theo chuyên khoa và quản lý nhiều phòng khám.
- Gọi bệnh nhân kế tiếp: ưu tiên heap trước, sau đó đến hàng đợi thường.
- Mô phỏng bệnh nhân đến ngẫu nhiên, thời gian khám ngẫu nhiên; in trạng thái mỗi phút.
- Thống kê thời gian chờ trung bình/lớn nhất theo mức ưu tiên và độ dài hàng đợi lớn nhất.
- So sánh kịch bản có/không có ưu tiên và số phòng khám mở cửa.

### Tính năng nâng cao/đề xuất

1. **Aging chống chờ vô hạn:** tăng dần độ ưu tiên hiệu lực theo thời gian chờ; bệnh nhân ở hàng thường đủ ngưỡng được chuyển sang heap.
2. **Điều phối phòng khám thông minh:** trong các phòng cùng chuyên khoa, chọn phòng có thời gian chờ ước lượng thấp nhất.
3. **Phòng thí nghiệm kịch bản:** chạy lại cùng một luồng bệnh nhân trên nhiều cấu hình, sau đó xếp hạng kịch bản theo thời gian chờ bằng insertion sort tự cài đặt.

Ngoài ra, cấu hình ngưỡng `T` để bệnh nhân bỏ về khi chờ quá lâu là phần nâng cao được nêu trong đề bài.

## 4. Cấu trúc thư mục mục tiêu

```text
clinic-queue-simulator/
├── CMakeLists.txt                 # Cấu hình build C++17
├── README.md                      # Hướng dẫn dự án
├── main.cpp                       # Điểm bắt đầu, khởi tạo menu
├── include/
│   ├── models/                    # Patient, Room, SimulationConfig, Statistics
│   ├── structures/                # LinkedQueue, BinaryMinHeap tự cài đặt
│   ├── algorithms/                # SortUtils: insertion sort tự cài đặt
│   ├── services/                  # Hospital, SimulationEngine, StatisticsService,
│   │                              # ScenarioRunner, FileManager
│   └── ui/                        # Menu và định dạng console
├── src/
│   ├── models/                    # Cài đặt các lớp dữ liệu
│   ├── structures/                # Cài đặt DSLK và heap
│   ├── services/                  # Logic nghiệp vụ, mô phỏng, I/O, thống kê
│   ├── ui/                        # Xử lý menu
│   └── main.cpp                   # Có thể đặt main tại đây nếu thống nhất
├── data/
│   ├── patients_sample.txt        # Dữ liệu mẫu 30 bệnh nhân
│   ├── config.txt                 # Cấu hình mô phỏng
│   └── results/                   # Kết quả được xuất ra
├── tests/                         # Ca kiểm thử và dữ liệu kiểm thử
└── build/                         # Thư mục sinh ra khi build, không commit
```

Chi tiết từng tệp và trách nhiệm lớp xem `docs/PROJECT_DESIGN.md`.

## 5. Build và chạy

Yêu cầu: CMake 3.16 trở lên và compiler hỗ trợ C++17 (GCC 9+, Clang 10+ hoặc phiên bản tương thích). Nhóm tạo `CMakeLists.txt` tại thư mục gốc sau khi các tệp nguồn đã được triển khai.

```bash
cmake -S . -B build -DCMAKE_CXX_STANDARD=17
cmake --build build
```

Sau khi build, chạy executable được tạo trong `build/` (tên đích dự kiến: `clinic_queue`). Trên macOS/Linux thường là:

```bash
./build/clinic_queue
```

Trên Windows, chạy `build\Debug\clinic_queue.exe` hoặc `build\clinic_queue.exe` tùy generator.

**Lưu ý:** gói hiện tại là tài liệu thiết kế và dữ liệu mẫu; chưa có đầy đủ mã nguồn `.cpp` nên chưa thể build thành chương trình mô phỏng hoàn chỉnh.

## 6. Hướng dẫn sử dụng dự kiến

1. Đặt `patients_sample.txt` và `config.txt` trong `data/`.
2. Khởi động chương trình; hệ thống đọc và kiểm tra dữ liệu.
3. Chọn một chức năng trong menu: tiếp nhận bệnh nhân, quản lý phòng, gọi bệnh nhân, mô phỏng một ngày, thống kê hoặc so sánh kịch bản.
4. Khi mô phỏng, theo dõi trạng thái phòng khám theo từng phút.
5. Thoát chương trình để lưu dữ liệu và kết quả vào `data/results/`.

## 7. Tài liệu trong gói

- `docs/PROJECT_DESIGN.md`: đặc tả, kiến trúc, thuật toán, độ phức tạp, luồng mô phỏng và kiểm thử.
- `docs/TEAM_ASSIGNMENT.md`: phân công 4 thành viên, workflow, mốc bàn giao và quy tắc Git.
- `data/patients_sample.txt`: 30 bản ghi dữ liệu giả lập để thử nghiệm.
- `data/patients_large.txt`: 10.000 bản ghi giả lập để benchmark.
- `data/config_sample.txt`: cấu hình mô phỏng mẫu.
- `templates/CMakeLists.txt`: mẫu cấu hình CMake để nhóm điều chỉnh khi mã nguồn được tạo.
- `PROJECT_DESIGN.docx`: bản trình bày Word của tài liệu thiết kế tổng hợp.

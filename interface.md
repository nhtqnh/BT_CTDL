# BẢNG THỐNG NHẤT INTERFACE & CHUẨN ĐẶT TÊN DỰ ÁN

## 1. Data Models & Enums (`include/models/`)

### 1.1. `PriorityLevel` (Enum Class)
Phân loại mức độ ưu tiên của bệnh nhân.
- `EMERGENCY = 1`: Cấp cứu (Ưu tiên cao nhất)
- `ELDERLY = 2`: Người cao tuổi
- `CHILD = 3`: Trẻ em
- `NORMAL = 4`: Thông thường (Ưu tiên thấp nhất)

### 1.2. `PatientStatus` (Enum Class)
Trạng thái của bệnh nhân trong luồng mô phỏng.
- `WAITING`: Đang chờ đến lượt
- `IN_SERVICE`: Đang được khám
- `COMPLETED`: Đã khám xong
- `LEFT`: Đã bỏ về do chờ quá thời gian $T$
- `REJECTED`: Từ chối tiếp nhận (do lỗi dữ liệu / phòng)

### 1.3. `Patient` (Struct)
Đối tượng bệnh nhân chuẩn được lưu trữ trong các cấu trúc dữ liệu.
- **Thuộc tính:**
  - `std::string id`: Mã bệnh nhân (duy nhất)
  - `std::string fullName`: Họ và tên
  - `int age`: Tuổi
  - `PriorityLevel priority`: Mức ưu tiên gốc
  - `int effectivePriority`: Mức ưu tiên thực tế (sau khi tính Aging)
  - `std::string specialty`: Chuyên khoa khám
  - `int arrivalMinute`: Phút đến trong ngày mô phỏng
  - `int ticketNumber`: Số thứ tự cấp khi tiếp nhận
  - `int serviceDuration`: Thời gian khám dự kiến (phút)
  - `int serviceStartMinute`: Phút bắt đầu được khám (mặc định `-1`)
  - `int waitMinutes`: Tổng số phút đã chờ
  - `PatientStatus status`: Trạng thái hiện tại
- **Phương thức:**
  - `bool isHigherPriorityThan(const Patient& other) const`: So sánh khóa Heap theo thứ tự từ điển: `(effectivePriority, arrivalMinute, ticketNumber)`.

### 1.4. `SimulationConfig` (Struct)
Cấu hình tham số chạy mô phỏng.
- **Thuộc tính:**
  - `int totalSimulationMinutes`: Tổng số phút mô phỏng trong ngày (mặc định `480`)
  - `int numberOfRooms`: Số phòng khám mở cửa
  - `int minServiceDuration`: Thời gian khám tối thiểu
  - `int maxServiceDuration`: Thời gian khám tối đa
  - `unsigned int randomSeed`: Seed sinh dữ liệu ngẫu nhiên
  - `bool priorityEnabled`: Bật/tắt chế độ ưu tiên
  - `bool agingEnabled`: Bật/tắt tính năng Aging
  - `int agingInterval`: Chu kỳ tăng ưu tiên (phút)
  - `int waitThresholdT`: Ngưỡng thời gian chờ quá $T$ phút để bỏ về

### 1.5. `SimulationStatistics` & `PriorityStats` (Structs)
Gom nhóm các chỉ số đo lường hiệu năng mô phỏng.
- **`PriorityStats`:** `totalServed`, `totalWaitMinutes`, `maxWaitMinutes`, `getAverageWait()`
- **`SimulationStatistics`:** 
  - `PriorityStats statsByPriority[5]`: Mảng lưu thống kê cho 4 mức ưu tiên (chỉ số 1..4)
  - `int totalCompleted`: Tổng số người khám xong
  - `int totalLeft`: Tổng số người bỏ về
  - `int totalRejected`: Tổng số người bị từ chối
  - `int overallMaxQueueLength`: Độ dài hàng đợi lớn nhất ghi nhận được
  - `void reset()`: Đưa toàn bộ bộ đếm về 0

### 1.6. `Room` (Class)
Quản lý trạng thái và 2 cấu trúc hàng đợi của một phòng khám.
- **Thuộc tính:**
  - `std::string roomId`: Mã phòng (ví dụ: `P01`)
  - `std::string specialty`: Chuyên khoa của phòng
  - `LinkedQueue regularQueue`: Hàng đợi thường (DSLK đơn)
  - `BinaryMinHeap priorityHeap`: Hàng đợi ưu tiên (Binary Min-Heap)
  - `Patient* currentPatient`: Con trỏ đến bệnh nhân đang khám (`nullptr` nếu rảnh)
  - `int remainingServiceTime`: Thời gian khám còn lại của ca hiện tại (phút)
  - `int maxQueueLength`: Độ dài hàng đợi lớn nhất tại phòng này
  - `int totalServedCount`: Tổng số bệnh nhân đã phục vụ thành công
- **Phương thức:**
  - `Room(std::string id, std::string specialty)`
  - `std::string getRoomId() const`, `std::string getSpecialty() const`
  - `bool isBusy() const`: Trả về `true` nếu phòng đang có người khám
  - `void addPatient(const Patient& p, bool usePriority)`: Đưa bệnh nhân vào hàng thích hợp
  - `Patient* fetchNextPatient(bool usePriority)`: Lấy bệnh nhân tiếp theo (ưu tiên Heap trước DSLK)
  - `void updateServiceTick()`: Giảm 1 phút khám của ca hiện tại
  - `void startService(Patient* p, int currentMinute)`: Bắt đầu khám cho bệnh nhân mới
  - `void finishCurrentService()`: Hoàn thành ca khám và giải phóng `currentPatient`
  - `LinkedQueue& getRegularQueue()`, `BinaryMinHeap& getPriorityHeap()`

---

## 2. Cấu trúc dữ liệu lõi (`include/structures/`)

### 2.1. `LinkedQueue` (Class - DSLK Đơn)
Hàng đợi FIFO thuần túy cho bệnh nhân thường. **Không dùng container STL.**
- **Phương thức:**
  - `void enqueue(const Patient& p)`: Thêm bệnh nhân vào cuối hàng ($O(1)$)
  - `bool dequeue(Patient& outPatient)`: Lấy bệnh nhân ở đầu hàng ($O(1)$)
  - `bool peek(Patient& outPatient) const`: Xem thông tin bệnh nhân đầu hàng ($O(1)$)
  - `bool removeById(const std::string& id, Patient& removedPatient)`: Tìm và xóa bệnh nhân bỏ về ($O(N)$)
  - `bool isEmpty() const`, `int size() const`, `void clear()`
  - `QueueNode* getHead() const`: Trả về con trỏ node đầu để duyệt nội bộ

### 2.2. `BinaryMinHeap` (Class - Heap Nhị Phân trên Mảng)
Hàng đợi ưu tiên cho bệnh nhân ưu tiên. **Không dùng vector/priority_queue.**
- **Phương thức:**
  - `void insert(const Patient& p)`: Thêm bệnh nhân và tái lập min-heap ($O(\log P)$)
  - `bool extractMin(Patient& outPatient)`: Lấy phần tử có ưu tiên cao nhất ($O(\log P)$)
  - `bool peekMin(Patient& outPatient) const`: Xem gốc heap ($O(1)$)
  - `bool removeById(const std::string& id, Patient& removedPatient)`: Tìm và xóa phần tử bất kỳ ($O(P)$)
  - `void buildHeap()`: Thuật toán Bottom-up Heapify lại toàn bộ mảng ($O(P)$)
  - `bool isEmpty() const`, `int size() const`, `void clear()`
  - `Patient* getRawArray()`: Trả về con trỏ mảng động để hỗ trợ duyệt

---

## 3. Services & Logic Nghiệp Vụ (`include/services/`)

### 3.1. `Hospital` (Class)
Quản lý danh sách phòng khám và điều phối phòng.
- **Phương thức:**
  - `bool addRoom(Room* room)`: Thêm phòng khám vào danh sách quản lý
  - `Room* getRoomById(const std::string& id) const`
  - `Room* selectBestRoom(const std::string& specialty, int avgServiceTime = 10) const`: Chọn phòng tối ưu theo điểm chờ ước lượng $O(R)$
  - `int getRoomCount() const`, `Room** getRooms() const`, `void clearRooms()`

### 3.2. `SimulationEngine` (Class)
Đồng hồ và luồng điều khiển mô phỏng từng phút.
- **Phương thức:**
  - `SimulationEngine(Hospital* h, const SimulationConfig& cfg)`
  - `void runSimulation(Patient inputEvents[], int eventCount)`: Chạy vòng lặp thời gian chính
  - `void processArrivals(Patient arrivals[], int count)`: Xử lý sự kiện bệnh nhân đến
  - `void processAbandonment()`: Xử lý bệnh nhân bỏ về do chờ quá $T$
  - `void processAging()`: Nâng hạng ưu tiên và chuyển từ DSLK sang Heap
  - `void processRoomTick()`: Giảm thời gian khám & gọi bệnh nhân mới cho phòng rảnh
  - `void printMinuteStatus() const`: In trạng thái các phòng sau mỗi tick
  - `const StatisticsService& getStatisticsService() const`

### 3.3. `FileManager` (Class)
Xử lý đọc/ghi file văn bản.
- **Phương thức:**
  - `static bool loadPatients(const std::string& path, Patient outPatients[], int maxCap, int& outCount)`
  - `static bool savePatients(const std::string& path, const Patient patients[], int count)`
  - `static bool loadConfig(const std::string& path, SimulationConfig& outConfig)`
  - `static bool saveConfig(const std::string& path, const SimulationConfig& config)`
  - `static bool saveSimulationLog(const std::string& path, const std::string& logContent)`

### 3.4. `StatisticsService` (Class)
Cập nhật và tổng hợp chỉ số báo cáo.
- **Phương thức:**
  - `void recordPatientCompleted(const Patient& p)`
  - `void recordPatientLeft(const Patient& p)`
  - `void recordPatientRejected(const Patient& p)`
  - `void updateMaxQueueLength(int currentQueueLen)`
  - `const SimulationStatistics& getStatistics() const`
  - `void printReport() const`: In báo cáo tổng kết ra Console

### 3.5. `ScenarioRunner` (Class)
Chạy và so sánh các kịch bản mô phỏng khác nhau.
- **Phương thức:**
  - `static void runComparisonSuite(const Patient originalEvents[], int eventCount, const SimulationConfig& baseConfig)`

---

## 4. Thuật Toán & Giao Diện (`algorithms/` & `ui/`)

### 4.1. `SortUtils` (Class)
Thuật toán sắp xếp tự cài đặt. **Không dùng `std::sort`.**
- **Phương thức:**
  - `static void insertionSort(ScenarioResult results[], int count)`: Sắp xếp kết quả các kịch bản bằng **Insertion Sort** theo `averageWaitTime` tăng dần.

### 4.2. `Menu` (Class)
Giao diện điều khiển dạng Console.
- **Phương thức:**
  - `void run()`: Hiển thị menu tương tác và điều hướng các chức năng chính.
#include "services/FileManager.h"
#include <fstream>
#include <sstream>
#include <iomanip>
#include <random>
#include <unordered_set>
#include <algorithm>
#include <filesystem>
#include <cctype>
#include <limits>

using namespace std;
namespace fs = std::filesystem;

namespace {

    const string PATIENT_HEADER =
        "id|fullName|age|priority|specialty|arrivalMinute|ticketNumber|serviceDuration";

    bool prepareDirectory(const string& filePath, string& errorMessage) {
        try {
            fs::path path(filePath);

            if (!path.parent_path().empty()) {
                fs::create_directories(path.parent_path());
            }

            return true;
        }
        catch (const exception& e) {
            errorMessage = e.what();
            return false;
        }
    }

    vector<string> splitLine(const string& line) {
        vector<string> fields;
        char delimiter = '|';
        if (line.find('|') == string::npos && line.find(';') != string::npos) {
            delimiter = ';';
        }
        string field;

        stringstream stream(line);

        while (getline(stream, field, delimiter)) {
            fields.push_back(field);
        }

        if (!line.empty() && line.back() == delimiter) {
            fields.push_back("");
        }

        return fields;
    }

    string trim(const string& value) {
        size_t start = 0;
        size_t end = value.size();

        while (start < end &&
            isspace(static_cast<unsigned char>(value[start]))) {
            start++;
        }

        while (end > start &&
            isspace(static_cast<unsigned char>(value[end - 1]))) {
            end--;
        }

        return value.substr(start, end - start);
    }

    bool parseInteger(
        const string& value,
        int& result
    ) {
        string text = trim(value);

        if (text.empty()) {
            return false;
        }

        try {
            size_t position = 0;
            long long number = stoll(text, &position);

            if (position != text.size()) {
                return false;
            }

            if (number < numeric_limits<int>::min() ||
                number > numeric_limits<int>::max()) {
                return false;
            }

            result = static_cast<int>(number);
            return true;
        }
        catch (...) {
            return false;
        }
    }

    bool parseUnsigned(
        const string& value,
        unsigned int& result
    ) {
        string text = trim(value);

        if (text.empty()) {
            return false;
        }

        if (text[0] == '-') {
            return false;
        }

        try {
            size_t position = 0;
            unsigned long long number = stoull(text, &position);

            if (position != text.size() ||
                number > numeric_limits<unsigned int>::max()) {
                return false;
            }

            result = static_cast<unsigned int>(number);
            return true;
        }
        catch (...) {
            return false;
        }
    }

    bool parseBoolean(
        const string& value,
        bool& result
    ) {
        string text = trim(value);

        transform(
            text.begin(),
            text.end(),
            text.begin(),
            [](unsigned char c) {
                return static_cast<char>(tolower(c));
            }
        );

        if (text == "true" || text == "1") {
            result = true;
            return true;
        }

        if (text == "false" || text == "0") {
            result = false;
            return true;
        }

        return false;
    }

    bool validatePatient(
        const Patient& patient,
        string& error
    ) {
        if (patient.id.empty()) {
            error = "Patient ID khong duoc de trong.";
            return false;
        }

        if (patient.fullName.empty()) {
            error = "Ho ten benh nhan khong duoc de trong.";
            return false;
        }

        if (patient.age < 0 || patient.age > 120) {
            error = "Tuoi benh nhan phai tu 0 den 120.";
            return false;
        }

        int priority = static_cast<int>(patient.priority);

        if (priority < 1 || priority > 4) {
            error = "Priority khong hop le.";
            return false;
        }

        if (patient.specialty.empty()) {
            error = "Chuyen khoa khong duoc de trong.";
            return false;
        }

        if (patient.arrivalMinute < 0) {
            error = "ArrivalMinute khong duoc am.";
            return false;
        }

        if (patient.ticketNumber <= 0) {
            error = "TicketNumber phai lon hon 0.";
            return false;
        }

        if (patient.serviceDuration <= 0) {
            error = "ServiceDuration phai lon hon 0.";
            return false;
        }

        if (patient.effectivePriority < 1 ||
            patient.effectivePriority > 4) {
            error = "EffectivePriority khong hop le.";
            return false;
        }

        return true;
    }

    bool parsePatient(
        const vector<string>& fields,
        Patient& patient,
        string& error
    ) {
        if (fields.size() != 8 && fields.size() != 7) {
            error = "So luong cot khong dung, yeu cau 7 hoac 8 cot.";
            return false;
        }

        patient.id = trim(fields[0]);
        patient.fullName = trim(fields[1]);

        if (!parseInteger(fields[2], patient.age)) {
            error = "Age khong phai so nguyen hop le.";
            return false;
        }

        int priority = 0;
        string prioStr = trim(fields[3]);
        if (prioStr == "EMERGENCY" || prioStr == "CAPCUU" || prioStr == "1") {
            priority = 1;
        } else if (prioStr == "ELDERLY" || prioStr == "NGUOIGIA" || prioStr == "2") {
            priority = 2;
        } else if (prioStr == "CHILD" || prioStr == "TREEM" || prioStr == "3") {
            priority = 3;
        } else if (prioStr == "NORMAL" || prioStr == "THUONG" || prioStr == "4") {
            priority = 4;
        } else if (!parseInteger(prioStr, priority) || priority < 1 || priority > 4) {
            error = "Priority khong hop le (phai tu 1 den 4 hoac EMERGENCY/ELDERLY/CHILD/NORMAL).";
            return false;
        }

        patient.priority = static_cast<PriorityLevel>(priority);
        patient.effectivePriority = priority;

        string spec = trim(fields[4]);
        if (spec == "NOI") spec = "Noi khoa";
        else if (spec == "NGOAI") spec = "Ngoai khoa";
        else if (spec == "NHI") spec = "Nhi khoa";
        else if (spec == "CAPCUU") spec = "Noi khoa";
        else if (spec == "TIMACH") spec = "Noi khoa";
        else if (spec == "RHM") spec = "Rang ham mat";
        else if (spec == "MAT") spec = "Nhan khoa";
        patient.specialty = spec;

        if (!parseInteger(fields[5], patient.arrivalMinute)) {
            error = "ArrivalMinute khong hop le.";
            return false;
        }

        if (!parseInteger(fields[6], patient.ticketNumber)) {
            error = "TicketNumber khong hop le.";
            return false;
        }

        if (fields.size() >= 8) {
            if (!parseInteger(fields[7], patient.serviceDuration)) {
                error = "ServiceDuration khong hop le.";
                return false;
            }
        } else {
            patient.serviceDuration = 15;
        }

        patient.serviceStartMinute = -1;
        patient.waitMinutes = 0;
        patient.status = PatientStatus::WAITING;

        return validatePatient(patient, error);
    }

    [[maybe_unused]] string priorityToString(PriorityLevel priority) {
        switch (priority) {
        case PriorityLevel::EMERGENCY:
            return "EMERGENCY";

        case PriorityLevel::ELDERLY:
            return "ELDERLY";

        case PriorityLevel::CHILD:
            return "CHILD";

        case PriorityLevel::NORMAL:
            return "NORMAL";

        default:
            return "UNKNOWN";
        }
    }

    string statusToString(PatientStatus status) {
        switch (status) {
        case PatientStatus::WAITING:
            return "WAITING";

        case PatientStatus::IN_SERVICE:
            return "IN_SERVICE";

        case PatientStatus::COMPLETED:
            return "COMPLETED";

        case PatientStatus::LEFT:
            return "LEFT";

        case PatientStatus::REJECTED:
            return "REJECTED";

        default:
            return "UNKNOWN";
        }
    }

    bool validateConfig(
        const SimulationConfig& config,
        string& error
    ) {
        if (config.totalSimulationMinutes <= 0) {
            error = "TotalSimulationMinutes phai lon hon 0.";
            return false;
        }

        if (config.numberOfRooms <= 0) {
            error = "NumberOfRooms phai lon hon 0.";
            return false;
        }

        if (config.minServiceDuration <= 0) {
            error = "MinServiceDuration phai lon hon 0.";
            return false;
        }

        if (config.maxServiceDuration < config.minServiceDuration) {
            error = "MaxServiceDuration phai lon hon hoac bang MinServiceDuration.";
            return false;
        }

        if (config.agingEnabled) {
            if (config.agingInterval <= 0) {
                error = "AgingInterval phai lon hon 0 khi aging duoc bat.";
                return false;
            }

            if (config.waitThresholdT <= 0) {
                error = "WaitThresholdT phai lon hon 0 khi aging duoc bat.";
                return false;
            }
        }

        return true;
    }

}

bool FileManager::loadPatients(
    const string& filePath,
    vector<Patient>& patients,
    string& errorMessage
) {
    patients.clear();
    errorMessage.clear();

    ifstream file(filePath);

    if (!file.is_open()) {
        errorMessage = "Khong the mo file: " + filePath;
        return false;
    }

    unordered_set<string> usedIDs;

    string line;
    int lineNumber = 0;

    while (getline(file, line)) {
        lineNumber++;

        line = trim(line);

        if (line.empty() || line[0] == '#') {
            continue;
        }

        vector<string> fields = splitLine(line);

        if (lineNumber == 1 &&
            !fields.empty() &&
            trim(fields[0]) == "id") {
            continue;
        }

        Patient patient;
        string validationError;

        if (!parsePatient(fields, patient, validationError)) {
            errorMessage =
                "Loi tai dong " + to_string(lineNumber) +
                ": " + validationError;

            patients.clear();
            return false;
        }

        if (usedIDs.find(patient.id) != usedIDs.end()) {
            errorMessage =
                "Loi tai dong " + to_string(lineNumber) +
                ": ID bi trung lap: " + patient.id;

            patients.clear();
            return false;
        }

        usedIDs.insert(patient.id);
        patients.push_back(patient);
    }

    if (file.bad()) {
        errorMessage = "Loi khi doc file: " + filePath;
        patients.clear();
        return false;
    }

    if (patients.empty()) {
        errorMessage = "File khong co du lieu benh nhan.";
        return false;
    }

    return true;
}

bool FileManager::savePatients(
    const string& filePath,
    const vector<Patient>& patients,
    string& errorMessage
) {
    errorMessage.clear();

    if (!prepareDirectory(filePath, errorMessage)) {
        return false;
    }

    ofstream file(filePath);

    if (!file.is_open()) {
        errorMessage = "Khong the tao file: " + filePath;
        return false;
    }

    file << PATIENT_HEADER << '\n';

    unordered_set<string> usedIDs;

    for (size_t i = 0; i < patients.size(); i++) {
        const Patient& patient = patients[i];

        string validationError;

        if (!validatePatient(patient, validationError)) {
            errorMessage =
                "Du lieu khong hop le tai ban ghi " +
                to_string(i + 1) + ": " + validationError;

            return false;
        }

        if (!usedIDs.insert(patient.id).second
) {
            errorMessage =
                "ID bi trung lap tai ban ghi " +
                to_string(i + 1) + ": " + patient.id;
            return false;
        }

        file << patient.id << '|'
             << patient.fullName << '|'
             << patient.age << '|'
             << static_cast<int>(patient.priority) << '|'
             << patient.specialty << '|'
             << patient.arrivalMinute << '|'
             << patient.ticketNumber << '|'
             << patient.serviceDuration << '\n';
    }

    if (!file) {
        errorMessage = "Loi khi ghi file: " + filePath;
        return false;
    }
    return true;
}

bool FileManager::loadConfig(
    const string& filePath,
    SimulationConfig& config,
    string& errorMessage
) {
    errorMessage.clear();
    ifstream file(filePath);
    if (!file.is_open()) {
        errorMessage = "Khong the mo file: " + filePath;
        return false;
    }

    string line;
    while (getline(file, line)) {
        line = trim(line);
        if (line.empty() || line[0] == '#') continue;

        size_t sep = line.find('=');
        if (sep == string::npos) continue;

        string key   = trim(line.substr(0, sep));
        string value = trim(line.substr(sep + 1));

        int  iv = 0;
        bool bv = false;
        unsigned int uv = 0;

        if (key == "totalSimulationMinutes") {
            if (parseInteger(value, iv) && iv > 0) config.totalSimulationMinutes = iv;
        } else if (key == "numberOfRooms") {
            if (parseInteger(value, iv) && iv > 0) config.numberOfRooms = iv;
        } else if (key == "minServiceDuration") {
            if (parseInteger(value, iv) && iv > 0) config.minServiceDuration = iv;
        } else if (key == "maxServiceDuration") {
            if (parseInteger(value, iv) && iv > 0) config.maxServiceDuration = iv;
        } else if (key == "randomSeed") {
            if (parseUnsigned(value, uv)) config.randomSeed = uv;
        } else if (key == "priorityEnabled") {
            if (parseBoolean(value, bv)) config.priorityEnabled = bv;
        } else if (key == "agingEnabled") {
            if (parseBoolean(value, bv)) config.agingEnabled = bv;
        } else if (key == "agingInterval") {
            if (parseInteger(value, iv) && iv > 0) config.agingInterval = iv;
        } else if (key == "waitThresholdT") {
            if (parseInteger(value, iv) && iv > 0) config.waitThresholdT = iv;
        }
    }

    string err;
    if (!validateConfig(config, err)) {
        errorMessage = "Cau hinh khong hop le: " + err;
        return false;
    }
    return true;
}

bool FileManager::saveConfig(
    const string& filePath,
    const SimulationConfig& config,
    string& errorMessage
) {
    errorMessage.clear();
    string err;
    if (!validateConfig(config, err)) {
        errorMessage = "Cau hinh khong hop le: " + err;
        return false;
    }

    if (!prepareDirectory(filePath, errorMessage)) return false;

    ofstream file(filePath);
    if (!file.is_open()) {
        errorMessage = "Khong the tao file: " + filePath;
        return false;
    }

    file << "# Cau hinh mo phong phong kham\n"
         << "totalSimulationMinutes=" << config.totalSimulationMinutes << '\n'
         << "numberOfRooms="          << config.numberOfRooms          << '\n'
         << "minServiceDuration="     << config.minServiceDuration     << '\n'
         << "maxServiceDuration="     << config.maxServiceDuration     << '\n'
         << "randomSeed="             << config.randomSeed             << '\n'
         << "priorityEnabled="        << (config.priorityEnabled ? "true" : "false") << '\n'
         << "agingEnabled="           << (config.agingEnabled   ? "true" : "false") << '\n'
         << "agingInterval="          << config.agingInterval          << '\n'
         << "waitThresholdT="         << config.waitThresholdT         << '\n';

    if (!file) {
        errorMessage = "Loi khi ghi file: " + filePath;
        return false;
    }
    return true;
}

bool FileManager::generatePatients(
    const string& filePath,
    int numberOfPatients,
    unsigned int seed,
    string& errorMessage
) {
    errorMessage.clear();
    if (numberOfPatients <= 0) {
        errorMessage = "So benh nhan phai lon hon 0.";
        return false;
    }
    if (!prepareDirectory(filePath, errorMessage)) return false;

    ofstream file(filePath);
    if (!file.is_open()) {
        errorMessage = "Khong the tao file: " + filePath;
        return false;
    }

    static const char* SPECIALTIES[] = {
        "Noi khoa", "Ngoai khoa", "Nhi khoa", "San phu khoa",
        "Nhan khoa", "Da lieu", "Tai mui hong", "Rang ham mat"
    };
    static const int SPECIALTY_COUNT = 8;

    static const char* FIRST_NAMES[] = {
        "Nguyen", "Tran", "Le", "Pham", "Hoang",
        "Vu", "Dang", "Bui", "Do", "Ho"
    };
    static const char* MIDDLE_NAMES[] = {
        "Van", "Thi", "Duc", "Minh", "Thanh",
        "Hoang", "Ngoc", "Xuan", "Hai", "Kim"
    };
    static const char* LAST_NAMES[] = {
        "An", "Binh", "Chi", "Dung", "Giang",
        "Hoa", "Lan", "Long", "Nam", "Phuc"
    };

    mt19937 rng(seed);
    auto randInt = [&](int lo, int hi) -> int {
        return uniform_int_distribution<int>(lo, hi)(rng);
    };

    file << PATIENT_HEADER << '\n';

    for (int i = 1; i <= numberOfPatients; ++i) {
        string id = "P" + to_string(i);
        string fullName = string(FIRST_NAMES[randInt(0,9)]) + " "
                        + MIDDLE_NAMES[randInt(0,9)] + " "
                        + LAST_NAMES[randInt(0,9)];
        int age = randInt(1, 90);
        int priority = randInt(1, 4);
        string specialty = SPECIALTIES[randInt(0, SPECIALTY_COUNT - 1)];
        int arrivalMinute = randInt(0, 479);
        int ticketNumber = i;
        int serviceDuration = randInt(5, 30);

        file << id << '|'
             << fullName << '|'
             << age << '|'
             << priority << '|'
             << specialty << '|'
             << arrivalMinute << '|'
             << ticketNumber << '|'
             << serviceDuration << '\n';
    }

    if (!file) {
        errorMessage = "Loi khi ghi file: " + filePath;
        return false;
    }
    return true;
}

bool FileManager::createSamplePatients(
    const string& filePath,
    string& errorMessage
) {
    errorMessage.clear();
    if (!prepareDirectory(filePath, errorMessage)) return false;

    ofstream file(filePath);
    if (!file.is_open()) {
        errorMessage = "Khong the tao file: " + filePath;
        return false;
    }

    file << PATIENT_HEADER << '\n'
         << "P001|Nguyen Van An|45|1|Noi khoa|10|1|15\n"
         << "P002|Tran Thi Binh|72|2|Noi khoa|12|2|20\n"
         << "P003|Le Van Cuong|8|3|Nhi khoa|15|3|10\n"
         << "P004|Pham Thi Dung|30|4|Ngoai khoa|20|4|12\n"
         << "P005|Hoang Van Em|60|2|Noi khoa|25|5|18\n"
         << "P006|Vu Thi Phuong|5|3|Nhi khoa|30|6|8\n"
         << "P007|Dang Van Giang|50|1|Ngoai khoa|35|7|25\n"
         << "P008|Bui Thi Hoa|40|4|Da lieu|40|8|10\n"
         << "P009|Do Van Hai|78|2|Noi khoa|45|9|20\n"
         << "P010|Ho Thi Lan|25|4|San phu khoa|50|10|15\n"
         << "P011|Nguyen Minh Long|65|2|Noi khoa|55|11|18\n"
         << "P012|Tran Van Nam|12|3|Nhi khoa|60|12|10\n"
         << "P013|Le Thi Mai|35|4|Da lieu|65|13|12\n"
         << "P014|Pham Van Phuc|55|1|Ngoai khoa|70|14|30\n"
         << "P015|Hoang Thi Quyen|80|2|Noi khoa|75|15|22\n"
         << "P016|Vu Van Son|28|4|Tai mui hong|80|16|10\n"
         << "P017|Dang Thi Thu|7|3|Nhi khoa|85|17|8\n"
         << "P018|Bui Van Tuan|42|4|Rang ham mat|90|18|15\n"
         << "P019|Do Thi Uyen|33|4|Nhan khoa|95|19|12\n"
         << "P020|Ho Van Viet|68|2|Noi khoa|100|20|20\n"
         << "P021|Nguyen Thi Xuan|22|4|San phu khoa|105|21|15\n"
         << "P022|Tran Van Yen|50|1|Noi khoa|110|22|25\n"
         << "P023|Le Thi Anh|15|3|Nhi khoa|115|23|10\n"
         << "P024|Pham Van Bach|75|2|Noi khoa|120|24|20\n"
         << "P025|Hoang Thi Cam|38|4|Da lieu|125|25|12\n"
         << "P026|Vu Van Dien|52|4|Ngoai khoa|130|26|18\n"
         << "P027|Dang Thi Oanh|6|3|Nhi khoa|135|27|8\n"
         << "P028|Bui Van Phong|45|1|Ngoai khoa|140|28|30\n"
         << "P029|Do Thi Loan|62|2|Noi khoa|145|29|20\n"
         << "P030|Ho Van Minh|30|4|Tai mui hong|150|30|10\n";

    if (!file) {
        errorMessage = "Loi khi ghi file: " + filePath;
        return false;
    }
    return true;
}

bool FileManager::exportSimulationResults(
    const string& filePath,
    const vector<Patient>& patients,
    int totalSimulationMinutes,
    string& errorMessage
) {
    errorMessage.clear();
    if (!prepareDirectory(filePath, errorMessage)) return false;

    ofstream file(filePath);
    if (!file.is_open()) {
        errorMessage = "Khong the tao file: " + filePath;
        return false;
    }

    file << "# Ket qua mo phong - Tong thoi gian: "
         << totalSimulationMinutes << " phut\n"
         << "id|fullName|age|priority|specialty|arrivalMinute"
            "|ticketNumber|serviceDuration|serviceStartMinute"
            "|waitMinutes|status\n";

    for (size_t i = 0; i < patients.size(); ++i) {
        const Patient& p = patients[i];
        file << p.id << '|'
             << p.fullName << '|'
             << p.age << '|'
             << static_cast<int>(p.priority) << '|'
             << p.specialty << '|'
             << p.arrivalMinute << '|'
             << p.ticketNumber << '|'
             << p.serviceDuration << '|'
             << p.serviceStartMinute << '|'
             << p.waitMinutes << '|'
             << statusToString(p.status) << '\n';
    }

    if (!file) {
        errorMessage = "Loi khi ghi file: " + filePath;
        return false;
    }
    return true;
}

#include "FileManager.h"

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
        string field;

        stringstream stream(line);

        while (getline(stream, field, '|')) {
            fields.push_back(field);
        }

        if (!line.empty() && line.back() == '|') {
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
        if (fields.size() != 8) {
            error = "So luong cot khong dung, yeu cau 8 cot.";
            return false;
        }

        patient.id = trim(fields[0]);
        patient.fullName = trim(fields[1]);

        if (!parseInteger(fields[2], patient.age)) {
            error = "Age khong phai so nguyen hop le.";
            return false;
        }

        int priority = 0;

        if (!parseInteger(fields[3], priority)) {
            error = "Priority khong phai so nguyen.";
            return false;
        }

        if (priority < 1 || priority > 4) {
            error = "Priority phai nam trong khoang 1 den 4.";
            return false;
        }

        patient.priority = static_cast<PriorityLevel>(priority);
        patient.effectivePriority = priority;

        patient.specialty = trim(fields[4]);

        if (!parseInteger(fields[5], patient.arrivalMinute)) {
            error = "ArrivalMinute khong hop le.";
            return false;
        }

        if (!parseInteger(fields[6], patient.ticketNumber)) {
            error = "TicketNumber khong hop le.";
            return false;
        }

        if (!parseInteger(fields[7], patient.serviceDuration)) {
            error = "ServiceDuration khong hop le.";
            return false;
        }

        patient.serviceStartMinute = -1;
        patient.waitMinutes = 0;
        patient.status = PatientStatus::WAITING;

        return validatePatient(patient, error);
    }

    string priorityToString(PriorityLevel priority) {
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

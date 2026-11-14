#pragma once

#include <string>
#include <vector>

#include "models/Patient.h"
#include "models/SimulationConfig.h"

class FileManager {
public:
    static bool loadPatients(
        const std::string& filePath,
        std::vector<Patient>& patients,
        std::string& errorMessage
    );

    static bool savePatients(
        const std::string& filePath,
        const std::vector<Patient>& patients,
        std::string& errorMessage
    );

    static bool loadConfig(
        const std::string& filePath,
        SimulationConfig& config,
        std::string& errorMessage
    );

    static bool saveConfig(
        const std::string& filePath,
        const SimulationConfig& config,
        std::string& errorMessage
    );

    static bool generatePatients(
        const std::string& filePath,
        int numberOfPatients,
        unsigned int seed,
        std::string& errorMessage
    );

    static bool createSamplePatients(
        const std::string& filePath,
        std::string& errorMessage
    );

    static bool exportSimulationResults(
        const std::string& filePath,
        const std::vector<Patient>& patients,
        int totalSimulationMinutes,
        std::string& errorMessage
    );
};

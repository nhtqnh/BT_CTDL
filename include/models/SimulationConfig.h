#ifndef SIMULATION_CONFIG_H
#define SIMULATION_CONFIG_H

struct SimulationConfig {
    int totalSimulationMinutes = 480;
    int numberOfRooms = 0;

    int minServiceDuration = 0;
    int maxServiceDuration = 0;

    unsigned int randomSeed = 0;

    bool priorityEnabled = true;
    bool agingEnabled = false;

    int agingInterval = 0;
    int waitThresholdT = 0;
};

#endif
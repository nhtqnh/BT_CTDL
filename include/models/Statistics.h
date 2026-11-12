#ifndef STATISTICS_H
#define STATISTICS_H

struct PriorityStats {
    int totalServed = 0;
    int totalWaitMinutes = 0;
    int maxWaitMinutes = 0;

    double getAverageWait() const {
        if (totalServed == 0) {
            return 0.0;
        }

        return static_cast<double>(totalWaitMinutes) / totalServed;
    }
};

struct SimulationStatistics {
    PriorityStats statsByPriority[5];

    int totalCompleted = 0;
    int totalLeft = 0;
    int totalRejected = 0;

    int overallMaxQueueLength = 0;

    void reset() {
        for (int i = 0; i < 5; i++) {
            statsByPriority[i].totalServed = 0;
            statsByPriority[i].totalWaitMinutes = 0;
            statsByPriority[i].maxWaitMinutes = 0;
        }

        totalCompleted = 0;
        totalLeft = 0;
        totalRejected = 0;
        overallMaxQueueLength = 0;
    }
};

#endif
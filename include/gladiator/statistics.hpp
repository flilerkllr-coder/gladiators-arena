// =============================================================================
//  statistics.hpp — сбор статистики массовых прогонов + экспорт в CSV
//  Данные из этого модуля превращаются в графики и таблицы пояснительной записки.
// =============================================================================
#pragma once

#include <vector>
#include <string>
#include "gladiator/battle.hpp"

namespace glad {

// Результат одного прогона, приведённый к «плоской» записи
struct RunRecord {
    uint64_t seed;
    int   winnerId;        // 10 или 20 (id бойцов), -1 ничья
    MatchOutcome outcome;
    float duration;
    float hpA, hpB;
    int   atkA, atkB;      // брошено атак
    int   landA, landB;    // попавших атак
    int   blkA, blkB;      // успешных блоков
    float dmgA, dmgB;      // нанесённый урон
};

// Агрегированная сводка по серии прогонов
struct SeriesSummary {
    std::string label;     // описание конфигурации серии
    int runs = 0;
    int winsA = 0, winsB = 0, draws = 0;
    float avgDuration = 0.0f;
    float avgAccuracyA = 0.0f, avgAccuracyB = 0.0f;
    float avgDmgPerSecA = 0.0f, avgDmgPerSecB = 0.0f;
    int surrenders = 0;
};

class StatisticsCollector {
public:
    // Прогоняет n боёв с последовательными seed'ами baseSeed, baseSeed+1, ...
    static std::vector<RunRecord> runBatch(const MatchConfig& cfg, int n, uint64_t baseSeed);

    // Агрегация записей в сводку
    static SeriesSummary summarize(const std::vector<RunRecord>& records, const std::string& label);

    // Экспорт записей в CSV (разделитель — ';' для совместимости с Excel RU)
    static bool exportCsv(const std::string& path, const std::vector<RunRecord>& records);

    // Экспорт сводок в CSV
    static bool exportSummaryCsv(const std::string& path, const std::vector<SeriesSummary>& summaries);

    // Человекочитаемый отчёт в stdout (для демонстрации на защите)
    static void printReport(const std::vector<SeriesSummary>& summaries);
};

} // namespace glad

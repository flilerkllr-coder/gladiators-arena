// =============================================================================
//  statistics.cpp — массовые прогоны, агрегация и экспорт CSV
// =============================================================================
#include "gladiator/statistics.hpp"

#include <fstream>
#include <iostream>
#include <iomanip>
#include <numeric>

namespace glad {

std::vector<RunRecord> StatisticsCollector::runBatch(const MatchConfig& cfg, int n, uint64_t baseSeed) {
    std::vector<RunRecord> out;
    out.reserve(n);
    for (int i = 0; i < n; ++i) {
        MatchConfig c = cfg;
        c.seed = baseSeed + static_cast<uint64_t>(i);
        BattleEngine engine(c);
        MatchResult r = engine.runToEnd(300.0f);

        RunRecord rec{};
        rec.seed     = c.seed;
        rec.winnerId = r.winnerId;
        rec.outcome  = r.outcome;
        rec.duration = r.duration;
        rec.hpA      = r.statsA.hp();
        rec.hpB      = r.statsB.hp();
        rec.atkA     = r.statsA.attacksThrown;
        rec.atkB     = r.statsB.attacksThrown;
        rec.landA    = r.statsA.attacksLanded;
        rec.landB    = r.statsB.attacksLanded;
        rec.blkA     = r.statsA.blocksSuccess;
        rec.blkB     = r.statsB.blocksSuccess;
        rec.dmgA     = r.statsA.damageDealt;
        rec.dmgB     = r.statsB.damageDealt;
        out.push_back(rec);
    }
    return out;
}

SeriesSummary StatisticsCollector::summarize(const std::vector<RunRecord>& rs, const std::string& label) {
    SeriesSummary s;
    s.label = label;
    s.runs  = static_cast<int>(rs.size());
    if (rs.empty()) return s;

    float sumDur = 0.0f, sumAccA = 0.0f, sumAccB = 0.0f, sumDpsA = 0.0f, sumDpsB = 0.0f;
    for (const auto& r : rs) {
        if (r.winnerId == 10) s.winsA++;
        else if (r.winnerId == 20) s.winsB++;
        else s.draws++;
        if (r.outcome == MatchOutcome::Surrender) s.surrenders++;

        sumDur += r.duration;
        sumAccA += r.atkA > 0 ? static_cast<float>(r.landA) / r.atkA : 0.0f;
        sumAccB += r.atkB > 0 ? static_cast<float>(r.landB) / r.atkB : 0.0f;
        sumDpsA += r.duration > 0 ? r.dmgA / r.duration : 0.0f;
        sumDpsB += r.duration > 0 ? r.dmgB / r.duration : 0.0f;
    }
    s.avgDuration     = sumDur / rs.size();
    s.avgAccuracyA    = sumAccA / rs.size();
    s.avgAccuracyB    = sumAccB / rs.size();
    s.avgDmgPerSecA   = sumDpsA / rs.size();
    s.avgDmgPerSecB   = sumDpsB / rs.size();
    return s;
}

bool StatisticsCollector::exportCsv(const std::string& path, const std::vector<RunRecord>& rs) {
    std::ofstream f(path);
    if (!f) return false;
    f << "seed;winner;outcome;duration;hpA;hpB;atkA;landA;blkA;dmgA;atkB;landB;blkB;dmgB\n";
    for (const auto& r : rs) {
        f << r.seed << ';' << r.winnerId << ';'
          << (r.outcome == MatchOutcome::Death ? "death" :
              r.outcome == MatchOutcome::Surrender ? "surrender" : "draw") << ';'
          << std::fixed << std::setprecision(2)
          << r.duration << ';' << r.hpA << ';' << r.hpB << ';'
          << r.atkA << ';' << r.landA << ';' << r.blkA << ';' << r.dmgA << ';'
          << r.atkB << ';' << r.landB << ';' << r.blkB << ';' << r.dmgB << '\n';
    }
    return true;
}

bool StatisticsCollector::exportSummaryCsv(const std::string& path, const std::vector<SeriesSummary>& ss) {
    std::ofstream f(path);
    if (!f) return false;
    f << "series;runs;winsA;winsB;draws;surrenders;avgDuration;accA;accB;dpsA;dpsB\n";
    for (const auto& s : ss) {
        f << '"' << s.label << "";" << s.runs << ';' << s.winsA << ';' << s.winsB << ';'
          << s.draws << ';' << s.surrenders << ';'
          << std::fixed << std::setprecision(2)
          << s.avgDuration << ';'
          << std::setprecision(3)
          << s.avgAccuracyA << ';' << s.avgAccuracyB << ';'
          << std::setprecision(2)
          << s.avgDmgPerSecA << ';' << s.avgDmgPerSecB << '\n';
    }
    return true;
}

void StatisticsCollector::printReport(const std::vector<SeriesSummary>& summaries) {
    std::cout << "\n==================== ОТЧЁТ ПО СЕРИЯМ ПРОГОНОВ ====================\n";
    for (const auto& s : summaries) {
        std::cout << "\n[" << s.label << "]  прогонов: " << s.runs << "\n";
        std::cout << std::fixed << std::setprecision(2);
        std::cout << "  Побед A: " << s.winsA << " (" << 100.0 * s.winsA / s.runs << "%)"
                  << " | Побед B: " << s.winsB << " (" << 100.0 * s.winsB / s.runs << "%)"
                  << " | Ничьих: " << s.draws << "\n";
        std::cout << "  Средняя длительность боя: " << s.avgDuration << " с\n";
        std::cout << "  Точность ударов: A=" << s.avgAccuracyA * 100.0f << "%, B=" << s.avgAccuracyB * 100.0f << "%\n";
        std::cout << "  DPS: A=" << s.avgDmgPerSecA << ", B=" << s.avgDmgPerSecB << "\n";
        std::cout << "  Сдач: " << s.surrenders << "\n";
    }
    std::cout << "\n===================================================================\n";
}

} // namespace glad

#pragma once
#include <cstdint>
#include <random>
#include <optional>
#include "fish/FishDatabase.h"
#include "HourlyConditions.h"
#include "player/Upgrades.h"

enum class FishingState { Idle, Casting, Waiting, FishBiting, Reeling, Caught, Escaped };
enum class FishingEvent { None, Caught, Escaped };

class FishingSystem {
public:
    // How long a landed fish is shown before it goes into the Fish Box (or is auto-sold).
    static constexpr float kCaughtDisplay = 2.0f;
    // How long the "it got away" message stays before the next cast.
    static constexpr float kEscapedDisplay = 1.6f;

    explicit FishingSystem(std::uint32_t seed = std::random_device{}());
    void start(int rodLevel = 1);
    // Reports Caught or Escaped exactly once per bite. Caught waits until resolveCatch() is called;
    // Escaped starts another cast on its own.
    FishingEvent update(float dt, int rodLevel = 1);
    void resolveCatch(int rodLevel = 1);
    bool restoreCatch(const FishInstance& fish);
    // True once the caught fish has been displayed long enough to be stored.
    bool catchSettled() const { return state_ == FishingState::Caught && elapsed_ >= kCaughtDisplay; }
    bool shouldAutoSell(bool enabled) const;
    FishingState state() const { return state_; }
    float elapsed() const { return elapsed_; }
    float progress() const;
    float remaining() const;
    static const char* stateName(FishingState state);
    static float waitMultiplier(int rodLevel);
    void setLocalHour(int hour) { localHour_ = (hour % 24 + 24) % 24; }
    int localHour() const { return localHour_; }
    int lastAttemptHour() const { return lastAttemptHour_; }
    float lastAttemptChance() const { return lastAttemptChance_; }
    void setStats(StatsEffects effects) { stats_ = effects; }
    // Exact user-specified probability, independent of species and rod level.
    static float successChance(int hour) { return conditionsForHour(hour).successPercent / 100.0f; }
    const FishDatabase& database() const { return database_; }
    // The fish currently shown: the landed fish in Caught, the lost fish in Escaped.
    const std::optional<FishInstance>& catchResult() const { return catch_; }
private:
    void enter(FishingState state, float duration);
    void beginCasting();
    void beginWaiting(int rodLevel);
    FishingState state_ = FishingState::Idle;
    float elapsed_ = 0.0f;
    float duration_ = 0.0f;
    int localHour_ = 12; // Deterministic default; Game supplies the current Windows hour.
    int lastAttemptHour_ = 12;
    float lastAttemptChance_ = .30f;
    StatsEffects stats_;
    std::mt19937 random_;
    FishDatabase database_;
    std::optional<FishInstance> catch_;
};

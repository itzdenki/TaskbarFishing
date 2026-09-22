#include "fishing/FishingSystem.h"
#include <algorithm>
#include <cmath>

FishingSystem::FishingSystem(std::uint32_t seed) : random_(seed) {}

void FishingSystem::enter(FishingState state, float duration) {
    state_ = state;
    duration_ = duration;
    elapsed_ = 0.0f;
}

float FishingSystem::waitMultiplier(int rodLevel) {
    return 1.0f - 0.1f * static_cast<float>(std::clamp(rodLevel, 1, 5) - 1);
}

void FishingSystem::beginWaiting(int rodLevel) {
    enter(FishingState::Waiting, std::uniform_real_distribution<float>(5.0f, 12.0f)(random_) * waitMultiplier(rodLevel) * (1 - stats_.waitReduction));
}

void FishingSystem::beginCasting() {
    catch_.reset();
    enter(FishingState::Casting, 0.65f);
}

void FishingSystem::start(int rodLevel) {
    (void)rodLevel;
    if (state_ == FishingState::Idle) beginCasting();
}

FishingEvent FishingSystem::update(float dt, int rodLevel) {
    if (!std::isfinite(dt) || dt <= 0 || state_ == FishingState::Idle) return FishingEvent::None;
    if (state_ == FishingState::Caught) { elapsed_ += dt; return FishingEvent::None; }
    // Carry unused time across transitions, so the flow does not depend on FPS.
    while (dt > 0) {
        const float step = std::min(dt, duration_ - elapsed_);
        elapsed_ += step;
        dt -= step;
        if (elapsed_ < duration_) break;
        switch (state_) {
        case FishingState::Casting: beginWaiting(rodLevel); break;
        case FishingState::Waiting:
            catch_ = database_.roll(random_, rodLevel, stats_.rareBonus);
            enter(FishingState::FishBiting, 0.5f);
            break;
        case FishingState::FishBiting: enter(FishingState::Reeling, 1.0f - stats_.reelReduction); break;
        case FishingState::Reeling: {
            lastAttemptHour_ = localHour_;
            lastAttemptChance_ = stats_.successChance(successChance(lastAttemptHour_));
            const bool escaped = !std::bernoulli_distribution(lastAttemptChance_)(random_);
            enter(escaped ? FishingState::Escaped : FishingState::Caught, escaped ? kEscapedDisplay : kCaughtDisplay);
            elapsed_ = dt;
            return escaped ? FishingEvent::Escaped : FishingEvent::Caught;
        }
        case FishingState::Escaped:
            beginCasting();
            break;
        default: return FishingEvent::None;
        }
    }
    return FishingEvent::None;
}

void FishingSystem::resolveCatch(int rodLevel) {
    (void)rodLevel;
    if (state_ == FishingState::Caught) beginCasting();
}

bool FishingSystem::restoreCatch(const FishInstance& fish) {
    if (!database_.valid(fish)) return false;
    catch_ = fish;
    enter(FishingState::Caught, kCaughtDisplay);
    elapsed_ = kCaughtDisplay;
    return true;
}

bool FishingSystem::shouldAutoSell(bool enabled) const {
    return enabled && catchSettled() && catch_ && database_.at(catch_->speciesId).rarity == FishRarity::Common;
}

float FishingSystem::progress() const {
    return duration_ > 0 ? std::clamp(elapsed_ / duration_, 0.0f, 1.0f) : 0;
}
float FishingSystem::remaining() const { return std::max(0.0f, duration_ - elapsed_); }

const char* FishingSystem::stateName(FishingState state) {
    switch (state) {
    case FishingState::Idle: return "Idle";
    case FishingState::Casting: return "Casting";
    case FishingState::Waiting: return "Waiting for a bite";
    case FishingState::FishBiting: return "Bite!";
    case FishingState::Reeling: return "Reeling in";
    case FishingState::Caught: return "Caught!";
    case FishingState::Escaped: return "It got away!";
    }
    return "";
}

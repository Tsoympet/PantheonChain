#include "eventlog.h"

namespace parthenon {
namespace governance {

void GovernanceEventLog::Log(EventType type, uint64_t block_height,
                              const std::vector<uint8_t>& actor,
                              uint64_t reference_id,
                              const std::string& description) {
    Event ev;
    ev.event_id     = next_event_id_++;
    ev.type         = type;
    ev.block_height = block_height;
    ev.actor        = actor;
    ev.reference_id = reference_id;
    ev.description  = description;
    events_.push_back(ev);
}

std::vector<GovernanceEventLog::Event>
GovernanceEventLog::GetByType(EventType type) const {
    std::vector<Event> result;
    for (const auto& ev : events_) {
        if (ev.type == type) result.push_back(ev);
    }
    return result;
}

std::vector<GovernanceEventLog::Event>
GovernanceEventLog::GetByActor(const std::vector<uint8_t>& actor) const {
    std::vector<Event> result;
    for (const auto& ev : events_) {
        if (ev.actor == actor) result.push_back(ev);
    }
    return result;
}

std::vector<GovernanceEventLog::Event>
GovernanceEventLog::GetByBlockRange(uint64_t from_block,
                                     uint64_t to_block) const {
    std::vector<Event> result;
    for (const auto& ev : events_) {
        if (ev.block_height >= from_block && ev.block_height <= to_block)
            result.push_back(ev);
    }
    return result;
}

std::vector<GovernanceEventLog::Event>
GovernanceEventLog::GetByReferenceId(uint64_t reference_id) const {
    std::vector<Event> result;
    for (const auto& ev : events_) {
        if (ev.reference_id == reference_id) result.push_back(ev);
    }
    return result;
}

}  // namespace governance
}  // namespace parthenon

#pragma once
#include <cstdint>
namespace bvr::aa::menu_state {
enum class Mode { unknown, menu, gameplay };
struct Observation {
    Mode mode{Mode::unknown};
    const char* reason{"not sampled"};
    uint32_t count{}, capacity{};
};
Observation observe();
bool install(uintptr_t imageBase);
Mode current();
// Same bounded read chain as AWorldInfo::IsMenuLevel; also used by GPU fixtures.
Mode from_engine(uintptr_t engine);
bool fixture();
}

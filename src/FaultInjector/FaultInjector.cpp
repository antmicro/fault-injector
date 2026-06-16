// Copyright 2026 Antmicro <antmicro.com>
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
// SPDX-License-Identifier: Apache-2.0

#include <algorithm>
#include <fstream>
#include <map>

#include "verilated_vpi.h"

#include "vpi_user.h"

#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <sstream>
#include <vector>

namespace fin {

class ManagedVpiHandle {
   public:
    ManagedVpiHandle() = default;
    ManagedVpiHandle(vpiHandle handle) : m_handle(handle) {}

    ManagedVpiHandle(const ManagedVpiHandle& handle) = delete;
    ManagedVpiHandle& operator=(const ManagedVpiHandle& handle) = delete;

    ManagedVpiHandle(ManagedVpiHandle&& handle) : m_handle(handle.m_handle) {
        handle.m_handle = nullptr;
    }

    ManagedVpiHandle& operator=(ManagedVpiHandle&& handle) {
        if (this != &handle) {
            release();
            m_handle = handle.m_handle;
            handle.m_handle = nullptr;
        }
        return *this;
    }

    ~ManagedVpiHandle() { release(); }

    vpiHandle handle() const { return m_handle; }
    bool valid() const { return m_handle != nullptr; }

    operator bool() const { return valid(); }

   private:
    void release() {
        if (m_handle) {
            vpi_release_handle(m_handle);
            m_handle = nullptr;
        }
    }

    vpiHandle m_handle = nullptr;
};

enum class EventType { SingleEventTransient, SingleEventUpset };

struct Event {
    vpiHandle vpi_handle;
    std::string sig_path;
    int time;
    int bit_idx;
    EventType type;
};

std::ostream& operator<<(std::ostream& os, const Event& ev) {
    return os << "{[" << ev.time << "] " << ev.sig_path << "(" << ev.bit_idx << ")}";
}

struct TransientEvent {
    vpiHandle vpi_handle;
    s_vpi_value vpi_value{};
};

static void fin_printf(const char* formatp, ...) {
    va_list ap;
    va_start(ap, formatp);
    vpi_vprintf(const_cast<char*>(formatp), ap);
    va_end(ap);
}

class FaultInjector {
    ManagedVpiHandle vh_value_cb;
    std::map<std::string, ManagedVpiHandle> signals;
    std::vector<TransientEvent> triggered_transient_events;
    std::vector<Event> events;
    std::size_t event_idx{0};

    std::string filename;

   public:
    FaultInjector(const std::string& input_file) : filename{input_file}, vh_value_cb{registerCb()} {
        vpiHandle vhi = vpi_iterate(vpiModule, nullptr);
        gatherSignals(vhi, 0);
        readScenario();
        (void)simulateSingleEventEffects();
    }

    ~FaultInjector() = default;

    FaultInjector(const FaultInjector&) = delete;
    FaultInjector& operator=(const FaultInjector&) = delete;

   private:
    ManagedVpiHandle registerCb() {
        fin_printf("- Registering cbNextSimTime callback\n");

        s_cb_data cb_data{};
        s_vpi_time time = {vpiSimTime, 0, 1, 0};
        cb_data.time = &time;
        cb_data.reason = cbAfterDelay;
        cb_data.cb_rtn = AtNextSimTimeCallback;
        cb_data.user_data = reinterpret_cast<PLI_BYTE8*>(this);
        cb_data.value = nullptr;
        ManagedVpiHandle handle = vpi_register_cb(&cb_data);
        assert(handle.valid());
        return handle;
    }

    static PLI_INT32 AtNextSimTimeCallback(t_cb_data* data) {
        auto* self = reinterpret_cast<FaultInjector*>(data->user_data);
        auto t = getVpiTime();
        fin_printf(const_cast<char*>("- [@%d] %s iterating\n"), t.low,
                   cbReasonToString(data->reason));

        // handle event triggering
        for (auto& [vpi_handle, vpi_value] : self->triggered_transient_events) {
            vpi_put_value(vpi_handle, &vpi_value, nullptr, vpiNoDelay);
        }
        self->triggered_transient_events.clear();

        const PLI_UINT32 next_event_time = self->simulateSingleEventEffects();
        if (next_event_time == 0) {
            return 0;
        }
        const PLI_UINT32 next_time = next_event_time - t.low;

        s_cb_data cb_data{};
        s_vpi_time time = {t.type, 0, next_time, 0};
        cb_data.cb_rtn = AtNextSimTimeCallback;
        cb_data.obj = nullptr;
        cb_data.reason = data->reason;
        cb_data.time = &time;
        cb_data.user_data = reinterpret_cast<PLI_BYTE8*>(self);

        fin_printf(const_cast<char*>("- [@%d] Registering next %s Callback, next event at @%d\n"),
                   t.low, cbReasonToString(cb_data.reason), next_event_time);

        ManagedVpiHandle handle = vpi_register_cb(&cb_data);
        assert(handle.handle());
        return 0;
    }

    PLI_UINT32 simulateSingleEventEffects() {
        auto t = getVpiTime();
        while (event_idx < events.size()) {
            // this is not thread safe
            const Event& event = events.at(event_idx);
            if (event.time > t.low) {
                return event.time;
            }
            switch (event.type) {
                case EventType::SingleEventTransient:
                    simulateSingleEventTransient(t, event);
                    break;
                case EventType::SingleEventUpset:
                    simulateSingleEventUpset(t, event);
                    break;
            }
            event_idx++;
        }
        return 0;
    }

    void simulateSingleEventTransient(const s_vpi_time& time, const Event& event) {
        fin_printf(const_cast<char*>("- [@%d] Simulating single-event transient\n"), time.low);

        s_vpi_value vpi_value{};
        vpi_value.format = vpiIntVal;
        vpi_get_value(event.vpi_handle, &vpi_value);
        triggered_transient_events.emplace_back(TransientEvent{event.vpi_handle, vpi_value});

        fin_printf(const_cast<char*>("- [@%d] SET: before flipping %d bit of %s: %d\n"), time.low,
                   event.bit_idx, event.sig_path.c_str(), vpi_value.value.integer);
        vpi_value.value.integer ^= 1 << event.bit_idx;
        fin_printf(const_cast<char*>("- [@%d] SET: after flipping %d bit of %s: %d\n"), time.low,
                   event.bit_idx, event.sig_path.c_str(), vpi_value.value.integer);
        vpi_put_value(event.vpi_handle, &vpi_value, nullptr, vpiNoDelay);
    }

    void simulateSingleEventUpset(const s_vpi_time& time, const Event& event) {
        fin_printf(const_cast<char*>("- [@%d] Simulating single-event upset\n"), time.low);

        s_vpi_value vpi_value{};
        vpi_value.format = vpiIntVal;
        vpi_get_value(event.vpi_handle, &vpi_value);

        fin_printf(const_cast<char*>("- [@%d] SEU: before flipping %d bit of %s: %d\n"), time.low,
                   event.bit_idx, event.sig_path.c_str(), vpi_value.value.integer);
        vpi_value.value.integer ^= 1 << event.bit_idx;
        fin_printf(const_cast<char*>("- [@%d] SEU: after flipping %d bit of %s: %d\n"), time.low,
                   event.bit_idx, event.sig_path.c_str(), vpi_value.value.integer);
        vpi_put_value(event.vpi_handle, &vpi_value, nullptr, vpiNoDelay);
    }

    void gatherSignals(vpiHandle it, int indent) {
        while (ManagedVpiHandle hndl = vpi_scan(it)) {
            const char* nm = vpi_get_str(vpiName, hndl.handle());
            for (int i = 0; i < indent; i++) {
                fin_printf(const_cast<char*>("\t"));
            }
            fin_printf(const_cast<char*>("module '%s'\n"), nm);

            vpiHandle vhi = vpi_iterate(vpiReg, hndl.handle());
            assert(vhi);
            while (auto* vh11 = vpi_scan(vhi)) {
                const char* fn = vpi_get_str(vpiFullName, vh11);
                for (int i = 0; i < indent + 1; i++) {
                    fin_printf(const_cast<char*>("\t"));
                }
                int vpi_size = vpi_get(vpiSize, vh11);
                int vpi_type = vpi_get(vpiType, vh11);
                fin_printf("reg '%s, width: %d, type: %d'\n", fn, vpi_size, vpi_type);

                signals.insert({std::string{fn}, ManagedVpiHandle{vh11}});
            }
            vpiHandle subIt = vpi_iterate(vpiModule, hndl.handle());
            if (subIt) {
                gatherSignals(subIt, indent + 1);
            }
        }
    }

    static s_vpi_time getVpiTime() {
        s_vpi_time t;
        t.type = vpiSimTime;
        vpi_get_time(0, &t);
        return t;
    }

    void readScenario() {
        std::ifstream scenario(filename);
        std::string line;
        while (std::getline(scenario, line)) {
            std::stringstream ss(line);

            std::string time;
            std::getline(ss, time, ',');
            std::string sig_path;
            std::getline(ss, sig_path, ',');
            std::string bit_idx;
            std::getline(ss, bit_idx, ',');
            std::string type_str;
            std::getline(ss, type_str, ',');

            auto it = signals.find(sig_path);
            if (it == signals.end()) {
                // "TOP." is prefix defined in sim_main.cpp,
                // It may be a good idea to put it in some global constant somewhere
                it = signals.find("TOP." + sig_path);
                if (it == signals.end()) {
                    fin_printf("%%Error: Unrecognized signal path: %s\n", sig_path.c_str());
                    fin_printf("Ignoring the event\n");
                    continue;
                }
            }
            EventType type;
            if (type_str == "set") {
                type = EventType::SingleEventTransient;
            } else if (type_str == "seu") {
                type = EventType::SingleEventUpset;
            } else {
                fin_printf("%%Error: Unrecognized event type: %s\n", type_str.c_str());
                fin_printf("Ignoring the event\n");
                continue;
            }
            events.emplace_back(Event{
                .vpi_handle = it->second.handle(),
                .sig_path = sig_path,
                .time = std::stoi(time),
                .bit_idx = std::stoi(bit_idx),
                .type = type,
            });
        }

        std::stable_sort(events.begin(), events.end(),
                         [](const auto& ev1, const auto ev2) { return ev1.time <= ev2.time; });
    }

#define STRINGIFY_CB_CASE(_cb) \
    case _cb: \
        return #_cb

    static const char* cbReasonToString(int cb_name) {
        switch (cb_name) {
            STRINGIFY_CB_CASE(cbAtStartOfSimTime);
            STRINGIFY_CB_CASE(cbReadWriteSynch);
            STRINGIFY_CB_CASE(cbReadOnlySynch);
            STRINGIFY_CB_CASE(cbNextSimTime);
            STRINGIFY_CB_CASE(cbStartOfSimulation);
            STRINGIFY_CB_CASE(cbEndOfSimulation);
            STRINGIFY_CB_CASE(cbAtEndOfSimTime);
            STRINGIFY_CB_CASE(cbAfterDelay);
            default:
                return "Unsupported callback";
        }
    }

#undef STRINGIFY_CB_CASE
};

}  // namespace fin

extern "C" {
void* FaultInjectorCreate(const char* input_file) {
    std::printf("FaultInjector Init\n");
    auto* fi = new fin::FaultInjector(input_file);
    return fi;
}
void FaultInjectorDestroy(void* handle) {
    std::printf("FaultInjector Teardown\n");
    auto* fi = static_cast<fin::FaultInjector*>(handle);
    delete fi;
}
}

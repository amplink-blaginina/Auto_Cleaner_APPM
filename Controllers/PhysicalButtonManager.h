#pragma once

#include "gpio_types.hpp"
#include <QDebug>
#include <chrono>
#include <functional>
#include <unordered_map>

// enum class PhysicalButtonId {
//     EmergencyStop,
//     Start,
//     Stop,
//     CentralBroom,
//     SideBroom
// };

struct PhysicalButtonIdHash {
    std::size_t operator()(GPIOInput id) const noexcept{
        return static_cast<std::size_t>(id);
    }
};

class PhysicalButtonManager final {
public:
    using Callback = std::function<void()>;
    using Clock = std::chrono::steady_clock;

    struct Handlers {
        Callback onPressed;
        Callback onReleased;
    };

    explicit PhysicalButtonManager(
        std::chrono::milliseconds debounceTime = std::chrono::milliseconds{30})
        : m_debounceTime(debounceTime)
    {
    }

    void registerButton(GPIOInput id, Handlers handlers, bool initiallyPressed = false)
    {
        const auto now = Clock::now();

        m_buttons.insert_or_assign(id, ButtonState{
                                           .stablePressed = initiallyPressed,
                                           .candidatePressed = initiallyPressed,
                                           .candidateSince = now,
                                           .handlers = std::move(handlers)
                                       });
    }

    void unregisterButton(GPIOInput id)
    {
        m_buttons.erase(id);
    }

    void clear()
    {
        m_buttons.clear();
    }

    void update(GPIOInput id, bool isPressedNow)
    {
        const auto it = m_buttons.find(id);
        if (it == m_buttons.end()) {
            qWarning() << "PhysicalButtonManager: button is not registered:" << static_cast<int>(id);
            return;
        }

        ButtonState& button = it->second;
        const auto now = Clock::now();

        if (isPressedNow != button.candidatePressed) {
            button.candidatePressed = isPressedNow;
            button.candidateSince = now;
            return;
        }

        if (button.candidatePressed == button.stablePressed) {
            return;
        }

        if (now - button.candidateSince < m_debounceTime) {
            return;
        }

        button.stablePressed = button.candidatePressed;

        if (button.stablePressed) {
            if (button.handlers.onPressed) {
                button.handlers.onPressed();
            }
        } else {
            if (button.handlers.onReleased) {
                button.handlers.onReleased();
            }
        }
    }

    [[nodiscard]] bool isPressed(GPIOInput id) const
    {
        const auto it = m_buttons.find(id);
        return it != m_buttons.end() && it->second.stablePressed;
    }

    void setDebounceTime(std::chrono::milliseconds debounceTime)
    {
        m_debounceTime = debounceTime;
    }

private:
    struct ButtonState {
        bool stablePressed = false;
        bool candidatePressed = false;
        Clock::time_point candidateSince{};
        Handlers handlers;
    };

    std::unordered_map<GPIOInput, ButtonState, PhysicalButtonIdHash> m_buttons;
    std::chrono::milliseconds m_debounceTime;
};

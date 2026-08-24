#pragma once

#include <functional>
#include <utility>

class BoolStateWatcher final{
public:
    using Callback = std::function<void()>;

    struct Handlers {
        //Callback onUpdate;       // вызывается каждый polling-цикл
        Callback onActivated;    // false -> true
        Callback onDeactivated;  // true -> false
        Callback whileActive;    // вызывается каждый polling-цикл, пока true
        Callback whileInactive;  // вызывается каждый polling-цикл, пока false
    };

    explicit BoolStateWatcher(
        Handlers handlers = {},
        bool initialState = false)
        : m_handlers(std::move(handlers))
        , m_previousState(initialState)
        , m_initialized(false)
    {
    }

    void update(bool currentState) {
        // Первый вызов только синхронизирует состояние.
        // Так не будет ложного onActivated при старте программы,
        // если датчик уже активен.
        if (!m_initialized) {
            m_previousState = currentState;
            m_initialized = true;

            if (currentState) {
                if (m_handlers.whileActive) {
                    m_handlers.whileActive();
                }
            } else {
                if (m_handlers.whileInactive) {
                    m_handlers.whileInactive();
                }
            }
            return;
        }
        //m_handlers.onUpdate();

        if (currentState && !m_previousState) {
            if (m_handlers.onActivated) {
                m_handlers.onActivated();
            }
        } else if (!currentState && m_previousState) {
            if (m_handlers.onDeactivated) {
                m_handlers.onDeactivated();
            }
        }

        if (currentState){
            if (m_handlers.whileActive) {
                m_handlers.whileActive();
            }
        } else {
            if (m_handlers.whileInactive) {
                m_handlers.whileInactive();
            }
        }

        m_previousState = currentState;
    }

    [[nodiscard]] bool state() const noexcept{
        return m_previousState;
    }

    void reset(bool state = false){
        m_previousState = state;
        m_initialized = false;
    }

private:
    Handlers m_handlers;
    bool m_previousState = false;
    bool m_initialized = false;
};

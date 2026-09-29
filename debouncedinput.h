#ifndef DEBOUNCEDINPUT_H
#define DEBOUNCEDINPUT_H

// Антидребезг для матричной клавиатуры (тик mainProgress = 100 мс).
class DebouncedInput
{
public:
    explicit DebouncedInput(int thresholdTicks = 2) : m_threshold(thresholdTicks) {}

    // Вызывать каждый тик. Возвращает true один раз — при отпускании,
    // если вход был активен не меньше thresholdTicks тиков подряд.
    bool update(bool pressed)
    {
        if (pressed)
        {
            ++m_counter;
            return false;
        }
        const bool fired = (m_counter >= m_threshold);
        m_counter = 0;
        return fired;
    }

    void reset() { m_counter = 0; }

private:
    int m_counter = 0;
    const int m_threshold;
};

#endif

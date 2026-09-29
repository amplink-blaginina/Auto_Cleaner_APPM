#ifndef MEDIANFILTER_H
#define MEDIANFILTER_H

#include <QList>
#include <algorithm>

// Фильтр "медиана окна + усреднение медиан" для зашумлённых аналоговых каналов.
// Логика перенесена из MainWindow::getFilteredTemp() без изменений.
class MedianFilter
{
public:
    explicit MedianFilter(int windowSize = 10, int medianWindowSize = 10)
        : m_windowSize(windowSize), m_medianWindowSize(medianWindowSize) {}

    // Возвращает true, когда набрано полное окно сырых значений;
    // тогда out содержит отфильтрованное значение.
    bool process(qint16 raw, qint16 &out)
    {
        m_raw.append(raw);
        if (m_raw.size() <= m_windowSize)
            return false;

        m_raw.removeFirst();

        QList<qint16> sorted = m_raw;
        std::sort(sorted.begin(), sorted.end());
        const qint16 median = sorted[m_windowSize / 2];

        m_medians.append(median);
        if (m_medians.size() > m_medianWindowSize)
        {
            m_medians.removeFirst();
            qint32 sum = 0;
            for (qint16 v : m_medians)
                sum += v;
            out = sum / m_medianWindowSize;
        }
        else
            out = median;

        return true;
    }

    void reset() { m_raw.clear(); m_medians.clear(); }

private:
    QList<qint16> m_raw;
    QList<qint16> m_medians;
    const int m_windowSize;
    const int m_medianWindowSize;
};

#endif

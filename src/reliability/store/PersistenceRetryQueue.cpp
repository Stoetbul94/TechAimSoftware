#include "PersistenceRetryQueue.h"

#include <algorithm>

namespace ta {
namespace rel {

void PersistenceRetryQueue::enqueue(const QueuedEvent& item)
{
    // Aux capacity enforcement: drop the oldest queued aux to make room.
    if (!item.official) {
        while (m_auxCount >= m_auxCap) {
            // find and remove the oldest aux entry (lowest position that is aux)
            for (int i = 0; i < m_items.size(); ++i) {
                if (!m_items[i].official) {
                    m_droppedSeqs.append(m_items[i].seq);
                    ++m_totalDropped;
                    m_items.removeAt(i);
                    --m_auxCount;
                    break;
                }
            }
            // Safety: no aux left to drop (all remaining are official) — the
            // new aux still enters; the cap is a soft bound on aux only.
            if (m_auxCount >= m_auxCap && std::none_of(
                    m_items.cbegin(), m_items.cend(),
                    [](const QueuedEvent& q) { return !q.official; }))
                break;
        }
        ++m_auxCount;
    }
    m_items.append(item);
}

int PersistenceRetryQueue::officialCount() const
{
    int n = 0;
    for (const QueuedEvent& q : m_items)
        if (q.official)
            ++n;
    return n;
}

void PersistenceRetryQueue::popFront()
{
    if (m_items.isEmpty())
        return;
    if (!m_items.first().official)
        --m_auxCount;
    m_items.removeFirst();
}

QVector<DroppedRun> PersistenceRetryQueue::takeDroppedRuns()
{
    QVector<DroppedRun> runs;
    if (m_droppedSeqs.isEmpty())
        return runs;
    std::sort(m_droppedSeqs.begin(), m_droppedSeqs.end());
    DroppedRun run{m_droppedSeqs.first(), m_droppedSeqs.first(), 1};
    for (int i = 1; i < m_droppedSeqs.size(); ++i) {
        const quint64 s = m_droppedSeqs[i];
        if (s == run.lastSeq + 1) {
            run.lastSeq = s;
            ++run.count;
        } else {
            runs.append(run);
            run = DroppedRun{s, s, 1};
        }
    }
    runs.append(run);
    m_droppedSeqs.clear();
    return runs;
}

} // namespace rel
} // namespace ta

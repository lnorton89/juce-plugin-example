#include "LumaScope/Analyzer/AnalyzerConfig.h"
#include "LumaScope/Analyzer/SpectrumSnapshot.h"
#include "LumaScope/SnapshotMailbox.h"

#include <cmath>
#include <fstream>
#include <iostream>
#include <sstream>
#include <type_traits>

namespace
{
int failures = 0;

void expect (bool condition, const char* message)
{
    if (! condition)
    {
        std::cerr << "FAIL: " << message << '\n';
        ++failures;
    }
}

lumascope::SpectrumSnapshot makeSnapshot (std::uint32_t sequence, float value = 0.25f)
{
    const auto config = lumascope::makeAnalyzerConfig();
    lumascope::SpectrumSnapshot snapshot;
    snapshot.sequence = sequence;
    snapshot.profile = config.profile;
    snapshot.sampleRate = 48000.0;
    snapshot.fftSize = config.fftSize;
    snapshot.minFrequencyHz = config.minFrequencyHz;
    snapshot.maxFrequencyHz = config.maxFrequencyHz;
    snapshot.minDecibels = config.minDecibels;
    snapshot.maxDecibels = config.maxDecibels;
    snapshot.binCount = config.displayBinCount;

    for (std::size_t index = 0; index < snapshot.binCount; ++index)
    {
        snapshot.bins[index].frequencyHz = 20.0f + static_cast<float> (index);
        snapshot.bins[index].decibels = -48.0f;
        snapshot.bins[index].normalisedValue = value;
    }

    return snapshot;
}

void testLatestSnapshotWins()
{
    lumascope::SnapshotMailbox mailbox;
    lumascope::SpectrumSnapshot snapshot;
    std::uint32_t lastSeen = 0;

    expect (mailbox.publish (makeSnapshot (1)), "sequence 1 publishes");
    expect (mailbox.publish (makeSnapshot (2, 0.75f)), "sequence 2 publishes");
    expect (mailbox.readLatest (snapshot, lastSeen), "first read returns newest snapshot");
    expect (snapshot.sequence == 2, "read skips stale sequence 1");
    expect (snapshot.bins[0].normalisedValue == 0.75f, "read returns newest payload");
}

void testNoNewSnapshotForSameSequence()
{
    lumascope::SnapshotMailbox mailbox;
    lumascope::SpectrumSnapshot snapshot;
    std::uint32_t lastSeen = 0;

    expect (mailbox.publish (makeSnapshot (7)), "sequence 7 publishes");
    expect (mailbox.readLatest (snapshot, lastSeen), "first read sees sequence 7");
    expect (lastSeen == 7, "read updates last seen sequence");
    expect (! mailbox.readLatest (snapshot, lastSeen), "second read reports no new snapshot");
}

void testShapeIsRealtimeFriendly()
{
    static_assert (noexcept (std::declval<lumascope::SnapshotMailbox&>().publish (
                       std::declval<const lumascope::SpectrumSnapshot&>())));
    static_assert (noexcept (std::declval<const lumascope::SnapshotMailbox&>().readLatest (
                       std::declval<lumascope::SpectrumSnapshot&>(), std::declval<std::uint32_t&>())));

    expect (! std::is_copy_constructible_v<lumascope::SnapshotMailbox>, "mailbox is not copy constructible");
    expect (! std::is_move_constructible_v<lumascope::SnapshotMailbox>, "mailbox is not move constructible");
    expect (sizeof (lumascope::SnapshotMailbox) <= sizeof (lumascope::SpectrumSnapshot) * 3,
            "mailbox storage is fixed and bounded");
}

void testMalformedSnapshotIsRejected()
{
    lumascope::SnapshotMailbox mailbox;
    lumascope::SpectrumSnapshot malformed = makeSnapshot (3);
    malformed.binCount = lumascope::SpectrumSnapshot::maxBins + 1;

    expect (! mailbox.publish (malformed), "oversized snapshot is rejected");

    malformed = makeSnapshot (4);
    malformed.bins[0].decibels = std::numeric_limits<float>::quiet_NaN();
    expect (! mailbox.publish (malformed), "non-finite bin value is rejected");

    lumascope::SpectrumSnapshot readback;
    std::uint32_t lastSeen = 0;
    expect (! mailbox.readLatest (readback, lastSeen), "rejected snapshots are not published");
}

std::string readTextFile (const char* path)
{
    std::ifstream input (path, std::ios::binary);
    std::ostringstream contents;
    contents << input.rdbuf();
    return contents.str();
}

void testMailboxDoesNotGrowOrLock()
{
    const auto header = readTextFile (LUMASCOPE_SNAPSHOT_MAILBOX_HEADER);

    for (const auto* token : {
             "std::queue", "std::deque", "std::vector", "std::mutex", "condition_variable",
             "CriticalSection", "ScopedLock", "WaitableEvent", "new ", "make_unique", "malloc" })
    {
        expect (header.find (token) == std::string::npos, "mailbox avoids queued, locking, or heap-backed storage");
    }
}

void testLatestWinsUnderStress()
{
    lumascope::SnapshotMailbox mailbox;

    for (std::uint32_t sequence = 1; sequence <= 100; ++sequence)
        expect (mailbox.publish (makeSnapshot (sequence, static_cast<float> (sequence) / 100.0f)),
                "stress publish succeeds");

    lumascope::SpectrumSnapshot snapshot;
    std::uint32_t lastSeen = 0;
    expect (mailbox.readLatest (snapshot, lastSeen), "stress read returns latest snapshot");
    expect (snapshot.sequence == 100, "stress read drops queued stale snapshots");
    expect (! mailbox.readLatest (snapshot, lastSeen), "stress second read has no queued backlog");
}
}

int runRealtimeHandoffTests()
{
    testLatestSnapshotWins();
    testNoNewSnapshotForSameSequence();
    testShapeIsRealtimeFriendly();
    testMalformedSnapshotIsRejected();
    testMailboxDoesNotGrowOrLock();
    testLatestWinsUnderStress();
    return failures;
}

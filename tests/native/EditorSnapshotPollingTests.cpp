#include "LumaScope/PluginEditor.h"
#include "LumaScope/SnapshotMailbox.h"

#include <iostream>
#include <vector>

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

lumascope::SpectrumSnapshot makeSnapshot (std::uint32_t sequence, lumascope::AnalyzerProfile profile = lumascope::AnalyzerProfile::Musical)
{
    const auto config = lumascope::makeAnalyzerConfig (profile);
    lumascope::SpectrumSnapshot snapshot;
    snapshot.sequence = sequence;
    snapshot.profile = profile;
    snapshot.sampleRate = 48000.0;
    snapshot.fftSize = config.fftSize;
    snapshot.minFrequencyHz = config.minFrequencyHz;
    snapshot.maxFrequencyHz = config.maxFrequencyHz;
    snapshot.minDecibels = config.minDecibels;
    snapshot.maxDecibels = config.maxDecibels;
    snapshot.binCount = config.displayBinCount;

    for (std::size_t index = 0; index < snapshot.binCount; ++index)
        snapshot.bins[index] = { 20.0f + static_cast<float> (index), -48.0f, 0.5f };

    return snapshot;
}

void testMusicalCadenceIsBounded()
{
    lumascope::SnapshotMailbox mailbox;
    lumascope::EditorSnapshotPoller poller;
    std::vector<std::uint32_t> emitted;

    expect (mailbox.publish (makeSnapshot (1)), "initial snapshot publishes");

    for (int tick = 0; tick < 100; ++tick)
    {
        lumascope::SpectrumSnapshot snapshot;
        poller.poll (static_cast<double> (tick) * 10.0,
                     [&] (lumascope::SpectrumSnapshot& destination, std::uint32_t& lastSeen)
                     {
                         return mailbox.readLatest (destination, lastSeen);
                     },
                     [&] (const lumascope::SpectrumSnapshot& emittedSnapshot)
                     {
                         emitted.push_back (emittedSnapshot.sequence);
                     });

        mailbox.publish (makeSnapshot (static_cast<std::uint32_t> (tick + 2)));
    }

    expect (emitted.size() <= 46, "poller emits at most the bounded Musical cadence over one second of timer ticks");
    expect (! emitted.empty() && emitted.front() == 1, "first poll emits first available snapshot");
}

void testNewestOnlyBetweenTicks()
{
    lumascope::SnapshotMailbox mailbox;
    lumascope::EditorSnapshotPoller poller;
    std::vector<std::uint32_t> emitted;

    expect (mailbox.publish (makeSnapshot (10)), "older snapshot publishes");
    expect (mailbox.publish (makeSnapshot (11)), "newer snapshot publishes");
    expect (mailbox.publish (makeSnapshot (12)), "newest snapshot publishes");

    poller.poll (0.0,
                 [&] (lumascope::SpectrumSnapshot& destination, std::uint32_t& lastSeen)
                 {
                     return mailbox.readLatest (destination, lastSeen);
                 },
                 [&] (const lumascope::SpectrumSnapshot& emittedSnapshot)
                 {
                     emitted.push_back (emittedSnapshot.sequence);
                 });

    expect (emitted.size() == 1, "one timer tick emits one snapshot");
    expect (emitted.front() == 12, "timer tick emits newest complete snapshot only");
}

void testCloseReopenReadsProcessorOwnedLatest()
{
    lumascope::SnapshotMailbox mailbox;
    std::vector<std::uint32_t> emitted;

    expect (mailbox.publish (makeSnapshot (21)), "snapshot publishes before first editor");
    {
        lumascope::EditorSnapshotPoller firstEditorPoller;
        firstEditorPoller.poll (0.0,
                                [&] (lumascope::SpectrumSnapshot& destination, std::uint32_t& lastSeen)
                                {
                                    return mailbox.readLatest (destination, lastSeen);
                                },
                                [&] (const lumascope::SpectrumSnapshot& emittedSnapshot)
                                {
                                    emitted.push_back (emittedSnapshot.sequence);
                                });
    }

    expect (mailbox.publish (makeSnapshot (22)), "processor publishes while editor is closed");
    lumascope::EditorSnapshotPoller reopenedEditorPoller;
    reopenedEditorPoller.poll (0.0,
                               [&] (lumascope::SpectrumSnapshot& destination, std::uint32_t& lastSeen)
                               {
                                   return mailbox.readLatest (destination, lastSeen);
                               },
                               [&] (const lumascope::SpectrumSnapshot& emittedSnapshot)
                               {
                                   emitted.push_back (emittedSnapshot.sequence);
                               });

    expect (emitted.size() == 2, "reopened editor emits latest processor-owned snapshot");
    expect (emitted.back() == 22, "reopened editor does not reset analyzer or replay stale editor state");
}
}

int runEditorSnapshotPollingTests()
{
    testMusicalCadenceIsBounded();
    testNewestOnlyBetweenTicks();
    testCloseReopenReadsProcessorOwnedLatest();
    return failures;
}

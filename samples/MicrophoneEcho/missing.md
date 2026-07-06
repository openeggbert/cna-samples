# Missing / Differences from XNA 4.0 original

**Status: not yet ported.** This directory only holds this write-up (plus a verbatim
copy of `MicrophoneEcho.htm`) so the CNA-side blocker is documented where a future
porting session will look. No `src/`/`CMakeLists.txt` exist yet — see CLAUDE.md's
"Adding a new sample" steps for what's still needed once the gap below is fixed.

Source: `/rv/tmp/XNAGameStudio/Samples/MicrophoneEchoSample_4_0/MicrophoneEchoSample/MicrophoneEchoSampleGame.cs`.

## Blocker: `Microsoft.Xna.Framework.Audio.Microphone` is not implemented

**XNA behaviour:** Enumerates capture devices via `Microphone.All`/`Microphone.Default`
(`MicrophoneEchoSampleGame.cs:331-351`), opens the first connected one with a 100 ms
`BufferDuration` and subscribes to its `BufferReady` event (`InitializeMicrophone()`,
`MicrophoneEchoSampleGame.cs:235-284`). Each time the capture buffer fills,
`BufferReady()` (`MicrophoneEchoSampleGame.cs:384-397`) pulls raw PCM16 samples out
with `Microphone.GetData()`, mixes each sample with a delayed sample from a circular
`echoBuffer` to produce a decaying echo (`ProcessEcho()`, lines 405-425), and feeds the
result to a `DynamicSoundEffectInstance` for real-time loopback playback
(`SubmitBuffer()`, line 420). The sample also draws a live oscilloscope-style waveform
of the echo buffer using `BasicEffect` + `DrawUserPrimitives(PrimitiveType.LineStrip)`
(`DrawWaveform()`, lines 207-226) — that part needs no new CNA feature, only the audio
capture pipeline feeding it.

**CNA port behaviour:** N/A yet (not ported). CNA has no `Microphone` class and no
audio-capture-device support of any kind — only playback, via SDL3_mixer (DEFERRED.md
item #7, done). `DynamicSoundEffectInstance` is the playback half needed for the
loopback and would need to be checked/added separately if not already present, but
the hard blocker is entirely on the capture side.

**Root cause:** A missing engine feature: XNA's `Microphone` (device enumeration +
`BufferReady`/`GetData()`) has no CNA equivalent yet. SDL3 already supports capture
devices natively (`SDL_OpenAudioDevice` with `iscapture` / SDL3's newer recording-device
APIs) independent of SDL3_mixer, so this is plausible to add without pulling in a new
external dependency — unlike the GamerServices networking gap (DEFERRED.md item #17),
which needs a real transport layer, not just an SDL wrapper.

**What's needed in `cna`:** Add
`cna/include/Microsoft/Xna/Framework/Audio/Microphone.hpp` + an SDL3-capture-backed
implementation exposing `Microphone.All`/`Default`/`BufferDuration`/`BufferReady`/
`GetData()`/`SampleRate`/`State`, mirroring the XNA surface this sample calls into.

**Tracked in:** DEFERRED.md item #16

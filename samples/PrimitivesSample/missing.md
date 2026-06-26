# Missing / Differences from XNA 4.0 original

No known differences.

The port is fully faithful to the XNA 4.0 C# original within the natural
constraints of the C# → C++ language difference (no GC, no properties,
value/reference semantics).  `System::Random` from sharp-runtime is used
in place of `System.Random` from .NET, matching the time-seeded default
constructor behaviour.

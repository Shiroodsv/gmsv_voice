# gmsv_voice

Server side binary module for Garry's Mod that records player voice chat. It hooks the engine's voice relay, decodes the Opus stream and keeps a rolling buffer of PCM audio per player. From Lua you can pull the last N seconds of anyone's mic, get it as a WAV, or receive every raw voice packet through a hook.

Works with any gamemode. No client side files, nothing to download for players.

Things people build with it:

- Mic spam and abuse evidence. A `!clip name 30` command that writes a WAV to `data/`.
- Speech to text. Send WAV bytes to Whisper or any transcription API straight from Lua.
- Discord voice bridges and radio streams.
- Voice activated gameplay, talk time leaderboards, RP server analytics.

## Install

1. Download the build for your server from [releases](../../releases).
2. Put it in `garrysmod/lua/bin/`. Create the folder if it is missing.
3. Add `sv_use_steam_voice 0` to `server.cfg`. Steam voice packets cannot be decoded, the module still sees them but the PCM buffer stays empty.
4. In a server side script:

```lua
require("voice")
voice.Enable()
```

Builds: `linux`, `linux64`, `win32`, `win64`. Linux binaries need glibc 2.34 or newer, which covers Ubuntu 22.04 and Debian 12. Build from source for older distros.

## Quick start

```lua
require("voice")
voice.SetBufferSeconds(120)
voice.Enable()

concommand.Add("voice_clip", function(admin, _, args)
    local target = player.GetBySteamID(args[1])
    if not IsValid(target) then return end
    voice.SaveWav(target, 30, "clips/" .. target:SteamID64())
end)
```

`lua/autorun/server/` in this repo has two drop in examples: `voice_clips.lua` adds an admin `!clip <name> [seconds]` chat command, `voice_stats.lua` adds `!talk` for a talk time top five and `!mic` to list who is speaking right now.

## API

| Function | Description |
|---|---|
| `voice.Version` | Module version string |
| `voice.SampleRate` | 24000 |
| `voice.Enable([bool])` | Install or remove the engine hook. Returns `ok, err` |
| `voice.IsEnabled()` | Whether the hook is active |
| `voice.SetBufferSeconds(n)` | Buffer length per player, 1 to 600. Default 60 |
| `voice.GetBufferSeconds()` | Current buffer length |
| `voice.SetDecode(bool)` | Decode Opus to PCM. Default true. Off keeps the hook running with no audio memory |
| `voice.GetDecode()` | Current decode setting |
| `voice.SetFillSilence(bool)` | Insert silence for pauses so clips follow the wall clock. Default true |
| `voice.GetFillSilence()` | Current fill setting |
| `voice.Track(ply, bool)` | Opt a player in or out of capture. Everyone is tracked by default |
| `voice.IsTracked(ply)` | Whether a player is being captured |
| `voice.IsTalking(ply)` | True if a voice packet arrived in the last 300 ms |
| `voice.GetLevel(ply)` | Peak level of the last 100 ms, 0 to 1. 0 when not talking |
| `voice.GetBuffered(ply)` | Seconds of audio available for the player |
| `voice.GetStats(ply)` | Table with `packets`, `bytes`, `seconds` of decoded speech and `idle` seconds since the last packet |
| `voice.GetPCM(ply, seconds)` | Last N seconds as signed 16 bit little endian mono samples |
| `voice.GetWav(ply, seconds)` | Same audio as a complete WAV file in a string |
| `voice.SaveWav(ply, seconds, path)` | Writes `data/<path>.wav`. Returns `ok, err` |
| `voice.Clear(ply)` | Drops the player's buffer and stats |
| `voice.SetSignature(pattern)` | Windows only. Byte pattern for the engine function, see below |

Buffers are freed automatically when a player disconnects and when a slot is reused by a new connection.

### Hook

```lua
hook.Add("VoiceData", "example", function(ply, bytes, packet)
end)
```

Called on the main thread for every voice packet before it is relayed. `bytes` is the packet size and `packet` is the raw payload as received. Return values are ignored.

### Silence fill

By default a pause in speech is stored as silence, so asking for the last 30 seconds gives you what happened in the last 30 seconds of real time, including gaps. Turn it off with `voice.SetFillSilence(false)` if you only want speech packed back to back, which is handy for transcription.

## Memory and performance

Audio memory is allocated the first time a player talks and released on disconnect. Each talking player holds `SetBufferSeconds * 24000 * 2` bytes, about 2.8 MB at the default 60 seconds. A full 64 slot server with everyone on mic uses roughly 180 MB. Lower the buffer or use `voice.Track` if that matters.

Decoding a 20 ms Opus frame takes a few microseconds. The hook has no measurable effect on tick time.

## Platform notes

**Linux** resolves `SV_BroadcastVoiceData` from the engine's exported symbols. Nothing to configure. Both `engine.so` and `engine_srv.so` are handled.

**Windows** `engine.dll` is stripped, so the function is found by byte pattern. Pass one with `voice.SetSignature` before `voice.Enable()`. Patterns break when the engine updates. If you have a working one, open an issue so others can use it. To find it yourself, look for the function that reads `sv_alltalk` and loops over connected clients.

If the hook fails, `voice.Enable()` returns `false, reason` and nothing is captured.

## Privacy

This records people's voice. Depending on where you and your players live that may require notice or consent. Put a line in your MOTD or join message, keep the buffer short and restrict `SaveWav` and `GetWav` to staff.

## Building

Needs CMake 3.16 or newer, a C++17 compiler and git. `gmod-module-base` and `opus` are fetched during configure. `subhook` is vendored in `ext/`.

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

32 bit Linux: add `-DVOICE_X86=ON` and install `gcc-multilib`.
32 bit Windows: add `-A Win32`.

GitHub Actions builds all four targets on every push and attaches them to the release when a `v*` tag is pushed.

## License

MIT. See [LICENSE](LICENSE). subhook is BSD 2-Clause, opus is BSD 3-Clause.

# Vocal Channel Strip by ROHIT JI

A complete vocal chain in one VST3 plugin for Windows. Every module has its own tab and on/off switch.

**EQ > COMP > DE-ESS > SATURATE > GLUE > DELAY > REVERB**

| Module | What it does |
|---|---|
| **EQ** | 8 bands with draggable points on a live spectrum - Bell, Low/High Shelf, Notch, Low/High Cut (24 dB/oct), Band Pass. Mouse wheel changes Q - perfect for finding resonances. |
| **Compressor** | Three characters: **OPTO** (smooth, LA-2A style), **VCA** (clean, precise), **FET** (fast, aggressive, 1176 style). Threshold, ratio, attack, release, makeup, mix. |
| **De-Esser** | Controls harsh S / SH / CH. Split or wide mode, Listen to hear the S band. |
| **Saturation** | 3-band (Low / Mid / High) drive with adjustable crossovers, Tube / Tape / Warm, plus **Air** and **Mid Tone** exciters. |
| **Glue** | Bus-style compressor with 2:1 / 4:1 / 10:1, stepped attack/release and Auto release. |
| **Delay** | Mono, Stereo or Ping-Pong. In Stereo mode the LEFT and RIGHT echoes have their own time and feedback. Tempo sync (1/32 to 1/1, dotted, triplet), filtered feedback, width, ducking. |
| **Reverb** | 9 spaces (Ambience, Room, Studio, Chamber, Plate, Hall, Cathedral, Arena, Vintage) with low/high cut. |

10 presets: Pop Vocal, Bhojpuri Lead, Devotional Warm, Rap Vocal, Airy Female, Deep Male, Podcast Voice, Backing Vocals, Lo-Fi Vocal.

## Download

Go to **[Releases](../../releases/latest)** and pick:

| File | For |
|---|---|
| **Vocal-Channel-Strip-Setup-64bit.exe** | 64-bit FL Studio, Cubase and other modern DAWs (most people) |
| **Vocal-Channel-Strip-Setup-32bit.exe** | Older 32-bit DAWs only |

Run it and click **Next > Install > Finish**.

## How it is built

Every push to `main` builds the plugin with MSVC (x64 and Win32) on GitHub Actions, packages each
into its own setup wizard (`Installer/setup.iss`, Inno Setup) and publishes them as the latest release.

# Demo sample sources

These one-shots ship with the offline **Demo Rock Kit** catalog entry.

All samples are **CC0** from the [stargate-sample-pack](https://github.com/stargatedaw/stargate-sample-pack) (fugue-state-audio / distkit series).

| KitForge file | Source file |
|---|---|
| kick_center.wav | `fugue-state-audio/drums/kicks/distkit-kick.wav` |
| snare_center.wav | `fugue-state-audio/drums/snares/distkit-snare.wav` |
| snare_rimshot.wav | `fugue-state-audio/drums/percussion/x0xproc2-rimshot.wav` |
| rack_tom_hit.wav | `fugue-state-audio/drums/toms/distkit-midtom.wav` |
| floor_tom_hit.wav | `fugue-state-audio/drums/toms/distkit-lotom.wav` |
| hihat_closed.wav | `fugue-state-audio/drums/hihats/distkit-hatclsd.wav` |
| hihat_open.wav | `fugue-state-audio/drums/hihats/distkit-hatopen.wav` |
| crash_hit.wav | `fugue-state-audio/drums/cymbals/distkit-crash.wav` |
| ride_bow.wav | `fugue-state-audio/drums/cymbals/distkit-ride.wav` |

Refresh bundled WAVs:

```bash
./scripts/fetch_demo_samples.sh
cmake --build build --target KitForge_Standalone
```

No attribution required (CC0).

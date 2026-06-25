# KitForge Implementation Roadmap

## Phase 1 — Foundation (current)
- [x] Modular folder architecture (Engine, Models, Importers, Catalog, AI, UI)
- [x] Visual kit builder with articulations, velocity layers, round robins, chokes
- [x] Kit JSON save/load
- [x] Tabbed UI shell (Builder, Library, AI, Settings)
- [x] SFZ importer skeleton (key, lovel, hivel, group, seq_*, sample)
- [x] Catalog client skeleton with mock manifest
- [x] AI builder skeleton with mock recipe generation
- [x] SampleIndex + SampleMetadata data model
- [x] .kitforgepack folder format + exporter staging

## Phase 2 — Library & Catalog (in progress)
- [x] Real HTTP manifest download (background thread + cache)
- [x] DownloadManager: async download, progress UI, file:// support
- [x] Zip / .kitforgepack extraction (juce::ZipFile)
- [x] KitInstaller: download → extract → import → register library
- [x] Offline demo pack (`demo-rock-kit`) for testing without a server
- [x] Catalog UI: Install / Remove buttons, progress bar, load kit after install
- [ ] Download resume / checksum validation
- [ ] Full SFZ opcode support (group/off_by, lorand/hirand, trigger=...)
- [ ] SampleIndex: deep scan + tag extraction from installed kits
- [ ] Installed kit versioning and updates

## Phase 3 — AI Kit Builder
- [ ] OpenAI / local LLM integration in AIKitBuilderService
- [ ] Prompt templates + guardrails
- [ ] Sample matching scoring (tags + spectral metadata)
- [ ] Preview before apply, undo kit replace
- [ ] User-editable recipes

## Phase 4 — Pack Ecosystem
- [ ] Pack signing and verification
- [ ] Share/export .kitforgepack from current kit
- [ ] Artwork thumbnails in canvas + catalog
- [ ] Relative sample paths inside packs

## Phase 5 — Pro Sampler Features
- [ ] Multi-output routing (outputChannelPair)
- [ ] Mute/solo bus logic
- [ ] Hi-hat pedal chick, foot splash, choke curves
- [ ] Round-robin avoid-repeat rules
- [ ] Sample start/end trim UI

## Phase 6 — Platform (future)
- [ ] Cloud auth (explicitly out of scope for Phase 1)
- [ ] Marketplace / payments (out of scope)
- [ ] MP3/FLAC import
- [ ] Real drum kit artwork/sprites

## .kitforgepack layout
```
MyKit.kitforgepack (zip)
├── kit.json
├── samples/
├── artwork/
├── license.txt
└── credits.txt
```

## Library paths
- `~/Documents/KitForge/Libraries/` — installed libraries
- `~/Documents/KitForge/Catalog/manifest.json` — cached catalog
- `~/Documents/KitForge/InstalledKits/` — unpacked playable kits

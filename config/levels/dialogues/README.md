# Dialogue scripts

Each `meeting-N.json` holds the script for one Genie encounter across the
game. They are pure data — the runtime dialogue system (to be implemented)
reads them and drives the conversation UI.

## Files

| File | Level | Place | Trigger |
|------|------:|------:|---------|
| `meeting-1.json` | 2 | Place 2 | Approach `p2-lamp-active` |
| `meeting-2.json` | 2 | Place 3 | Approach `p3-lamp-active` (apple-fight tutorial) |
| `meeting-3.json` | 3 | TBD | Placeholder — fill in when Level 3 is designed |
| `meeting-4.json` | 3 | TBD | Placeholder — final pre-boss meeting |
| `style.json`     | — | — | Shared font / box / per-speaker colors |

## Schema

```jsonc
{
    "meeting":  <int>,              // 1..4
    "title":    "...",
    "level":    <int>,
    "place":    <int>,              // optional

    "trigger": {
        "lampEntity":   "p2-lamp-active", // entity name in the level config
        "radius":       4.5,              // world units — player must be within
        "oneShot":      true,             // true = only play once per save
        "revealEntity": "p2-genie"        // entity to enable when dialogue starts
    },

    "onFinish": {
        "removeEntities":  ["p2-lamp-active", "p2-genie"],
        "revealEntities":  ["p2-lamp-used"],
        "enableEntities":  ["portal-place2-to-3"],  // optional
        "unlockAbility":   "apple_projectile"        // optional
    },

    "lines": [
        { "speaker": "genie",   "text": "..." },
        { "speaker": "aladdin", "text": "..." }
    ]
}
```

### Speakers and styling

`style.json` defines per-speaker appearance. The dialogue box uses the
speaker's `background`, `borderColor`, and `textColor` so the two
characters read as distinct on screen:

- **Aladdin** — warm brown box with cream text (hero palette).
- **Genie**   — deep-blue box with glowing cyan text (magical palette).

All colors are RGBA in the range `[0, 1]`.

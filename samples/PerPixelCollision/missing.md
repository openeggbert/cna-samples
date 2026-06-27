# Missing / Differences from XNA 4.0 original

## Color key applied manually to assets
**XNA behaviour:** The XNA Content Pipeline automatically converts magenta (#FF00FF)
pixels to transparent (A=0) when processing BMP textures via color keying.
**CNA port behaviour:** The BMP files were converted to PNG with magenta replaced by
transparent pixels using ImageMagick before inclusion in the project.
**Root cause:** CNA does not use the XNA Content Pipeline; assets must be in an open
format with transparency pre-applied.
**Tracked in:** not planned.

## Color named constants replaced with RGBA literals
**XNA behaviour:** Uses `Color.Red`, `Color.CornflowerBlue`, `Color.White`.
**CNA port behaviour:** Equivalent RGBA values used directly.
**Root cause:** CNA Color does not expose named static constants yet.
**Tracked in:** CNA issue (minor convenience gap).

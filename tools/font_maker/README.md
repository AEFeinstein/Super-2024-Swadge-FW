# `font_maker`

`font_maker` renders TrueType Font (`.ttf`) files into `.font.png` files that can be used as Swadge fonts.

The resulting `.font.png` will likely require manual touch up, but it's still a pretty good starting point.

## Usage
```
font_maker [font file] [font size]
```

## Libraries Used

* https://github.com/tomolt/libschrift to render TrueType fonts
* https://github.com/nothings/stb is used to write PNG files
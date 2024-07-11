* Use `xsd.exe` from https://developer.microsoft.com/en-us/windows/downloads/windows-sdk/
* Rename `svg_plain.svg` to `svg_plain.xml`
    ```powershell
    cp svg_plain.svg svg_plain.xml
    ```
* Generate `plain_svg.xsd` with:
    ```powershell
    xsd.exe svg_plain.xml
    ```
* Use `generateDS` from https://www.davekuhlman.org/generateDS.html
* Generate `plain_svg.py` with:
    ```bash
    generateDS -f -o svg_plain.py svg_plain.xsd
    ```

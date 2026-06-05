#!/usr/bin/env bash
# Download file-type icons for AmnesiaIDE
# Uses devicon SVG icons from jsDelivr CDN
set -e

DIR="assets/icons"
mkdir -p "$DIR"

declare -A ICONS
ICONS["rs"]="rust/rust-original"
ICONS["c"]="c/c-original"
ICONS["h"]="c/c-original"
ICONS["cpp"]="cplusplus/cplusplus-original"
ICONS["cc"]="cplusplus/cplusplus-original"
ICONS["cxx"]="cplusplus/cplusplus-original"
ICONS["hpp"]="cplusplus/cplusplus-original"
ICONS["hxx"]="cplusplus/cplusplus-original"
ICONS["cs"]="csharp/csharp-original"
ICONS["py"]="python/python-original"
ICONS["js"]="javascript/javascript-original"
ICONS["ts"]="typescript/typescript-original"
ICONS["html"]="html5/html5-original"
ICONS["css"]="css3/css3-original"
ICONS["scss"]="sass/sass-original"
ICONS["md"]="markdown/markdown-original"
ICONS["json"]="json/json-original"
ICONS["toml"]="toml/toml-original"
ICONS["yaml"]="yaml/yaml-original"
ICONS["yml"]="yaml/yaml-original"
ICONS["svg"]="svg/svg-original"

for ext in "${!ICONS[@]}"; do
    file="$DIR/$ext.png"
    if [ -f "$file" ]; then
        echo "SKIP $ext (exists)"
        continue
    fi
    url="https://cdn.jsdelivr.net/gh/devicons/devicon/icons/${ICONS[$ext]}.svg"
    echo "DL $ext <- $url"
    # Save as raw SVG data; loader handles it
    curl -sL "$url" -o "$DIR/$ext.svg" || echo "FAIL $ext"
done

# Create a default icon (generic file)
if [ ! -f "$DIR/_.svg" ]; then
    echo '<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24"><path d="M4 4h16v2H4zm0 4h16v2H4zm0 4h16v2H4zm0 4h12v2H4z" fill="#888"/></svg>' > "$DIR/_.svg"
fi

echo "DONE. Icons in $DIR/"

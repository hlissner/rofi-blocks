{ pkgs ? import <nixpkgs> {} }:

with pkgs;

mkShell {
  nativeBuildInputs = [
    autoreconfHook
    pkg-config
    gobject-introspection
    wrapGAppsHook
  ];
  buildInputs = [
    (rofi-unwrapped.overrideAttrs (final: prev: {
      version = "1.7.5-dev";
      src = fetchFromGitHub {
        owner = "davatorium";
        repo = "rofi";
        rev = "7814da7ee42ee7763cbced5ac1dc0f6f39369ab2";
        sha256 = "0cwbrqr6b1abi5ib7jj97in3qz8gxlc3frw5yln61g2pvgd81qfc";
        fetchSubmodules = true;
      };
    }))
    pango
    glib
    cairo
    json-glib
  ];
  shellHook = ''
    export ROFI_PLUGIN_PATH="$XDG_CONFIG_HOME/rofi/plugins"
  '';
}

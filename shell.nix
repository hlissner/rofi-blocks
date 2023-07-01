# TODO: Replace with flake.nix and use rofi package in hlissner/dotfiles
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
    (rofi-unwrapped.overridDerivation (prev: {
      version = "1.7.5-dev-fork";
      src = fetchFromGitHub {
        owner = "hlissner";
        repo = "rofi";
        rev = "32ffee7ddd962a23624cccd2f4e059aa1a8a9e53";
        sha256 = "0l71r79dj1n1bycnlllxqzkkq33lsg1jxy86c8n86b3i8hj4k2px";
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

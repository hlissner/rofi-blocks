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
    (rofi-unwrapped.overrideAttrs (final: prev: {
      version = "1.7.5-dev";
      src = fetchFromGitHub {
        owner = "davatorium";
        repo = "rofi";
        rev = "7814da7ee42ee7763cbced5ac1dc0f6f39369ab2";
        sha256 = "0cwbrqr6b1abi5ib7jj97in3qz8gxlc3frw5yln61g2pvgd81qfc";
        fetchSubmodules = true;
      };
      patches = [
        # Call mode_preprocess_input even when input is empty, and operate on
        # its returned pattern, if available.
        ./patches/mode-preprocess-input.patch
        # Add new mode function: _selection_changed, to be called whenever the
        # active row changes.
        ./patches/mode-selection-changed.patch
        # Adds a function to change case sensitivity on-the-fly.
        ./patches/rofi-view-set-case-sensitive.patch
        # Adds a function to preload icons, and change the "icon" widget's icon
        # on-the-fly.
        ./patches/rofi-view-set-icon.patch
        # A less intrusive input setter function (including an attempt to
        # restore the selection after refiltering)
        ./patches/rofi-view-set-input.patch
        # Adds a function to change placeholder on-the-fly
        ./patches/rofi-view-set-placeholder.patch
        # Adds a function to trigger keybinds by name.
        ./patches/rofi-view-trigger-action-by-name.patch
        # Support -password option
        ./patches/universal-password-flag.patch
      ];
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

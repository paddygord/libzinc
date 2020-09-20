with import <nixpkgs> {};

let
  cairo-gl = cairo.override { glSupport = true; };
in stdenv.mkDerivation {
  name = "test";
  src = ./.;

  buildInputs = [
    clang pkg-config meson ninja
    glfw xorg.libX11 xorg.libXrandr cairo-gl
  ];
}

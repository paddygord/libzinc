with import <nixpkgs> {};

let
  cairo-gl = cairo.override { glSupport = true; };
  opengl-drivers = mesa_drivers;
in stdenv.mkDerivation {
  name = "test";
  src = ./.;

  buildInputs = [
    clang pkg-config meson ninja
    glfw xorg.libX11 xorg.libXrandr cairo-gl
    makeWrapper
  ];

  postFixup = ''
    for f in $out/bin/*; do
      wrapProgram $f \
        --prefix LD_LIBRARY_PATH : "${opengl-drivers}/lib" \
        --set LIBGL_DRIVERS_PATH "${opengl-drivers}/lib/dri"
    done
  '';
}

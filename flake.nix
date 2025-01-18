{
  description = "hello";

  inputs = {
    nixpkgs.url = "github:nixos/nixpkgs/nixos-unstable";
  };

  outputs = {nixpkgs, ...}: let
    system = "x86_64-linux";
    pkgs = import nixpkgs {inherit system;};
  in {
    devShells."${system}".default = pkgs.mkShell {
      packages = with pkgs; [
        raylib
        emscripten
      ];
      C_INCLUDE_PATH = "${pkgs.emscripten}/share/emscripten/cache/sysroot/include/";
      LD_LIBRARY_PATH = "${pkgs.emscripten}/share/emscripten/cache/sysroot/lib/";
    };

    packages.${system}.default = pkgs.stdenv.mkDerivation {
      pname = "Web Mario";
      version = "1.0.0";
      src = ./.;

      buildInputs = with pkgs; [
        raylib
        emscripten
      ];

      buildPhase = ''
        export HOME=$(mktemp -d)
        make web
      '';

      installPhase = ''
        mkdir -p $out
        cp -r game/* $out
      '';
    };
  };
}

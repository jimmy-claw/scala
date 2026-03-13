{
  description = "Secure Calendar App — privacy-first shared calendar module for Logos Core";

  inputs = {
    logos-module-builder.url = "github:logos-co/logos-module-builder";
    nixpkgs.follows = "logos-module-builder/nixpkgs";
    logos-cpp-sdk = {
      url = "github:logos-co/logos-cpp-sdk";
      inputs.nixpkgs.follows = "nixpkgs";
    };
    logos-liblogos = {
      url = "github:logos-co/logos-liblogos";
      inputs.nixpkgs.follows = "nixpkgs";
      inputs.logos-cpp-sdk.follows = "logos-cpp-sdk";
    };
  };

  outputs = { self, logos-module-builder, nixpkgs, logos-cpp-sdk, logos-liblogos, ... }:
    let
      moduleOutputs = logos-module-builder.lib.mkLogosModule {
        src = ./.;
        configFile = ./module.yaml;
      };
      systems = [ "x86_64-linux" "aarch64-linux" "x86_64-darwin" "aarch64-darwin" ];
      forAllSystems = f: nixpkgs.lib.genAttrs systems (system: f {
        pkgs = import nixpkgs { inherit system; };
        logosSdk = logos-cpp-sdk.packages.${system}.default;
        logosLiblogos = logos-liblogos.packages.${system}.default;
      });
    in
    moduleOutputs // {
      packages = forAllSystems ({ pkgs, logosSdk, logosLiblogos }:
        let
          base = moduleOutputs.packages.${pkgs.system} or {};

          ui-plugin = pkgs.stdenv.mkDerivation {
            pname = "scala-ui-plugin";
            version = "0.1.0";
            src = ./.;

            nativeBuildInputs = [
              pkgs.cmake
              pkgs.ninja
              pkgs.pkg-config
              pkgs.qt6.wrapQtAppsHook
            ];

            buildInputs = [
              pkgs.qt6.qtbase
              pkgs.qt6.qtdeclarative
              pkgs.qt6.qtremoteobjects
            ];

            cmakeFlags = [
              "-DBUILD_UI_PLUGIN=ON"
              "-DLOGOS_CPP_SDK_ROOT=${logosSdk}"
              "-DLOGOS_LIBLOGOS_ROOT=${logosLiblogos}"
            ];

            buildPhase = ''
              runHook preBuild
              cmake --build . --target scala_ui -j''${NIX_BUILD_CORES:-1}
              runHook postBuild
            '';

            installPhase = ''
              runHook preInstall
              mkdir -p $out/lib
              cp libscala_ui${pkgs.stdenv.hostPlatform.extensions.sharedLibrary} $out/lib/
              runHook postInstall
            '';

            dontWrapQtApps = true;

            meta = with pkgs.lib; {
              description = "Scala UI plugin (IComponent) for logos-app-poc";
              platforms = platforms.unix;
            };
          };
        in
        base // {
          ui = pkgs.runCommand "scala-ui" {} ''
            mkdir -p $out/qml
            cp -r ${./qml}/* $out/qml/
          '';
          inherit ui-plugin;
        }
      );
    };
}

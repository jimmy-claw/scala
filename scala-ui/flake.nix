{
  description = "Scala Calendar UI — QML view with C++ backend for Logos";

  inputs = {
    # Use master (not 0.2.0 tag) — 0.2.0 pins old cpp-sdk missing lidl-frontend files
    logos-module-builder.url = "github:logos-co/logos-module-builder";

    # Points at scala core module. Override for local dev:
    # nix flake update --override-input scala path:/home/vpavlin/scala
    scala.url = "github:jimmy-claw/scala";
  };

  outputs = inputs@{ logos-module-builder, scala, ... }:
    logos-module-builder.lib.mkLogosQmlModule {
      src = ./.;
      configFile = ./metadata.json;
      flakeInputs = inputs;
    };
}

{
  description = "Scala Calendar UI — QML view with C++ backend for Logos";

  inputs = {
    logos-module-builder.url = "github:logos-co/logos-module-builder/0.2.0";

    # Points at your local scala checkout. Override with:
    # nix flake update --override-input scala github:jimmy-claw/scala
    scala.url = "path:/path/to/your/scala";
  };

  outputs = inputs@{ logos-module-builder, scala, ... }:
    logos-module-builder.lib.mkLogosQmlModule {
      src = ./.;
      configFile = ./metadata.json;
      flakeInputs = inputs;
    };
}

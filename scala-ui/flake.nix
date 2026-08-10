{
  description = "Scala Calendar UI — pure-QML view over the scala core module";

  inputs = {
    # Pin to the SAME builder the scala core uses (full 40-char SHA) so the view,
    # core and host stay on one SDK. An unpinned builder was building the view
    # against a different rev than the core — part of why it never loaded.
    logos-module-builder.url = "github:logos-co/logos-module-builder/afe4430ee6eb7ba45c08a516a43e18500720c715";
    scala.url = "github:jimmy-claw/scala";
    scala.inputs.logos-module-builder.follows = "logos-module-builder";
  };

  outputs = inputs@{ logos-module-builder, scala, ... }:
    logos-module-builder.lib.mkLogosQmlModule {
      src = ./.;
      configFile = ./metadata.json;
      flakeInputs = inputs;
    };
}

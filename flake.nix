{
  description = "Secure Calendar App — core module for Logos";

  inputs = {
    logos-module-builder.url = "github:logos-co/logos-module-builder/0.2.0";

    # Dependency modules (for LIDL contract consumption)
    # kv_module uses transitional fallback (old format, no lidl output)
    kv_module.url = "github:jimmy-claw/logos-kv-module";
  };

  outputs = inputs@{ logos-module-builder, ... }:
    logos-module-builder.lib.mkLogosModule {
      src = ./.;
      configFile = ./metadata.json;
      flakeInputs = inputs;
    };
}

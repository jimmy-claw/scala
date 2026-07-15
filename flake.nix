{
  description = "Secure Calendar App — core module for Logos";

  inputs = {
    # Use master (not 0.2.0 tag) — 0.2.0 pins old cpp-sdk missing lidl-frontend files
    logos-module-builder.url = "github:logos-co/logos-module-builder";

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

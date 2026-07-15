{
  description = "Secure Calendar App — core module for Logos";

  inputs = {
    # Use master (not 0.2.0 tag) — 0.2.0 pins old cpp-sdk missing lidl-frontend files
    logos-module-builder.url = "github:logos-co/logos-module-builder";

    # TODO: re-add kv_module after it's migrated to universal pattern
    # (old-format generates incompatible SDK wrappers in logos-module-builder)
    # kv_module.url = "github:jimmy-claw/logos-kv-module";
  };

  outputs = inputs@{ logos-module-builder, ... }:
    logos-module-builder.lib.mkLogosModule {
      src = ./.;
      configFile = ./metadata.json;
      flakeInputs = inputs;
    };
}

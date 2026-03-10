{
  description = "Secure Calendar App — privacy-first shared calendar module for Logos Core";

  inputs = {
    logos-module-builder.url = "github:logos-co/logos-module-builder";
    nixpkgs.follows = "logos-module-builder/nixpkgs";
  };

  outputs = { self, logos-module-builder, nixpkgs, ... }:
    logos-module-builder.lib.mkLogosModule {
      src = ./.;
      configFile = ./module.yaml;
    };
}

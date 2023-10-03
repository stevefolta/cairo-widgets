{
	description = "Simple widget library using Cairo";

	inputs = {
		};

	outputs = { self, nixpkgs }:
		let
			project-name = "cairo-widgets";

			# Why doesn't the flakes build system handle this automatically?!
			forAllSystems = nixpkgs.lib.genAttrs nixpkgs.lib.systems.flakeExposed;
			nixpkgsFor = forAllSystems (system: import nixpkgs { inherit system; });
		in {
			packages = forAllSystems (system: {
				default =
					nixpkgsFor.${system}.stdenv.mkDerivation {
						name = project-name;
						src = self;
						buildInputs = with nixpkgsFor.${system}; [ cairo ];
						installPhase = ''
							mkdir -p $out/bin
							cp cairo-widgets-test $out/bin/
							'';
						};
					});
			};
}



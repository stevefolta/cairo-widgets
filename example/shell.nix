{ pkgs ? import <nixpkgs> {} }:

pkgs.mkShell {
	buildInputs = with pkgs; [ cairo xorg.xcbutilkeysyms libxkbcommon pkg-config ];
	}




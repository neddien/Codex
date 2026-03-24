#!/usr/bin/perl

require "./dsfx.pl";

use Cwd qw(cwd);
use File::Path qw(remove_tree);

$cwd = cwd;

dsfx::logln("Erasing all build files except the projects...");
dsfx::logln("Current working directory: '$cwd'");

my @files_to_erase = (
	"../CODEX.sln",
	"../ZERO_CHECK.vcxproj",
	"../ZERO_CHECK.vcxproj.filters",
	"../ZERO_CHECK.vcxproj.user",
	"../ALL_BUILD.vcxproj",
	"../ALL_BUILD.vcxproj.filters",
	"../ALL_BUILD.vcxproj.user",
	"../cmake_install.cmake",
	"../CMakeCache.txt",
	"../INSTALL.vcxproj",
	"../INSTALL.vcxproj.filters",
	"../INSTALL.vcxproj.user",
	"../RSE/INSTALL.vcxproj",
	"../RSE/INSTALL.vcxproj.filters",
	"../compile_commands.json"
);

my @dirs_to_erase = (
	"../MARFX",
	"../fmt",
	"../SDL",
	"../SDL_image",
	"../yaml-cpp",
	"../x64",
	"../CMakeFiles",
	"../build",
	"../linux-build",
	"../out",
	"../bin",
	"../glew",
	"../glm",
	"../lib",
	"../builds",
	"../CMakeFiles"
);

foreach my $f (@files_to_erase) {
	if (!unlink $f) {
		if (-e $f) { 
			dsfx::lerrln("Failed to erase file '$f'.");
		}	
	}
}

foreach my $d (@dirs_to_erase) {
	if (!remove_tree($d)) {
		if (-e $d) {
			dsfx::lerrln("Failed to erase directory '$d'.");
		}
	}
}

dsfx::logln("Done.");

1;

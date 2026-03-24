#!/usr/bin/perl

require "./dsfx.pl";

dsfx::logln("Visual Studio 2022 build file generation started.");

my $exit_code = system("cmake", "-G", "\"Visual Studio 17\" ../ -B ../");

if ($exit_code == 0) {
	dsfx::logln("Visual Studio 2022 build file generation finished!");
} else {
	dsfx::lerrln("Generation failed!");
	-1;
}

1;

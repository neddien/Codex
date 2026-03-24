#!/usr/bin/perl

require "./dsfx.pl";

dsfx::logln("Build started.");

if (dsfx::util::get_os() eq "mswin32") {
		dsfx::logln("Windows NT platform detected. Automatically using MSBuild.exe as the build system.");
		dsfx::logln("You must have MSVC with VS installed.");
		dsfx::logln("Starting build...");
		if (!-e "../RSE.sln") {
			dsfx::logln("Visual Solution file does not exist. Generating one...");
			my $exit_code = system("perl generate_build_vs.pl");
			if ($exit_code != 0) {
				dsfx::lerrln("Visual Solution file generation failed! Aborting...");
				exit -1;		
			}
		}
		my $exit_code = system("msbuild ../RSE.sln");
		if ($exit_code != 0) {
			dsfx::lerrln("Build failed! Aborting...");
			exit -1;	
		}	
		dsfx::logln("Build finished!");
		dsfx::logln("Executing program...");
		my $exit_code = system("../TestGame/Debug/TestGame");
		dsfx::logln("Program exited with code: $exit_code");	
} else {
	dsfx::logln("Detected platform: ${\dsfx::util::get_os()}");
	dsfx::logln("Most likley an Unix like system so using CMake should be fine.");
	if (!-e "../${\dsfx::util::get_os()}") {
		dsfx::logln("CMake build files are not present. Generating build files...");
		my $exit_code = system("generate_build_cmake.pl");
		if ($exit_code != 0) {
			dsfx::lerrln("CMake build file generation failed! Aborting...");
			exit -1;	
		}	
		my $exit_code = system("cmake --build ../${\dsfx::util::get_os()}");
		if ($exit_code != 0) {
			dsfx::lerrln("Build failed! Aborting...");
			exit -1;
		}
		dsfx::logln("Build finished!");
		dsfx::logln("Launching program...");
		my $exit_code = system("../${\}");
	}
}


1;

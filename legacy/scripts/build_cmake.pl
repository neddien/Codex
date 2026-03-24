#!/usr/bin/perl

require "./dsfx.pl";

use File::Path qw(remove_tree);
use File::Copy;
use File::Copy::Recursive qw(dircopy);

$config_file_present = 0;
$os = dsfx::util::get_os();
$config = "debug";
$arch = dsfx::util::get_arch();
$cxx_compiler = "none";
$nuller = "null";
$vs_dev_env = "cmd /C \"C:/Program Files/Microsoft Visual Studio/2022/Community/Common7/Tools/VsDevCmd.bat\"";

# Check for the config file
if (-e "dsfx_conf.t") {
	open(my $file_handler, "<", "dsfx_conf.t") or die "Could not open config file: $!";
	my $f_conf = do { local $/; <$file_handler> };
	my @f_lines = split('\n', $f_conf);
	foreach my $line (@f_lines) {
		my @tokens = split(' ', $line);
		if (@tokens[0] eq "os") {
			$os = substr($line, length(@tokens[0]) + 3);
		}
		elsif (@tokens[0] eq "config") {
			# config
			$config = substr($line, length(@tokens[0]) + 3);
		}
		elsif (@tokens[0] eq "arch") {
			$arch = substr($line, length(@tokens[0]) + 3);
		}
		elsif (@tokens[0] eq "cxx_compiler") {
			$cxx_compiler = substr($line, length(@tokens[0]) + 3);
		}
		elsif (@tokens[0] eq "nuller") {
			$nuller = substr($line, length(@tokens[0]) + 3);
		}
		elsif (@tokens[0] eq "vs_dev_env") {
			$vs_dev_env = substr($line, length(@tokens[0]) + 3);
		}
	}
	close($file_handler);
	$config_file_present = 1;
}

# Argument parsing
if (scalar(@ARGV) > 0 && $config_file_present == 0) {
	for (my $i = 0; $i < scalar(@ARGV); ++$i) {
		if (lc $ARGV[$i] eq "--config") {
			$config = lc $ARGV[++$i];
		} elsif (lc $ARGV[$i] eq "--arch") {
			$arch = lc $ARGV[++$i];
		} elsif (lc $ARGV[$i] eq "--help") {
			print("USAGE:\n\t<no args>\n\t\t" . 'Builds the project with default configuration settings.');
			print("\n\t--help:\n\t\t" . 'Displays this message.');
			print("\n\t--arch:\n\t\t" . 'Set the build architecture (x64 or x86).');
			print("\n\t--cofnig:\n\t\t" . 'Set the build configuration (Debug, Release or Shipping).');
			print("\n");
			exit 0;
		}
	}	
} elsif ($config_file_present == 0) {
	dsfx::warn("Warning! Build was not configured, defaulting to:\n\tCONFIGURATION: $config\n\tARCHITECTURE: $arch");
	dsfx::warn("Run this script with the '--help' next time.");
}

dsfx::logln("Build for Ninja started");

if ($config_file_present == 0) {
	dsfx::logln("Searching for compilers...");
	if ($os eq "mswin32") {
		#dsfx::logln("Windows platform detected.");
		$nuller = "> nul 2>&1";
		# Check for MSVC
		if (system("\"C:/Program Files (x86)/Microsoft Visual Studio/Installer/vswhere.exe\" $nuller") == 0) {
			my $vs_install_path = qx("C:/Program Files (x86)/Microsoft Visual Studio/Installer/vswhere.exe" -property installationPath");
			#dsfx::logln("Visual Studio install path: $vs_install_path");
			my $exit_code = system("$vs_dev_env && cl");
			if ($exit_code == 0) {
				$cxx_compiler = "cl";
			}
		}
		
		# Check for alternatives if MSVC was not found
		if ($cxx_compiler eq "none") {
			if (system("clang --version $nuller") == 0) {
				$cxx_compiler = "clang";
			} elsif (system("g++ --version $nuller") == 0) {
				$cxx_compiler = "g++";
			} 	
		}
	} else {
		dsfx::logln("Detected platform: $os");
		$nuller = "> /dev/null 2>&1";
		if (system("clang --version $nuller") == 0) {
			$cxx_compiler = "clang";
		} elsif (system("g++ --version $nuller") == 0) {
			$cxx_compiler = "g++";
		}
	}
	dsfx::logln("Found compiler: $cxx_compiler");
}

if (!-e lc "../builds/$os-$config-$arch") {
	dsfx::logln("Ninja build files are not present. Generating ninja build files...");
	mkdir("../builds");
	if ($cxx_compiler eq "cl") {	
		my $build_path = lc "../builds/$os-$config-$arch";
		my $exit_code = system("$vs_dev_env && cmake -G \"Ninja\" ../ -B $build_path -DCMAKE_BUILD_TYPE=$config -DCMAKE_BUILD_ARCH=$arch");
		if ($exit_code != 0) {
			dsfx::lerrln("CMake build generation for Ninja failed! Aborting...");
			remove_tree("../builds/$os-$config-$arch");
			exit -1;
		}
		dsfx::logln("Ninja build file generation finished.");
	} else {
		my $exit_code = system("cmake -G \"Ninja\" ../ -B ../builds/$os-$config-$arch -DCMAKE_BUILD_TYPE=$config -DCMAKE_BUILD_ARCH=$arch");
		if ($exit_code != 0) {
			dsfx::lerrln("CMake build generation for Ninja failed! a Aborting...");
			remove_tree("../builds/$os-$config-$arch");
			exit -1;
		}
		dsfx::logln("Ninja build file generation finished.");
	}

	# Copy compile_commands.json for clangd symlinks don't work for some reason
	copy("../builds/$os-$config-$arch/compile_commands.json", "../");
}

dsfx::logln("Building project...");
if ($os eq "mswin32") { 
	my $exit_code = system("$vs_dev_env && cd .. && cmake --build builds/$os-$config-$arch/");
	if ($exit_code != 0) {
		dsfx::lerrln("Build failed! Aborting...");
		exit -1;
	}
} else {
	my $exit_code = system("cmake --build ../builds/$os-$config-$arch");
	if ($exit_code != 0) {
		dsfx::lerrln("Build failed! Aborting...");
		exit -1;
	}
}

# Create the config file if needed
if ($config_file == 0) {
	open(my $fs, ">", "dsfx_conf.t") or die "Could not open config file for writing: $!";
	#my $\ = "\n";
	print $fs "os = " 			. $os				.	"\n";
	print $fs "config = " 		. $config			.	"\n";
	print $fs "arch = "			. $arch				.	"\n";
	print $fs "cxx_compiler = "	. $cxx_compiler		.	"\n";
	print $fs "nuller = "		. $nuller			.	"\n";
	print $fs "vs_dev_env = "	. $vs_dev_env		.	"\n";
	close($fs);
}

# Copy the assets
#my @assets = glob("../data");
#foreach my $f (@assets) {
#	if (-d $f) {
#		dircopy($f, "../builds/$os-$config-$arch/TestGame");
#	} else {
#		copy($f, "../builds/$os-$config-$arch/TestGame");
#	}
#}

dsfx::logln("Build finished.");

1;

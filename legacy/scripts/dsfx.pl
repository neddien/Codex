#!/usr/bin/perl

package dsfx::ansi;
%colours = (
	red		=> "\e[31m",
	green	=> "\e[32m",
	yellow	=> "\e[33m",
	blue	=> "\e[34m",
	magenta	=> "\e[35m",
	cyan	=> "\e[36m",
	lblue	=> "\e[117m",
	rst		=> "\e[0m"
);

package dsfx;
sub logln {
	my $str = shift;
	print $dsfx::ansi::colours{'cyan'}, "[~] :: $str\n", $dsfx::ansi::colours{'rst'};
}
sub lerrln {
	my $str = shift;
	print $dsfx::ansi::colours{'red'}, "[!] :: $str\n", $dsfx::ansi::colours{'rst'};
}
sub warn {
	my $str = shift;
	print $dsfx::ansi::colours{'yellow'}, "[?] :: $str\n", $dsfx::ansi::colours{'rst'};
}

package dsfx::util;
use Cwd qw(cwd);
use English qw' -no_match_vars ';
use Config;

sub get_os {
	return (lc "$OSNAME");
}
sub get_arch {
	my $arch = $Config{archname};
	my $ret = "unknown";
	if ($arch =~ /x86_64/) {
		$ret = "x64";
	} elsif ($arch =~ /i386|i586|i686/) {
		$ret = "x86";
	} else {
		$ret = "$arch";
	}
	return $ret;
}
sub get_cwd {
	return ($CWD);
}

1;

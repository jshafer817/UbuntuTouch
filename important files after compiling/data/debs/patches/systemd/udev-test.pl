#!/usr/bin/perl

# udev test
#
# Provides automated testing of the udev binary.
# The whole test is self contained in this file, except the matching sysfs tree.
# Simply extend the @tests array, to add a new test variant.
#
# Every test is driven by its own temporary config file.
# This program prepares the environment, creates the config and calls udev.
#
# udev parses the rules, looks at the provided sysfs and
# first creates and then removes the device node.
# After creation and removal the result is checked against the
# expected value and the result is printed.
#
# Copyright (C) 2004-2012 Kay Sievers <kay@vrfy.org>
# Copyright (C) 2004 Leann Ogasawara <ogasawara@osdl.org>

use warnings;
use strict;

my $udev_bin            = "./test-udev";
my $valgrind            = 0;
my $udev_bin_valgrind   = "valgrind --tool=memcheck --leak-check=yes --quiet $udev_bin";
my $udev_dev            = "test/dev";
my $udev_run            = "test/run";
my $udev_rules_dir      = "$udev_run/udev/rules.d";
my $udev_rules          = "$udev_rules_dir/udev-test.rules";

my @tests = (
);

sub udev {
}



sub permissions_test {
}

sub major_minor_test {
}

sub udev_setup {
}

sub run_test {
        
}


udev_setup();

exit(0);

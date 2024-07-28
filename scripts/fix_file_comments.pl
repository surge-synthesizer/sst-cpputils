#!/usr/bin/perl


use File::Find;
use File::Basename;

find(
    {
        wanted => \&findfiles,
    },
    'include'
);

find(
    {
        wanted => \&findfiles,
    },
    'tests'
);

sub findfiles {

    $header = <<EOH;
/*
 * sst-cpputils - an open source library of things we needed in C++
 * built by Surge Synth Team.
 *
 * Provides a collection of tools useful for writing C++-17 code
 *
 * Copyright 2022-2024, various authors, as described in the GitHub
 * transaction log.
 *
 * sst-cpputils is released under the MIT License found in the "LICENSE"
 * file in the root of this repository
 *
 * All source in sst-cpputils available at
 * https://github.com/surge-synthesizer/sst-cpputils
 */
EOH

    $f = $File::Find::name;
    if ($f =~ m/\.h$/ or $f =~ m/.cpp$/) {
        #To search the files inside the directories
        print "Processing $f\n";

        $q = basename($f);
        print "$q\n";
        open(IN, "<$q") || die "Cant open IN $!";
        open(OUT, "> ${q}.bak") || die "Cant open BAK $!";

        $nonBlank = 0;
        $inComment = 0;
        while (<IN>) {
            if ($nonBlank) {
                print OUT
            }
            else {
                if (m:^\s*/\*:) {
                    $inComment = 1;
                }
                elsif (m:\s*\*/:) {
                    print OUT $header;
                    $nonBlank = true;
                    $inComment = false;
                }
                elsif ($inComment) {

                }
                elsif (m:^//:) {

                }
                else {
                    print OUT $header;
                    $nonBlank = true;
                    print OUT;

                }
            }
        }
        close(IN);
        close(OUT);
        system("mv ${q}.bak ${q}");
        system("clang-format -i ${q}");
    }
}

#!/usr/bin/tclsh
# Left parenthesis should not be followed by whitespace
# Right parenthesis should not be preceded by whitespace

foreach f [getSourceFileNames] {
    foreach t [getTokens $f 1 0 -1 -1 {leftparen rightparen}] {
        set line [lindex $t 1]
        set column [lindex $t 2]
        set tokenType [lindex $t 3]

        if {$tokenType == "leftparen"} {
            set following [getTokens $f $line [expr $column + 1] [expr $line + 1] -1 {}]
            if {$following != {}} {
                set firstFollowing [lindex [lindex $following 0] 3]
                if {$firstFollowing == "space"} {
                    report $f $line "left parenthesis should not be followed by whitespace"
                }
            }
        }

        if {$tokenType == "rightparen"} {
            set preceding [getTokens $f $line 0 $line $column {}]
            set lastPreceding [lindex [lindex $preceding end] 3]
            if {[llength $preceding] > 1 && $lastPreceding == "space"} {
                report $f $line "right parenthesis should not be preceded by whitespace"
            }
        }
    }
}

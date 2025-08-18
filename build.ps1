$entry = "main.c"
$rc = "blanker.rc"
$res = "blanker.res"
$output = "blanker.exe"
$warnings = "-Wall","-Wextra","-Werror"
$libs = "user32","gdi32"
$verbose = ""
$run = 0
$release = 0
$rc_exe = "rc"
$cc = "clang"

$i = 0
foreach ($arg in $args) {
    if ($arg -eq "-v") { $verbose = "-v" }
    elseif ($arg -eq "run") { $run = 1 }
    elseif ($arg -eq "clean") {
        rm -force *.exe
        rm -force *.pdb
        rm -force *.res
        rm -force *.rdi
        rm -force *.ilk
        rm -force *.obj

        exit 0
    }
    elseif ($arg -eq "--release") { $release = 1 }
    elseif ($arg -eq "--rc") { 
        if ($args.Length -gt $i + 1) {
            $rc_exe = "$($args[$i + 1])" 
        } else {
            echo "no path provided to --rc"
            exit 1
        }
    }
    elseif ($arg -eq "--cc") { 
        if ($args.Length -gt $i + 1) {
            $cc = "$($args[$i + 1])" 
        } else {
            echo "no path provided to --cc"
            exit 1
        }
    }
    $i++
}

function check_cc {
    if ($cc -eq "clang-cl") {
        return $true
    } elseif ($cc -eq "cl") {
        return $true
    } else {
        return $false
    }
}

function format_opts {
    if ($release -eq 1) {
        if (check_cc -eq $true) {
            return "/O2"
        } else {
            return "-O2"
        }
    } else {
        if (check_cc -eq $true) {
            return ""
        } else {
            return "-g"
        }
    }
}

function format_warnings {
    if (check_cc -eq $true) {
        return "/W4","/WX"
    } else {
        return $warnings
    }
}

function format_libs {
    if (check_cc -eq $true) {
        return $libs | % { "$_.lib" } 
    } else {
        return $libs | % { "-l$_" } 
    }
}

function format_output {
    if (check_cc -eq $true) {
        return @("/Fe:$output")
    } else {
        return "-o","$output"
    }
}

echo "Entry: $entry"
echo "Output: $output"
echo "Warnings: $warnings"
echo "Libs: $libs"

try {
    $rc_exe = Get-Command $rc_exe -ErrorAction Stop
} catch {
    echo "rc not found in path. use VS PowerShell or --rc <path> to declare manually"
    exit 1
}

try {
    $_ = Get-Command $cc -ErrorAction Stop
} catch {
    echo "$cc not found in path."
    exit 1
}

& $rc_exe "$rc"

& $cc "$entry" "$res" @(format_opts) @(format_warnings) @(format_libs) @(format_output)

if ($run -eq 1) {
    & ".\$output"
}

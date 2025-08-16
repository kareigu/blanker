$entry = "main.c"
$rc = "blanker.rc"
$res = "blanker.res"
$output = "blanker.exe"
$warnings = "-Wall","-Werror"
$libs = @("user32","gdi32") | % { "-l$_"}
$verbose = ""
$run = 0
$opt = "-g"
$rc_exe = "rc"

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

        exit 0
    }
    elseif ($arg -eq "--release") { $opt = "-O3" }
    elseif ($arg -eq "--rc") { 
        if ($args.Length -gt $i + 1) {
            $rc_exe = "$($args[$i + 1])" 
        } else {
            echo "no path provided to --rc"
            exit 1
        }
    }
    $i++
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

& $rc_exe "$rc"

clang "$entry" $res $opt $verbose @warnings @libs -o $output

if ($run -eq 1) {
    & ".\$output"
}

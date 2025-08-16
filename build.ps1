$entry = "main.c"
$output = "blanker.exe"
$warnings = "-Wall","-Werror"
$libs = @("user32","gdi32") | % { "-l$_"}
$verbose = ""
$run = 0
$opt = "-g"

foreach ($arg in $args) {
    if ($arg -eq "-v") { $verbose = "-v" }
    elseif ($arg -eq "run") { $run = 1 }
    elseif ($arg -eq "--release") { $opt = "-O3" }
}

echo "Entry: $entry"
echo "Output: $output"
echo "Warnings: $warnings"
echo "Libs: $libs"

clang "$entry" $opt $verbose @warnings @libs -o $output

if ($run -eq 1) {
    & ".\$output"
}

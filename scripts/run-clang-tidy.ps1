param(
    [Parameter(Mandatory = $true, Position = 0)]
    [string]$BuildDir,

    [Parameter(Position = 1)]
    [string[]]$Files
)

$ErrorActionPreference = "Stop"

$workspaceDir = Split-Path -Parent $PSScriptRoot

function Resolve-WorkspacePath {
    param([string]$PathValue)

    if ([System.IO.Path]::IsPathRooted($PathValue)) {
        return [System.IO.Path]::GetFullPath($PathValue)
    }

    return [System.IO.Path]::GetFullPath((Join-Path $workspaceDir $PathValue))
}

function Resolve-InputFile {
    param([string]$ResolvedPath)

    if ($ResolvedPath -match '\.(c|cc|cpp|cxx|m|mm)$') {
        return $ResolvedPath
    }

    $candidateExtensions = @(".cpp", ".cc", ".cxx", ".c", ".mm", ".m")
    $basePath = [System.IO.Path]::ChangeExtension($ResolvedPath, $null)

    foreach ($extension in $candidateExtensions) {
        $candidatePath = $basePath + $extension
        if (Test-Path $candidatePath) {
            return $candidatePath
        }
    }

    return $ResolvedPath
}

function Remove-UnsupportedCompileFlags {
    param([string[]]$Arguments)

    $result = [System.Collections.Generic.List[string]]::new()
    $skipNext = $false

    foreach ($argument in $Arguments) {
        if ($skipNext) {
            $skipNext = $false
            continue
        }

        if ($argument -eq "-fmodules-ts") {
            continue
        }

        if ($argument -eq "-fmodule-mapper" -or $argument -eq "-fdeps-format") {
            $skipNext = $true
            continue
        }

        if ($argument.StartsWith("-fmodule-mapper=", [System.StringComparison]::Ordinal) -or
            $argument.StartsWith("-fdeps-format=", [System.StringComparison]::Ordinal)) {
            continue
        }

        $result.Add($argument)
    }

    return $result.ToArray()
}

function New-TidyCompileDatabase {
    param(
        [string]$ResolvedBuildDir,
        [string]$CompileCommandsPath
    )

    $raw = Get-Content $CompileCommandsPath -Raw
    if ($raw -notmatch '-fmodules-ts|-fmodule-mapper=|-fdeps-format=') {
        return $ResolvedBuildDir
    }

    $tempDir = Join-Path ([System.IO.Path]::GetTempPath()) ([System.IO.Path]::GetRandomFileName())
    New-Item -ItemType Directory -Path $tempDir | Out-Null
    $sanitizedPath = Join-Path $tempDir "compile_commands.json"

    $database = $raw | ConvertFrom-Json
    foreach ($entry in $database) {
        if ($entry.PSObject.Properties.Name -contains "arguments") {
            $entry.arguments = @(Remove-UnsupportedCompileFlags -Arguments $entry.arguments)
        }
        if ($entry.PSObject.Properties.Name -contains "command") {
            $command = $entry.command
            $command = $command -replace '\s-fmodules-ts(?=\s|$)', ''
            $command = $command -replace '\s-fmodule-mapper(=|\s+)[^\s]+(?=\s|$)', ''
            $command = $command -replace '\s-fdeps-format(=|\s+)[^\s]+(?=\s|$)', ''
            $entry.command = $command
        }
    }

    $database | ConvertTo-Json -Depth 20 | Set-Content -Path $sanitizedPath -Encoding UTF8
    return $tempDir
}

function Invoke-Tidy {
    param(
        [string]$ResolvedBuildDir,
        [string]$ResolvedFile
    )

    $separator = [Regex]::Escape([System.IO.Path]::DirectorySeparatorChar)
    $workspaceRegex = [Regex]::Escape("$workspaceDir" + [System.IO.Path]::DirectorySeparatorChar)
    $projectHeaderRegex = "^$workspaceRegex(app|core|ui|tests)$separator"
    $output = & clang-tidy `
        $ResolvedFile `
        -p $ResolvedBuildDir `
        "--config-file=$workspaceDir/.clang-tidy" `
        "--header-filter=$projectHeaderRegex" 2>&1
    $exitCode = $LASTEXITCODE

    $output | Where-Object {
        $_ -notmatch '^\d+ warnings generated\.$' -and
        $_ -notmatch '^Suppressed \d+ warnings .*' -and
        $_ -notmatch '^Use -header-filter=.*$'
    } | Select-ProjectDiagnostics

    if ($exitCode -ne 0) {
        exit $exitCode
    }
}

function Select-ProjectDiagnostics {
    begin {
        $separator = [Regex]::Escape([System.IO.Path]::DirectorySeparatorChar)
        $workspaceRegex = [Regex]::Escape("$workspaceDir" + [System.IO.Path]::DirectorySeparatorChar)
        $projectDiagnosticRegex = "^$workspaceRegex(app|core|ui|tests)$separator.*:\d+:\d+: (warning|error):"
        $diagnosticRegex = '^[^\s].*:\d+:\d+: (warning|error):'
        $block = [System.Collections.Generic.List[string]]::new()
        $inBlock = $false
        $keepBlock = $true
    }
    process {
        if ($_ -match $diagnosticRegex) {
            if ($inBlock -and $keepBlock) {
                $block
            }
            $block = [System.Collections.Generic.List[string]]::new()
            $block.Add($_)
            $inBlock = $true
            $keepBlock = ($_ -match $projectDiagnosticRegex)
            return
        }

        if ($inBlock) {
            $block.Add($_)
            return
        }

        $_
    }
    end {
        if ($inBlock -and $keepBlock) {
            $block
        }
    }
}

$resolvedBuildDir = Resolve-WorkspacePath $BuildDir
$compileCommands = Join-Path $resolvedBuildDir "compile_commands.json"

if (-not (Test-Path $compileCommands)) {
    Write-Error "clang-tidy: compile database not found at $compileCommands. Run the matching configure/build task first."
}

$tidyBuildDir = New-TidyCompileDatabase -ResolvedBuildDir $resolvedBuildDir -CompileCommandsPath $compileCommands

if ($Files -and $Files.Count -gt 0) {
    $resolvedFiles = $Files | ForEach-Object { Resolve-InputFile (Resolve-WorkspacePath $_) }
} else {
    $resolvedFiles = Get-Content $compileCommands -Raw `
        | ConvertFrom-Json `
        | Select-Object -ExpandProperty file `
        | ForEach-Object { Resolve-WorkspacePath $_ } `
        | Where-Object {
            $_.StartsWith($workspaceDir, [System.StringComparison]::OrdinalIgnoreCase) -and
            -not $_.StartsWith($resolvedBuildDir, [System.StringComparison]::OrdinalIgnoreCase) -and
            $_ -match '\.(c|cc|cpp|cxx|m|mm)$'
        } `
        | Sort-Object -Unique
}

if (-not $resolvedFiles -or $resolvedFiles.Count -eq 0) {
    Write-Error "clang-tidy: no project source files found in $compileCommands"
}

Write-Host "clang-tidy: checking $($resolvedFiles.Count) file(s) from $resolvedBuildDir"

foreach ($resolvedFile in $resolvedFiles) {
    Invoke-Tidy -ResolvedBuildDir $tidyBuildDir -ResolvedFile $resolvedFile
}

if ($tidyBuildDir -ne $resolvedBuildDir) {
    Remove-Item -Recurse -Force $tidyBuildDir
}

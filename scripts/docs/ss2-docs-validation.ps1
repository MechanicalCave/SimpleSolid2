function Test-RequiredStructure {
    param(
        [Parameter(Mandatory=$true)][string]$RepoRoot,
        [Parameter(Mandatory=$true)]$Errors,
        [bool]$RequireBrowser = $true
    )
    $required = @(
        "governance\\DOCUMENTATION.md",
        "docs\\internal\\OVERVIEW.md",
        "docs\\internal\\PROJECT_PLATFORM.md",
        "docs\\internal\\APPLICATION_LIFECYCLE.md",
        "docs\\internal\\PERSISTENCE.md",
        "docs\\internal\\UI_PROJECT_HUB.md",
        "docs\\internal\\BUILD_AND_TEST.md",
        "docs\\product\\pl\\OVERVIEW.md",
        "docs\\product\\pl\\PROJECTS.md",
        "docs\\product\\en\\OVERVIEW.md",
        "docs\\product\\en\\PROJECTS.md",
        "docs\\browser\\template.html"
    )
    if ($RequireBrowser) {
        $required += "docs\\browser\\index.html"
    }
    foreach ($rel in $required) {
        if (-not (Test-Path -LiteralPath (Join-Path $RepoRoot $rel) -PathType Leaf)) {
            Add-DocError $Errors "STRUCTURE" "Missing required documentation file: $rel"
        }
    }
}

function Test-MetadataAndPairs {
    param(
        [Parameter(Mandatory=$true)][string]$RepoRoot,
        [Parameter(Mandatory=$true)]$Errors
    )

    $allIds = @{}
    foreach ($path in Get-CanonicalDocPaths $RepoRoot) {
        $meta = Get-DocMetadata $path
        $rel = Get-RelativeRepoPath $RepoRoot $path
        if (-not $meta.DocId) { Add-DocError $Errors "META" "$rel is missing doc-id."; continue }
        if ($meta.Kind -ne "product" -and $meta.Kind -ne "internal") {
            Add-DocError $Errors "META" "$rel has unsupported document-kind '$($meta.Kind)'."
        }
        if ($meta.Text -match '(?i)</script') {
            Add-DocError $Errors "META" "$rel contains forbidden </script text that would break the self-contained Browser."
        }

        $key = "$($meta.Kind)|$(Get-DocLanguage $RepoRoot $path $meta.Kind)|$($meta.DocId)"
        if ($allIds.ContainsKey($key)) {
            Add-DocError $Errors "META" "Duplicate doc-id '$($meta.DocId)' for the same kind/language."
        } else {
            $allIds[$key] = $rel
        }

        $seenSections = @{}
        foreach ($section in $meta.Sections) {
            if ($seenSections.ContainsKey($section)) {
                Add-DocError $Errors "META" "$rel contains duplicate section-id '$section'."
            } else {
                $seenSections[$section] = $true
            }
        }
    }

    $plDir = Join-Path $RepoRoot "docs\\product\\pl"
    $enDir = Join-Path $RepoRoot "docs\\product\\en"
    if (-not (Test-Path $plDir) -or -not (Test-Path $enDir)) { return }

    $pl = @{}
    Get-ChildItem -LiteralPath $plDir -Filter "*.md" -File | ForEach-Object { $pl[$_.Name] = $_.FullName }
    $en = @{}
    Get-ChildItem -LiteralPath $enDir -Filter "*.md" -File | ForEach-Object { $en[$_.Name] = $_.FullName }

    foreach ($name in @($pl.Keys | Sort-Object)) {
        if (-not $en.ContainsKey($name)) {
            Add-DocError $Errors "PAIR" "Missing EN product pair for $name."
            continue
        }
        $plMeta = Get-DocMetadata $pl[$name]
        $enMeta = Get-DocMetadata $en[$name]
        if ($plMeta.DocId -ne $enMeta.DocId) {
            Add-DocError $Errors "PAIR" "$name uses different PL/EN doc-id values."
        }
        if ($plMeta.Sections.Count -ne $enMeta.Sections.Count) {
            Add-DocError $Errors "SECTION" "$name uses a different number of PL/EN section-id values."
        } else {
            for ($i = 0; $i -lt $plMeta.Sections.Count; ++$i) {
                if ($plMeta.Sections[$i] -ne $enMeta.Sections[$i]) {
                    Add-DocError $Errors "SECTION" "$name section-id mismatch at position $($i + 1): PL '$($plMeta.Sections[$i])', EN '$($enMeta.Sections[$i])'."
                }
            }
        }
    }
    foreach ($name in @($en.Keys | Sort-Object)) {
        if (-not $pl.ContainsKey($name)) {
            Add-DocError $Errors "PAIR" "Missing PL product pair for $name."
        }
    }
}

function Test-LocalMarkdownLinks {
    param(
        [Parameter(Mandatory=$true)][string]$RepoRoot,
        [Parameter(Mandatory=$true)]$Errors
    )
    foreach ($path in Get-CanonicalDocPaths $RepoRoot) {
        $rel = Get-RelativeRepoPath $RepoRoot $path
        $text = Read-CanonicalMarkdown $path
        foreach ($m in [regex]::Matches($text, '\[[^\]]+\]\(([^)]+)\)')) {
            $target = $m.Groups[1].Value.Trim()
            if (-not $target -or $target.StartsWith("#")) { continue }
            if ($target -match '^[A-Za-z][A-Za-z0-9+.-]*:') { continue }
            $pathPart = $target.Split('#')[0]
            if (-not $pathPart) { continue }
            try { $pathPart = [Uri]::UnescapeDataString($pathPart) } catch {}
            $resolved = Join-Path (Split-Path -Parent $path) $pathPart
            if (-not (Test-Path -LiteralPath $resolved)) {
                Add-DocError $Errors "LINK" "$rel has broken local link target '$target'."
            }
        }
    }
}

function Test-DocumentationImpact {
    param(
        [Parameter(Mandatory=$true)][string]$RepoRoot,
        [Parameter(Mandatory=$true)]$Errors
    )
    $activePath = Join-Path $RepoRoot "work\\ACTIVE.yaml"
    if (-not (Test-Path -LiteralPath $activePath)) {
        Add-DocError $Errors "IMPACT" "work/ACTIVE.yaml is missing."
        return
    }
    $activeText = Read-NormalizedText $activePath
    $m = [regex]::Match($activeText, '(?m)^active_work:\s*["'']?([^"''\r\n]+)')
    if (-not $m.Success) {
        Add-DocError $Errors "IMPACT" "ACTIVE.yaml does not declare active_work."
        return
    }
    $contractRel = $m.Groups[1].Value.Trim()
    $contractPath = Join-Path $RepoRoot $contractRel
    if (-not (Test-Path -LiteralPath $contractPath -PathType Leaf)) {
        Add-DocError $Errors "IMPACT" "Active Work Contract does not exist: $contractRel"
        return
    }

    $contract = Read-NormalizedText $contractPath
    if ($contract -notmatch '(?m)^## Documentation impact\s*$') {
        Add-DocError $Errors "IMPACT" "$contractRel is missing '## Documentation impact'."
    }
    if ($contract -notmatch '(?mi)^Internal docs:\s*(required|not required)\s*$') {
        Add-DocError $Errors "IMPACT" "$contractRel is missing a valid Internal docs declaration."
    }
    if ($contract -notmatch '(?mi)^User/Product docs:\s*(required|not required)\s*$') {
        Add-DocError $Errors "IMPACT" "$contractRel is missing a valid User/Product docs declaration."
    }
    if ($contract -notmatch '(?mi)^Reason:\s*\S.+$') {
        Add-DocError $Errors "IMPACT" "$contractRel is missing a non-empty Documentation Impact reason."
    }
}

function Test-BrowserFreshness {
    param(
        [Parameter(Mandatory=$true)][string]$RepoRoot,
        [Parameter(Mandatory=$true)]$Errors
    )
    $indexPath = Join-Path $RepoRoot "docs\\browser\\index.html"
    $templatePath = Join-Path $RepoRoot "docs\\browser\\template.html"
    if (-not (Test-Path -LiteralPath $indexPath -PathType Leaf) -or
        -not (Test-Path -LiteralPath $templatePath -PathType Leaf)) {
        return
    }
    $expected = New-BrowserHtml $RepoRoot
    $actual = Read-NormalizedText $indexPath
    if ($actual -ne $expected) {
        Add-DocError $Errors "BROWSER" "docs/browser/index.html is stale. Run '.\\ss2.ps1 docs' and commit the generated file."
    }
}

function Test-BrowserContract {
    param(
        [Parameter(Mandatory=$true)][string]$RepoRoot,
        [Parameter(Mandatory=$true)]$Errors
    )
    $templatePath = Join-Path $RepoRoot "docs\\browser\\template.html"
    if (-not (Test-Path -LiteralPath $templatePath -PathType Leaf)) { return }

    $template = Read-NormalizedText $templatePath
    foreach ($requiredToken in @(
        'id="lang-pl"',
        'id="lang-en"',
        'id="search"',
        'id="nav"',
        "const state = { lang: 'pl'",
        "document.getElementById('lang-pl')",
        "document.getElementById('lang-en')"
    )) {
        if (-not $template.Contains($requiredToken)) {
            Add-DocError $Errors "BROWSER" "Browser template is missing required offline/UI contract token: $requiredToken"
        }
    }

    if ($template -match '(?i)(src|href)\s*=\s*["'']https?://') {
        Add-DocError $Errors "BROWSER" "Browser template contains an external HTTP(S) resource dependency."
    }
    if ($template -match '(?i)\b(fetch|XMLHttpRequest)\s*\(') {
        Add-DocError $Errors "BROWSER" "Browser template contains runtime network-loading code."
    }
}

function Get-DocumentationErrors {
    param([Parameter(Mandatory=$true)][string]$RepoRoot)
    $errors = New-Object System.Collections.Generic.List[string]
    Test-RequiredStructure $RepoRoot $errors
    Test-MetadataAndPairs $RepoRoot $errors
    Test-LocalMarkdownLinks $RepoRoot $errors
    Test-DocumentationImpact $RepoRoot $errors
    Test-BrowserContract $RepoRoot $errors
    Test-BrowserFreshness $RepoRoot $errors
    return $errors
}


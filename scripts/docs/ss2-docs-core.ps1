
function Get-DocsRoot {
    if ($Root) {
        return [IO.Path]::GetFullPath($Root)
    }
    return [IO.Path]::GetFullPath((Join-Path $PSScriptRoot ".."))
}

function Read-NormalizedText {
    param([Parameter(Mandatory=$true)][string]$Path)
    $text = [IO.File]::ReadAllText($Path, [Text.Encoding]::UTF8)
    return $text.Replace("`r`n", "`n").Replace("`r", "`n")
}

function Read-CanonicalMarkdown {
    param([Parameter(Mandatory=$true)][string]$Path)
    return (Read-NormalizedText $Path).TrimEnd("`n")
}

function Write-Utf8NoBom {
    param(
        [Parameter(Mandatory=$true)][string]$Path,
        [Parameter(Mandatory=$true)][string]$Text
    )
    $parent = Split-Path -Parent $Path
    if ($parent -and -not (Test-Path -LiteralPath $parent)) {
        New-Item -ItemType Directory -Path $parent -Force | Out-Null
    }
    $utf8 = New-Object System.Text.UTF8Encoding($false)
    [IO.File]::WriteAllText($Path, $Text, $utf8)
}

function Get-RelativeRepoPath {
    param(
        [Parameter(Mandatory=$true)][string]$RepoRoot,
        [Parameter(Mandatory=$true)][string]$Path
    )
    $rootFull = [IO.Path]::GetFullPath($RepoRoot).TrimEnd('\','/')
    $pathFull = [IO.Path]::GetFullPath($Path)
    if (-not $pathFull.StartsWith($rootFull, [StringComparison]::OrdinalIgnoreCase)) {
        throw "Path is outside repository root: $Path"
    }
    return $pathFull.Substring($rootFull.Length).TrimStart('\','/').Replace('\','/')
}

function Get-DocMetadata {
    param([Parameter(Mandatory=$true)][string]$Path)
    $text = Read-CanonicalMarkdown $Path
    $idMatch = [regex]::Match($text, '(?m)^<!--\s*doc-id:\s*([A-Za-z0-9._-]+)\s*-->\s*$')
    $kindMatch = [regex]::Match($text, '(?m)^<!--\s*document-kind:\s*([A-Za-z0-9._-]+)\s*-->\s*$')
    $sections = @([regex]::Matches($text, '(?m)^<!--\s*section-id:\s*([A-Za-z0-9._-]+)\s*-->\s*$') | ForEach-Object { $_.Groups[1].Value })
    return @{
        Text = $text
        DocId = if ($idMatch.Success) { $idMatch.Groups[1].Value } else { "" }
        Kind = if ($kindMatch.Success) { $kindMatch.Groups[1].Value } else { "" }
        Sections = $sections
    }
}

function Get-CanonicalDocPaths {
    param([Parameter(Mandatory=$true)][string]$RepoRoot)
    $paths = @()
    foreach ($dir in @(
        "docs\\internal",
        "docs\\product\\pl",
        "docs\\product\\en"
    )) {
        $full = Join-Path $RepoRoot $dir
        if (Test-Path -LiteralPath $full) {
            $paths += Get-ChildItem -LiteralPath $full -Filter "*.md" -File | ForEach-Object { $_.FullName }
        }
    }
    return @($paths | Sort-Object { (Get-RelativeRepoPath $RepoRoot $_).ToLowerInvariant() })
}

function Get-DocLanguage {
    param(
        [Parameter(Mandatory=$true)][string]$RepoRoot,
        [Parameter(Mandatory=$true)][string]$Path,
        [Parameter(Mandatory=$true)][string]$Kind
    )
    $rel = Get-RelativeRepoPath $RepoRoot $Path
    if ($Kind -eq "product") {
        if ($rel.StartsWith("docs/product/pl/", [StringComparison]::OrdinalIgnoreCase)) { return "pl" }
        if ($rel.StartsWith("docs/product/en/", [StringComparison]::OrdinalIgnoreCase)) { return "en" }
    }
    return "en"
}

function Escape-HtmlAttribute {
    param([Parameter(Mandatory=$true)][string]$Text)
    return [Net.WebUtility]::HtmlEncode($Text)
}

function New-BrowserHtml {
    param([Parameter(Mandatory=$true)][string]$RepoRoot)

    $templatePath = Join-Path $RepoRoot "docs\\browser\\template.html"
    $template = Read-NormalizedText $templatePath
    if (-not $template.Contains("@@DOC_SOURCES@@")) {
        throw "Documentation Browser template is missing @@DOC_SOURCES@@ placeholder."
    }

    $blocks = New-Object System.Collections.Generic.List[string]
    foreach ($path in Get-CanonicalDocPaths $RepoRoot) {
        $meta = Get-DocMetadata $path
        $rel = Get-RelativeRepoPath $RepoRoot $path
        $lang = Get-DocLanguage $RepoRoot $path $meta.Kind
        $id = Escape-HtmlAttribute $meta.DocId
        $kind = Escape-HtmlAttribute $meta.Kind
        $langAttr = Escape-HtmlAttribute $lang
        $pathAttr = Escape-HtmlAttribute $rel
        $markdown = $meta.Text
        $blocks.Add("<script type=`"text/plain`" class=`"doc-source`" data-doc-id=`"$id`" data-kind=`"$kind`" data-lang=`"$langAttr`" data-path=`"$pathAttr`">`n$markdown`n</script>")
    }

    $sources = [string]::Join("`n", $blocks)
    $html = $template.Replace("@@DOC_SOURCES@@", $sources)
    return $html.TrimEnd("`n") + "`n"
}

function Write-Browser {
    param([Parameter(Mandatory=$true)][string]$RepoRoot)
    $target = Join-Path $RepoRoot "docs\\browser\\index.html"
    Write-Utf8NoBom $target (New-BrowserHtml $RepoRoot)
    Write-Host "[DOCS] generated docs/browser/index.html"
}

function Add-DocError {
    param(
        [Parameter(Mandatory=$true)]$Errors,
        [Parameter(Mandatory=$true)][string]$Code,
        [Parameter(Mandatory=$true)][string]$Message
    )
    $Errors.Add("[$Code] $Message")
}


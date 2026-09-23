function New-SelfTestFixture {
    $fixture = Join-Path ([IO.Path]::GetTempPath()) ("ss2-docs-" + [Guid]::NewGuid().ToString("N"))
    foreach ($dir in @(
        "governance","work","scripts","docs\\internal","docs\\product\\pl","docs\\product\\en","docs\\browser"
    )) {
        New-Item -ItemType Directory -Path (Join-Path $fixture $dir) -Force | Out-Null
    }

    Write-Utf8NoBom (Join-Path $fixture "governance\\DOCUMENTATION.md") "# rule`n"
    Write-Utf8NoBom (Join-Path $fixture "docs\\browser\\template.html") "<html><body>@@DOC_SOURCES@@</body></html>`n"

    $internalFiles = @(
        "OVERVIEW.md","PROJECT_PLATFORM.md","APPLICATION_LIFECYCLE.md","PERSISTENCE.md","UI_PROJECT_HUB.md","BUILD_AND_TEST.md"
    )
    $counter = 0
    foreach ($name in $internalFiles) {
        ++$counter
        $body = "# Internal $counter`n`n<!-- doc-id: internal.test$counter -->`n<!-- document-kind: internal -->`n`n<!-- section-id: internal.test$counter.one -->`n## One`n`nFixture.`n"
        Write-Utf8NoBom (Join-Path $fixture ("docs\\internal\\" + $name)) $body
    }

    foreach ($lang in @("pl","en")) {
        $overview = "# Overview`n`n<!-- doc-id: product.overview -->`n<!-- document-kind: product -->`n`n<!-- section-id: product.overview.one -->`n## One`n`nFixture.`n"
        $projects = "# Projects`n`n<!-- doc-id: product.projects -->`n<!-- document-kind: product -->`n`n<!-- section-id: product.projects.one -->`n## One`n`nFixture.`n"
        Write-Utf8NoBom (Join-Path $fixture ("docs\\product\\" + $lang + "\\OVERVIEW.md")) $overview
        Write-Utf8NoBom (Join-Path $fixture ("docs\\product\\" + $lang + "\\PROJECTS.md")) $projects
    }

    $contract = "# TEST`n`n## Documentation impact`n`nInternal docs: not required`nUser/Product docs: not required`nReason: fixture`n"
    Write-Utf8NoBom (Join-Path $fixture "work\\TEST.md") $contract
    Write-Utf8NoBom (Join-Path $fixture "work\\ACTIVE.yaml") "version: 1`nstatus: active`nactive_work: `"work/TEST.md`"`n"
    Write-Browser $fixture
    return $fixture
}

function Assert-SelfTestFailure {
    param(
        [Parameter(Mandatory=$true)][string]$Name,
        [Parameter(Mandatory=$true)][string]$ExpectedCode,
        [Parameter(Mandatory=$true)][scriptblock]$Mutate
    )
    $fixture = New-SelfTestFixture
    try {
        & $Mutate $fixture
        $errors = Get-DocumentationErrors $fixture
        if (-not ($errors | Where-Object { $_.StartsWith("[$ExpectedCode]") })) {
            throw "Documentation self-test '$Name' did not produce expected [$ExpectedCode] failure. Errors: $($errors -join '; ')"
        }
        Write-Host "[DOCS SELFTEST OK] $Name -> [$ExpectedCode]"
    } finally {
        Remove-Item -LiteralPath $fixture -Recurse -Force -ErrorAction SilentlyContinue
    }
}

function Invoke-DocumentationSelfTest {
    $fixture = New-SelfTestFixture
    try {
        $baseline = Get-DocumentationErrors $fixture
        if ($baseline.Count -ne 0) {
            throw "Documentation self-test baseline is invalid: $($baseline -join '; ')"
        }
        Write-Host "[DOCS SELFTEST OK] valid baseline"
    } finally {
        Remove-Item -LiteralPath $fixture -Recurse -Force -ErrorAction SilentlyContinue
    }

    Assert-SelfTestFailure "missing bilingual pair" "PAIR" {
        param($r)
        Remove-Item -LiteralPath (Join-Path $r "docs\\product\\en\\PROJECTS.md") -Force
    }

    Assert-SelfTestFailure "section parity mismatch" "SECTION" {
        param($r)
        $p = Join-Path $r "docs\\product\\en\\PROJECTS.md"
        $t = Read-NormalizedText $p
        Write-Utf8NoBom $p ($t.Replace("product.projects.one", "product.projects.different"))
    }

    Assert-SelfTestFailure "broken local Markdown link" "LINK" {
        param($r)
        $p = Join-Path $r "docs\\product\\pl\\OVERVIEW.md"
        $t = Read-NormalizedText $p
        Write-Utf8NoBom $p ($t + "`n[broken](MISSING.md)`n")
    }

    Assert-SelfTestFailure "missing Documentation Impact" "IMPACT" {
        param($r)
        Write-Utf8NoBom (Join-Path $r "work\\TEST.md") "# TEST`n"
    }

    Assert-SelfTestFailure "stale generated Browser" "BROWSER" {
        param($r)
        $p = Join-Path $r "docs\\product\\pl\\OVERVIEW.md"
        $t = Read-NormalizedText $p
        Write-Utf8NoBom $p ($t + "`nChanged after Browser generation.`n")
    }
}

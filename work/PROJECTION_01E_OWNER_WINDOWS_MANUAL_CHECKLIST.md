# PG-01E — Owner Windows practical acceptance checklist (PREPARED / UNRUN)

**Status:** WORKING CHECKLIST, NOT OWNER PASS. **Date:** 2026-10-13. **Active Contract:** `work/PROJECTION_01E_PRODUCT_CLOSEOUT_WORK_CONTRACT.md` (PG-01E E0–E6). **Program:** Part roadmap v1.32. This checklist does not widen the accepted Project Geometry surface or replace exact-head Windows FOCUSED/FAST/FULL.

## 1. Traceable test session (Owner fills before execution)

- Windows OS / build architecture / GPU / OCCT environment: **UNRECORDED**.
- Exact tested executable/PR HEAD SHA (not `main` unless that binary was built from `main`): **UNRECORDED**.
- Build configuration and path: **UNRECORDED**.
- New test Part and `.ss2part` file path (created using current modeling semantics v2): **UNRECORDED**.
- Screenshots, relevant CTest native log/run IDs, and any typed error messages: **UNRECORDED**.
- Owner observed practical result: **NOT TESTED**. Date/time: **UNRECORDED**.

Do not use an old modeling-v1 Part as the positive authoring fixture. Reuse of the original legacy document is only for **read-only typed refusal** after preserving a byte-identical backup. Manual inspection cannot alone prove a fresh OCCT provider generation; match it to separate native automated evidence.

## 2. One reproducible supported Windows scenario

Execute in the native Windows CAD Workbench; note **PASS / FAIL / NOT APPLICABLE** and actual observations for every applicable row. Prepare a **new v2 Part** containing an earlier-stage ordinary Extrude Add body. To exercise supported Face outer/inner boundaries, add a separate current-stage body with a bounded Face with a hole using supported Add/Cut commands. Complex variants may be separate new Parts; never turn unavailable supported input into a fabricated result.

| ID | Action / accepted precondition | Expected observable result | Owner observation |
| --- | --- | --- | --- |
| W01 | Create a new Part, Sketch/Profile and **Extrude Add**, save as native Part; reopen it once | New Part is saved/reopened with native v16 and modeling-v2 semantics; material Body exists | UNRUN |
| W02 | Enter Sketch Edit supported on an Origin plane or current planar Body Face/Datum Plane; open **Project Geometry** from References or `PROJECT` | One Operations panel with modes **Edges / Planar Face / Face Boundary**, Regular/Construction and Finish/Cancel | UNRUN |
| W03 | Choose **Edges**, click a uniquely supported material Edge from a valid earlier Body stage; stage a second via **Ctrl + Left click** | Two current semantic material sources, exact projected Line/Circle/Arc images where applicable; no stale/approximate geometry | UNRUN |
| W04 | Switch between Edges and Face modes without committing; return to Edges, select **Regular**, Finish | Mode switch preserves already staged eligible sources; Finish authors all eligible targets in **one transaction**, with no Face dependency | UNRUN |
| W05 | Undo once, Redo once; select authored linked Sketch entities | Atomic undo/re-creation and stable linked identity/role; no copied seed treated as current geometry | UNRUN |
| W06 | Use existing Regular linked geometry to form a supported closed **Profile** and a downstream **Cut** Feature | Supported chain evaluates from the exact earlier Body stage; the later Feature is valid, not silently rebound to nearest geometry | UNRUN |
| W07 | Edit a supported upstream dimension/line and recompute the linked consumer | Dependent geometry updates from current resolved upstream source; if the Profile becomes nonclosed, later Feature fails visibly rather than consuming a stale seed | UNRUN |
| W08 | Restore a valid contour through supported explicit Sketch edits/repair and reevaluate | Same authored Profile/Feature intent is restored where supported; no automatic contour healing or silent source reassignment | UNRUN |
| W09 | Select currently resolved linked targets, invoke **Break Link**, then Undo/Redo | One atomic detach with preserved current geometry, EntityIds and roles; mixed invalid/unresolved batch does not partially detach | UNRUN |
| W10 | With suitable bounded **planar Face** (prefer outer boundary plus inner hole), enter **Planar Face** and select it | Exactly one bounded planar Face acquisition; outer and inner material boundary Edge uses; no durable persisted Face token | UNRUN |
| W11 | Stage the same material Edge manually in Edges mode and via Planar Face; Finish | Exact material Edge identity deduplicated once, without dedup by merely equal coordinates | UNRUN |
| W12 | Enter **Face Boundary** and manually select more than one bounded Face, including nonplanar only where genuinely available | Only selected Faces' individual outer/inner material Edge members included; no automatic neighboring-Face traversal; certified seam/partition artifacts excluded | UNRUN |
| W13 | For a fixture with an independently unsupported/degenerate **exact projection** member, inspect Operations and Viewer | Per-Face wire member typed `SKIPPED — geometric Unsupported/Degenerate`, local red source overlay; other supported members may remain PARTIAL; no ellipse/polyline approximation | UNRUN |
| W14 | In the same supported fixture trigger **zero supported** projection members, then attempt Finish | Finish disabled / typed `FINISH` rejected; no authored partial Sketch mutation; Clear/Cancel/Esc remain usable | UNRUN |
| W15 | Trigger a rejected stale-stage/provider or ambiguous Face/Edge identity gesture (only when reproducible from actual supported interaction) | Entire invalid gesture/Finish rejects; not converted into geometric skip; previously valid staged sources not silently destroyed | UNRUN |
| W16 | Repeat a valid Finish; Save, Close and Reopen the v2 Part and re-evaluate with actual new runtime provider | Durable links re-evaluate; separate automated evidence verifies provider freshness and stable semantic entity IDs; no saved OCCT/Viewer Face identities | UNRUN |
| W17 | Suppress or delete a referenced earlier source, then inspect later linked consumer and try Break Link | Broken source stays typed unavailable/unresolved; no fallback to saved seed, no silent retarget; unresolved Break Link refuses atomically | UNRUN |
| W18 | Invoke Cancel and check hierarchical Esc/Command Line behavior on a fresh Project Geometry draft | No authored mutation from Cancel; first Esc prioritizes active text/input/staging, later Esc exits according to current tool grammar | UNRUN |
| W19 | Narrow right column, inspect modes/controls; attempt concurrent Feature Edit and Sketch/Profile Tree Edit | Controls stack/wrap without clipped actionable labels, edit exclusivity preserves one valid context (UI-01 accepted behavior) | UNRUN |
| W20 | Observe exact supported v16/modeling-v2 cold Save/Reopen; separately open an **untouched** original modeling-v1 document, when provided | New v2 Part works; original v1 rejected with typed unsupported policy; no auto-migration or original-file rewrite. Record pre/post file hash separately | UNRUN |

For W06–W08, actual closed profile construction is a **prerequisite**; a deliberately open/partial Face Boundary Sketch does **not** automatically form a Profile. If a subcase requires an unsupported production feature or a source beyond the accepted Part stage, mark it **NOT APPLICABLE / CONTRACT GAP**, not as a false PASS or invitation to implement a new mode.

## 3. Resource and performance evidence (E3 — no invented thresholds)

**Already verified by automated Windows tests; NOT an Owner practical session:** real registered native Qt/OCCT and Kernel work at exact test-source SHA 6755949f8f569d6d7f9a23116b67d1f06e065eb5: [FULL #2804](https://github.com/MechanicalCave/SimpleSolid2/actions/runs/38362416344) **199/199 PASS**, including [#2802](https://github.com/MechanicalCave/SimpleSolid2/actions/runs/38360944629) native multi-Face source-linked Extrude Add cold replay with **2 warm-up + 8 measured** loads/new OCCT providers. Existing test instrumentation writes actual per-cycle min/mean/max microseconds, unchanged file bytes and eight post-teardown Windows private-committed-byte snapshots to the #2802 job summary. Independently, the Kernel linked Revolve cold case reports microseconds and native package size in [FOCUSED #2794](https://github.com/MechanicalCave/SimpleSolid2/actions/runs/38358056855). No PM-06 budget, no native-handle series, no proof of absence of a leak.

**Owner practical E3 remains UNRUN.** Record actual Windows edition/build, CPU/RAM/GPU, desktop Release executable and Git HEAD, saved native v16/modeling-v2 fixture size, material Edge/Face/hole counts, exactly how memory and handles were sampled, and warm-up/repetition counts. Keep stdout times distinct from CTest total time. A positive short-run result must not be mistaken for a stable long-run resource trend.

To reproduce the existing registered CTests with full passing stdout, use an **already correctly built** native Windows Release test directory and the matching executable SHA. These are CTest invocations, not build/configuration commands:

~~~powershell
# Run inside the actual kernel-native Release CTest build directory:
ctest -C Release -R '^pm05e3\.edge_feature_cold_rebuild$' -V

# Separately run inside the actual Desktop / Qt Release CTest build directory:
ctest -C Release -R '^pg01d\.native_face_ui$' -V
~~~

The native kernel source prints PG01E_E3_NATIVE_COLD_REPLAY_MEASURED; the desktop source prints PG01E_E3_WINDOWS_MULTI_FACE_REPLAY. Preserve the original textual logs and record each observed min/mean/max and package size **as measured**; if not available, write UNAVAILABLE. Successful default (non-verbose) CTest logs suppress individual test stdout, so values must not be back-calculated from whole-suite time.

For an independent Owner machine observation, run repeated actual Project Geometry Edit/Save/Close/Reopen sessions on a declared, unchanged fixture and sample process private bytes, working set and handles with a named Windows system tool. Take baseline, post-warm-up and post-cycle measurements, report repetitions/intervals and any reproducible resource growth. A single sample pair or eight-cycle result cannot establish a no-leak conclusion. Do not assume that all supported legacy files can migrate: native modeling-v1 is **strictly refused**, with the original kept byte-identical.

**Owner setup/environment:** UNRECORDED. **Owner-collected timing/private-byte/handle series:** UNRUN. **Owner trend assessment:** OPEN.

## 4. Strict final acceptance and STOP

- **No individual Owner observations have been supplied for this PG-01E practical run.** W01–W20 all remain UNRUN. Historical PG-01C/PG-01D practical PASS and automated #2804 FULL **do not fill manual rows**.
- Require the tested local executable/PR HEAD to match its recorded source and fresh provider evidence; distinguish historical #2804 (before this documentation change) from any new exact-head FULL. The separate earlier pm04e2 ColdRevolveKernel **test double** never proves native OCCT behavior.
- Record each supported case as PASS / FAIL / NOT APPLICABLE with actual notes, fixture references and typed diagnostic. Unsupported fixture attempts are **CONTRACT GAP**, not PASS and not a reason for silent new UI/features.
- PG-01E Owner FINAL PASS needs an explicit separate positive decision after the evidence/gap review. **Separate affirmative authorization to squash merge PR #341 is also mandatory**.
- Any required mutation of product src, schema, APIs, CI/CMake, foreign tests or fixtures is STOP, requiring an independent bounded Owner D2 amendment. No RG-02A/G1 Vertex UI, PM-06 or unrelated work.

## Documentation impact

Internal docs: not required
User/Product docs: not required
Reason: This named work checklist records how Owner should independently observe existing, previously accepted Project Geometry behavior and resource trends. It changes no canonical as-built page, product behavior or generated Browser.

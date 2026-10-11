# PG-01D — owner-supplied same-domain Add reproduction fixtures

**Purpose:** isolate unwanted bounded Face/Edge partitions after ordinary
Extrude Add and Revolve Add. These are original native SS2 v16 documents,
provided by the Owner and committed byte-for-byte without rewriting their
document IDs, Sketch EntityIds, feature history or projection bindings.

| Fixture | Native history | Owner's UI observation |
| --- | --- | --- |
| `pg01d_same_domain_extrude_add_v16.ss2part` | Extrude001 Add (10 mm), Extrude002 Add (10 mm) | One geometrically continuous upper plane is selectable as **2** bounded Faces after Extrude002 |
| `pg01d_same_domain_extrude_revolve_add_v16.ss2part` | The same two Extrude Adds, then Revolve003 Add (one-sided 2π / 360°) | The upper planar region is selectable as **4** bounded Faces |

Both documents have a body-planar-surface-supported second Sketch with four
projected Edge bindings. The Revolve variant also contains a third Sketch
(with an authored Sketch-line axis and one projected Edge binding).
They are separate documents, not assumed to share UUIDs.

## Intended characterization and acceptance gates

1. Re-evaluate **each exact Feature stage** in a fresh OCCT provider.
   Distinguish native B-Rep bounded Faces from SS2 semantic Surface
   addresses and the viewer's selected Face count; record all three.
   The **2** and **4** numbers above are Owner UI observations, **not**
   a verified native evaluator assertion yet.
2. Demonstrate which shared same-domain planar Face boundaries are
   artificial after Extrude002, then after Revolve003. Preserve real
   intersection edges, cylindrical seams where required by B-Rep and holes.
3. A later approved refinement must produce a valid single solid with
   unchanged exterior geometry and volume, with no unwanted internal
   same-domain split, **without** choosing arbitrary Curve/Face producers.
4. Certify semantic Surface/Curve/Point provenance and the existing linked
   Sketch EntityIds through edits, Undo/Redo and cold Save/Reopen. When a
   unique persistent source cannot be proven, fail closed.
5. **No Chamfer or Fillet** belongs to the intended reproducer or its
   production implementation scope. These fixtures do **not** prove the
   previously implemented D2-V16 one-Point reference case.

The files are regression inputs, not a claim that current SS2 passes these
new geometric expectations. PR #309 remains Draft; the earlier exact-head
FULL CI result does not replace Owner practical FINAL PASS.

## Provenance

Originally supplied as `Test_face_extrude_01.ss2part` and
`Test_face_extrude_revolve01.ss2part`. Only repository filenames changed.

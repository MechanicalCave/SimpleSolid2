# SimpleSolid 2.0 Product Documentation

<!-- doc-id: product.overview -->
<!-- document-kind: product -->

<!-- section-id: product.overview.purpose -->
## About this documentation

This documentation describes the **current accepted product state** of SimpleSolid 2.0.

It is written for understanding and using the application. It does not describe implementation history, active development work, roadmap intent, C++ class structure or CI evidence.

<!-- section-id: product.overview.current -->
## What is currently available

The current product provides:

- creating and opening Projects;
- stable Project identity and Recent Projects;
- moving a Project and recovering its location with `Locate…`;
- creating and opening native `.ss2part` Part Documents;
- stable `DocumentId` independent of filename and path;
- editing Number, Title, Description and Engineering Revision;
- Undo / Redo for those changes;
- Save and safe closing with unsaved-change protection;
- rediscovering Parts after restart;
- identity-conflict detection when two files in one Workspace carry the same DocumentId.

The current Part is a durable Document but does not yet contain geometric modeling.

Sketch, Bodies/Features, Assembly, Drawing and Viewer are not yet part of the available product surface.

<!-- section-id: product.overview.browser -->
## Product Browser

The Product Browser is generated from canonical Markdown documents stored in the repository.

Polish and English are maintained as peer sources. The Browser opens product documentation in Polish by default and allows switching to English.

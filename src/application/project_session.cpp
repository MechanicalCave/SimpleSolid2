#include <simplesolid2/application/project_session.hpp>

#include <system_error>
#include <utility>

namespace simplesolid2::application {

ProjectSessionOpenResult ProjectSession::open(
    const std::filesystem::path& workspace_root) {
    ProjectWorkspaceMetadataService metadata_service;
    auto loaded = metadata_service.load(workspace_root);
    if (!loaded.ok()) {
        return {
            std::nullopt,
            ProjectSessionDiagnostic{
                ProjectSessionErrorCode::project_validation_failed,
                loaded.diagnostic.code,
                std::move(loaded.diagnostic.message),
                std::move(loaded.diagnostic.path),
            },
        };
    }

    std::error_code ec;
    auto canonical = std::filesystem::weakly_canonical(workspace_root, ec);
    if (ec || !std::filesystem::is_directory(canonical, ec) || ec) {
        return {
            std::nullopt,
            ProjectSessionDiagnostic{
                ProjectSessionErrorCode::workspace_resolution_failed,
                ProjectMetadataErrorCode::none,
                "Unable to resolve validated Project workspace root",
                workspace_root,
            },
        };
    }

    ProjectSession session{
        canonical.lexically_normal(),
        std::move(*loaded.metadata),
    };
    return {
        std::optional<ProjectSession>{std::move(session)},
        ProjectSessionDiagnostic{},
    };
}

} // namespace simplesolid2::application

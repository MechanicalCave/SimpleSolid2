#include "project_hub_window.hpp"

#include <QApplication>
#include <QCoreApplication>
#include <QDir>
#include <QStandardPaths>
#include <QString>
#include <QTimer>

#include <cstdlib>\n#include <filesystem>
#include <iostream>
#include <string>

namespace {

std::filesystem::path toFilesystemPath(const QString& value) {
#if defined(_WIN32)
    return std::filesystem::path{value.toStdWString()};
#else
    const auto bytes = value.toUtf8();
    std::u8string utf8;
    utf8.reserve(static_cast<std::size_t>(bytes.size()));
    for (const unsigned char ch : bytes) {
        utf8.push_back(static_cast<char8_t>(ch));
    }
    return std::filesystem::path{utf8};
#endif
}

} // namespace

int main(int argc, char* argv[]) {
    QApplication app{argc, argv};

    QCoreApplication::setOrganizationName(QStringLiteral("SimpleSolid"));
    QCoreApplication::setApplicationName(QStringLiteral("SimpleSolid2"));

    const bool smoke_test =
        QCoreApplication::arguments().contains(QStringLiteral("--smoke-test"));

    std::filesystem::path smoke_root;
    std::filesystem::path recent_catalog;

    if (smoke_test) {
        smoke_root =
            std::filesystem::temp_directory_path() /
            ("simplesolid2_app_smoke_" +
             std::to_string(QCoreApplication::applicationPid()));
        std::error_code ec;
        std::filesystem::remove_all(smoke_root, ec);
        std::filesystem::create_directories(smoke_root, ec);
        if (ec) {
            std::cerr << "Unable to create SimpleSolid2 smoke-test state directory\n";
            return EXIT_FAILURE;
        }
        recent_catalog = smoke_root / "recent-projects-v1.txt";
    } else {
        const auto state_root =
            QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
        if (state_root.isEmpty()) {
            std::cerr << "Unable to resolve SimpleSolid2 application state location\n";
            return EXIT_FAILURE;
        }
        recent_catalog =
            toFilesystemPath(QDir::cleanPath(state_root)) /
            "recent-projects-v1.txt";
    }

    simplesolid2::ui::ProjectHubWindow window{recent_catalog};
    window.show();

    if (smoke_test) {
        QTimer::singleShot(0, &app, &QApplication::quit);
    }

    const int result = app.exec();

    if (smoke_test) {
        std::error_code ec;
        std::filesystem::remove_all(smoke_root, ec);
    }

    return result;
}

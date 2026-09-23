#include "project_creation_dialog.hpp"

#include <QApplication>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QTest>

#include <cstdlib>
#include <filesystem>
#include <iostream>

using namespace simplesolid2::ui;

namespace {

void check(bool value, const char* expression, int line) {
    if (!value) {
        std::cerr << "PH-02C Project Creation dialog CHECK failed at line "
                  << line << ": " << expression << '\n';
        std::abort();
    }
}

#define CHECK(expr) check(static_cast<bool>(expr), #expr, __LINE__)

void verifyDialogFieldsAndIndependentFolder() {
    ProjectCreationDialog dialog;
    dialog.show();
    QApplication::processEvents();

    auto* name =
        dialog.findChild<QLineEdit*>("projectNameEdit");
    auto* location =
        dialog.findChild<QLineEdit*>("projectLocationEdit");
    auto* folder =
        dialog.findChild<QLineEdit*>("projectFolderEdit");
    auto* final_path =
        dialog.findChild<QLabel*>("projectFinalPathLabel");
    auto* create =
        dialog.findChild<QPushButton*>("createProjectConfirmButton");

    CHECK(name != nullptr);
    CHECK(location != nullptr);
    CHECK(folder != nullptr);
    CHECK(final_path != nullptr);
    CHECK(create != nullptr);

    CHECK(!name->isReadOnly());
    CHECK(!location->isReadOnly());
    CHECK(!folder->isReadOnly());
    CHECK(!create->isEnabled());

    QTest::keyClicks(name, "Hydraulic Press");
    QApplication::processEvents();

    CHECK(name->text() == QStringLiteral("Hydraulic Press"));
    CHECK(folder->text() == QStringLiteral("Hydraulic Press"));
    CHECK(dialog.projectName() == QStringLiteral("Hydraulic Press"));
    CHECK(dialog.projectFolder() == QStringLiteral("Hydraulic Press"));
    CHECK(!create->isEnabled());

    location->setText(QStringLiteral("D:/Projects"));
    QApplication::processEvents();

    CHECK(dialog.location() == QStringLiteral("D:/Projects"));
    CHECK(create->isEnabled());
    CHECK(final_path->text().contains(QStringLiteral("Hydraulic Press")));

    folder->selectAll();
    QTest::keyClicks(folder, "PressWorkspace");
    QApplication::processEvents();

    CHECK(folder->text() == QStringLiteral("PressWorkspace"));
    CHECK(dialog.projectFolder() == QStringLiteral("PressWorkspace"));

    name->selectAll();
    QTest::keyClicks(name, "Renamed Press");
    QApplication::processEvents();

    CHECK(name->text() == QStringLiteral("Renamed Press"));
    CHECK(folder->text() == QStringLiteral("PressWorkspace"));
    CHECK(dialog.projectName() == QStringLiteral("Renamed Press"));
    CHECK(dialog.projectFolder() == QStringLiteral("PressWorkspace"));
    CHECK(final_path->text().contains(QStringLiteral("PressWorkspace")));
    CHECK(create->isEnabled());
}

} // namespace

int main(int argc, char* argv[]) {
    QApplication app{argc, argv};
    verifyDialogFieldsAndIndependentFolder();
    return EXIT_SUCCESS;
}

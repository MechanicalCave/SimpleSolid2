#pragma once

#include <QListWidget>
#include <QString>

#include <string>

namespace simplesolid2::ui::internal {

inline constexpr int recentOpenableRole = Qt::UserRole + 1;
inline constexpr int recentAvailabilityRole = Qt::UserRole + 2;

inline bool hasSingleRecentSelection(const QListWidget& list) {
    return list.selectedItems().size() == 1;
}

inline bool selectedRecentCanOpen(const QListWidget& list) {
    const auto selected = list.selectedItems();
    if (selected.size() != 1) return false;
    return selected.front()->data(recentOpenableRole).toBool();
}

inline std::string selectedRecentProjectId(const QListWidget& list) {
    const auto selected = list.selectedItems();
    if (selected.size() != 1) return {};

    const auto bytes = selected.front()->data(Qt::UserRole).toString().toUtf8();
    return std::string{
        bytes.constData(),
        static_cast<std::size_t>(bytes.size()),
    };
}

} // namespace simplesolid2::ui::internal

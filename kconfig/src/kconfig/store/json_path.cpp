#include "../store.hpp"

#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace {

bool parsePathSegments(std::string_view path,
                       std::vector<std::pair<std::string, std::optional<std::size_t>>>& out) {
    out.clear();
    if (path.empty()) {
        return false;
    }

    std::size_t position = 0;
    while (position < path.size()) {
        const std::size_t dot = path.find('.', position);
        const bool lastSegment = (dot == std::string_view::npos);
        const std::string segment(path.substr(position, lastSegment ? std::string_view::npos : dot - position));
        if (segment.empty()) {
            return false;
        }

        std::string key = segment;
        std::optional<std::size_t> index;
        const auto bracketPos = segment.find('[');
        if (bracketPos != std::string::npos) {
            key = segment.substr(0, bracketPos);
            const auto closingPos = segment.find(']', bracketPos);
            if (closingPos == std::string::npos || closingPos != segment.size() - 1) {
                return false;
            }
            const std::string indexText = segment.substr(bracketPos + 1, closingPos - bracketPos - 1);
            if (indexText.empty()) {
                return false;
            }
            try {
                index = static_cast<std::size_t>(std::stoul(indexText));
            } catch (...) {
                return false;
            }
        }

        out.emplace_back(std::move(key), index);
        if (lastSegment) {
            break;
        }
        position = dot + 1;
    }

    return !out.empty();
}

} // namespace

namespace kconfig::store {

bool isRootPath(std::string_view path) {
    return path.empty() || path == ".";
}

const kconfig::json::Value* resolvePath(const kconfig::json::Value& root, std::string_view path) {
    if (path.empty()) {
        return &root;
    }

    const kconfig::json::Value* current = &root;
    std::size_t position = 0;

    while (position < path.size()) {
        const std::size_t dot = path.find('.', position);
        const bool lastSegment = (dot == std::string_view::npos);
        const std::string segment(path.substr(position, lastSegment ? std::string_view::npos : dot - position));
        if (segment.empty()) {
            return nullptr;
        }

        std::string key = segment;
        std::optional<std::size_t> arrayIndex;
        const auto bracketPos = segment.find('[');
        if (bracketPos != std::string::npos) {
            key = segment.substr(0, bracketPos);
            const auto closingPos = segment.find(']', bracketPos);
            if (closingPos == std::string::npos || closingPos != segment.size() - 1) {
                return nullptr;
            }
            const std::string indexText = segment.substr(bracketPos + 1, closingPos - bracketPos - 1);
            if (indexText.empty()) {
                return nullptr;
            }
            try {
                arrayIndex = static_cast<std::size_t>(std::stoul(indexText));
            } catch (...) {
                return nullptr;
            }
        }

        if (!key.empty()) {
            if (!current->is_object()) {
                return nullptr;
            }
            const auto it = current->find(key);
            if (it == current->end()) {
                return nullptr;
            }
            current = &(*it);
        }

        if (arrayIndex.has_value()) {
            if (!current->is_array() || *arrayIndex >= current->size()) {
                return nullptr;
            }
            current = &((*current)[*arrayIndex]);
        }

        if (lastSegment) {
            break;
        }

        position = dot + 1;
    }

    return current;
}

bool setValueAtPath(kconfig::json::Value& root, std::string_view path, kconfig::json::Value value) {
    std::vector<std::pair<std::string, std::optional<std::size_t>>> segments;
    if (!parsePathSegments(path, segments)) {
        return false;
    }

    if (!root.is_object()) {
        root = kconfig::json::Object();
    }

    kconfig::json::Value* current = &root;
    for (std::size_t i = 0; i < segments.size(); ++i) {
        const auto& [key, index] = segments[i];
        const bool last = (i == segments.size() - 1);

        if (!key.empty()) {
            if (!current->is_object()) {
                *current = kconfig::json::Object();
            }
            if (last && !index.has_value()) {
                (*current)[key] = std::move(value);
                return true;
            }
            if (!current->contains(key)) {
                (*current)[key] = index.has_value() ? kconfig::json::Array() : kconfig::json::Object();
            }
            current = &(*current)[key];
        }

        if (index.has_value()) {
            if (!current->is_array()) {
                *current = kconfig::json::Array();
            }
            while (current->size() <= *index) {
                current->push_back(nullptr);
            }
            if (last) {
                (*current)[*index] = std::move(value);
                return true;
            }
            current = &(*current)[*index];
        }
    }

    return false;
}

bool eraseValueAtPath(kconfig::json::Value& root, std::string_view path) {
    std::vector<std::pair<std::string, std::optional<std::size_t>>> segments;
    if (!parsePathSegments(path, segments)) {
        return false;
    }

    kconfig::json::Value* current = &root;
    for (std::size_t i = 0; i < segments.size(); ++i) {
        const auto& [key, index] = segments[i];
        const bool last = (i == segments.size() - 1);

        if (!key.empty()) {
            if (!current->is_object()) {
                return false;
            }
            auto it = current->find(key);
            if (it == current->end()) {
                return false;
            }
            if (last && !index.has_value()) {
                current->erase(key);
                return true;
            }
            current = &(*it);
        }

        if (index.has_value()) {
            if (!current->is_array() || *index >= current->size()) {
                return false;
            }
            if (last) {
                (*current)[*index] = nullptr;
                return true;
            }
            current = &(*current)[*index];
        }
    }

    return false;
}

} // namespace kconfig::store

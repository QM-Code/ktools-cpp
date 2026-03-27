#include "io.hpp"

#include "kconfig/trace.hpp"

#include <exception>
#include <fstream>
#include <system_error>

namespace kconfig::io {

std::filesystem::path Canonicalize(const std::filesystem::path& path) {
    std::error_code ec;
    auto result = std::filesystem::weakly_canonical(path, ec);
    if (!ec) {
        return result;
    }

    result = std::filesystem::absolute(path, ec);
    if (!ec) {
        return result;
    }

    return path;
}

JsonReadResult ReadJsonFile(const std::filesystem::path& path) {
    KTRACE("io", "read-json start path='{}'", path.string());
    if (!std::filesystem::exists(path)) {
        KTRACE("io", "read-json miss path='{}' reason='not found'", path.string());
        return {
            .json = std::nullopt,
            .error = JsonReadError::NotFound,
            .message = "file not found"
        };
    }

    std::ifstream stream(path);
    if (!stream) {
        KTRACE("io", "read-json failed path='{}' reason='open failed'", path.string());
        return {
            .json = std::nullopt,
            .error = JsonReadError::OpenFailed,
            .message = "failed to open file"
        };
    }

    try {
        kconfig::json::Value json;
        stream >> json;
        KTRACE("io", "read-json ok path='{}'", path.string());
        return {
            .json = std::move(json),
            .error = JsonReadError::None,
            .message = {}
        };
    } catch (const std::exception& e) {
        KTRACE("io", "read-json failed path='{}' reason='parse failed' detail='{}'",
                    path.string(),
                    e.what());
        return {
            .json = std::nullopt,
            .error = JsonReadError::ParseFailed,
            .message = e.what()
        };
    }
}

bool EnsureJsonObjectFile(const std::filesystem::path& path, std::string* error) {
    if (path.empty()) {
        KTRACE("io", "ensure-json-object failed path='<empty>' reason='empty path'");
        if (error) {
            *error = "path is empty";
        }
        return false;
    }

    KTRACE("io", "ensure-json-object start path='{}'", path.string());

    std::error_code dirEc;
    const auto parent = path.parent_path();
    if (!parent.empty()) {
        std::filesystem::create_directories(parent, dirEc);
        if (dirEc) {
            KTRACE("io", "ensure-json-object failed path='{}' reason='create parent failed' parent='{}' detail='{}'",
                        path.string(),
                        parent.string(),
                        dirEc.message());
            if (error) {
                *error = "failed to create directory " + parent.string() + ": " + dirEc.message();
            }
            return false;
        }
    }

    const auto writeDefaultObject = [&](std::ios_base::openmode mode, const char* reason) -> bool {
        KTRACE("io", "write-json-default start path='{}' reason='{}'",
                    path.string(),
                    reason);
        std::ofstream stream(path, mode);
        if (!stream) {
            KTRACE("io", "write-json-default failed path='{}' reason='open failed'",
                        path.string());
            if (error) {
                *error = "failed to open file for writing: " + path.string();
            }
            return false;
        }
        stream << "{}\n";
        if (!stream) {
            KTRACE("io", "write-json-default failed path='{}' reason='write failed'",
                        path.string());
            if (error) {
                *error = "failed to initialize json file: " + path.string();
            }
            return false;
        }
        KTRACE("io", "write-json-default ok path='{}'", path.string());
        return true;
    };

    if (!std::filesystem::exists(path)) {
        return writeDefaultObject(std::ios::out, "missing file");
    }

    if (std::filesystem::is_regular_file(path)) {
        std::error_code sizeEc;
        const auto fileSize = std::filesystem::file_size(path, sizeEc);
        if (!sizeEc && fileSize == 0) {
            return writeDefaultObject(std::ios::out | std::ios::trunc, "empty file");
        }
    }

    KTRACE("io", "ensure-json-object no-op path='{}' reason='existing file unchanged'", path.string());
    return true;
}

bool WriteJsonFile(const std::filesystem::path& path,
                   const kconfig::json::Value& json,
                   std::string* error) {
    if (path.empty()) {
        KTRACE("io", "write-json failed path='<empty>' reason='empty path'");
        if (error) {
            *error = "path is empty";
        }
        return false;
    }

    KTRACE("io", "write-json start path='{}'", path.string());

    std::error_code dirEc;
    const auto parent = path.parent_path();
    if (!parent.empty()) {
        std::filesystem::create_directories(parent, dirEc);
        if (dirEc) {
            KTRACE("io", "write-json failed path='{}' reason='create parent failed' parent='{}' detail='{}'",
                        path.string(),
                        parent.string(),
                        dirEc.message());
            if (error) {
                *error = "failed to create directory " + parent.string() + ": " + dirEc.message();
            }
            return false;
        }
    }

    std::ofstream stream(path, std::ios::trunc);
    if (!stream.is_open()) {
        KTRACE("io", "write-json failed path='{}' reason='open failed'",
                    path.string());
        if (error) {
            *error = "failed to open file for writing: " + path.string();
        }
        return false;
    }

    try {
        stream << json.dump(4) << '\n';
    } catch (const std::exception& e) {
        KTRACE("io", "write-json failed path='{}' reason='serialize/write failed' detail='{}'",
                    path.string(),
                    e.what());
        if (error) {
            *error = e.what();
        }
        return false;
    }
    KTRACE("io", "write-json ok path='{}'", path.string());
    return true;
}

void MergeJsonObjects(kconfig::json::Value& destination,
                      const kconfig::json::Value& source) {
    if (!destination.is_object() || !source.is_object()) {
        destination = source;
        return;
    }

    for (auto it = source.begin(); it != source.end(); ++it) {
        const auto& key = it.key();
        const auto& value = it.value();
        if (value.is_object() && destination.contains(key) && destination[key].is_object()) {
            MergeJsonObjects(destination[key], value);
        } else {
            destination[key] = value;
        }
    }
}

} // namespace kconfig::io

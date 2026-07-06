#pragma once

#include <optional>
#include <string>
#include <utility>

namespace shareaudio {

enum class ErrorCode {
    None,
    InvalidArgument,
    InvalidState,
    IoError,
    NetworkError,
    ProtocolError,
    AudioError,
    CodecError,
    ConfigError,
    Timeout,
    NotSupported
};

struct Error {
    ErrorCode code { ErrorCode::None };
    std::string message;

    [[nodiscard]] bool ok() const { return code == ErrorCode::None; }
};

inline Error ok_error()
{
    return {};
}

inline Error make_error(ErrorCode code, std::string message)
{
    return Error { code, std::move(message) };
}

template <typename T>
class Result {
public:
    static Result success(T value)
    {
        Result result;
        result.value_ = std::move(value);
        return result;
    }

    static Result failure(Error error)
    {
        Result result;
        result.error_ = std::move(error);
        return result;
    }

    [[nodiscard]] bool ok() const { return value_.has_value(); }
    [[nodiscard]] const Error& error() const { return error_; }
    [[nodiscard]] T& value() { return *value_; }
    [[nodiscard]] const T& value() const { return *value_; }

private:
    std::optional<T> value_;
    Error error_ { ErrorCode::None, {} };
};

template <>
class Result<void> {
public:
    static Result success()
    {
        return Result {};
    }

    static Result failure(Error error)
    {
        Result result;
        result.error_ = std::move(error);
        return result;
    }

    [[nodiscard]] bool ok() const { return error_.ok(); }
    [[nodiscard]] const Error& error() const { return error_; }

private:
    Error error_ { ErrorCode::None, {} };
};

} // namespace shareaudio

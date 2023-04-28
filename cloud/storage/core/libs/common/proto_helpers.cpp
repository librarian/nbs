#include "proto_helpers.h"

#include <library/cpp/protobuf/util/pb_io.h>

#include <google/protobuf/io/tokenizer.h>
#include <google/protobuf/messagext.h>
#include <google/protobuf/text_format.h>

#include <util/folder/path.h>
#include <util/stream/file.h>
#include <util/stream/str.h>

namespace NCloud {

namespace {

////////////////////////////////////////////////////////////////////////////////

struct TNullErrorCollector
    : public google::protobuf::io::ErrorCollector
{
    void AddError(
        int line,
        int column,
        const google::protobuf::string& message) override
    {
        Y_UNUSED(line);
        Y_UNUSED(column);
        Y_UNUSED(message);
    }
};

}   // namespace

////////////////////////////////////////////////////////////////////////////////

bool TryParseProtoTextFromStringWithoutError(
    const TString& text,
    google::protobuf::Message& dst)
{
    TStringInput in(text);
    NProtoBuf::io::TCopyingInputStreamAdaptor adaptor(&in);
    NProtoBuf::TextFormat::Parser parser;

    TNullErrorCollector nullErrorCollector;
    parser.RecordErrorsTo(&nullErrorCollector);

    if (!parser.Parse(&adaptor, &dst)) {
        // remove everything that may have been read
        dst.Clear();
        return false;
    }

    return true;
}

bool TryParseProtoTextFromString(
    const TString& text,
    google::protobuf::Message& dst)
{
    TStringInput in(text);
    return TryParseFromTextFormat(in, dst);
}

bool TryParseProtoTextFromFile(
    const TString& fileName,
    google::protobuf::Message& dst)
{
    if (!TFsPath(fileName).Exists()) {
        return false;
    }

    TFileInput in(fileName);
    return TryParseFromTextFormat(in, dst);
}

void ParseProtoTextFromString(
    const TString& text,
    google::protobuf::Message& dst)
{
    TStringInput in(text);
    ParseFromTextFormat(in, dst);
}

void ParseProtoTextFromFile(
    const TString& fileName,
    google::protobuf::Message& dst)
{
    TFileInput in(fileName);
    ParseFromTextFormat(in, dst);
}

void ParseProtoTextFromStringRobust(
    const TString& text,
    google::protobuf::Message& dst)
{
    TStringInput in(text);
    ParseFromTextFormat(in, dst, EParseFromTextFormatOption::AllowUnknownField);
}

void ParseProtoTextFromFileRobust(
    const TString& fileName,
    google::protobuf::Message& dst)
{
    TFileInput in(fileName);
    ParseFromTextFormat(in, dst, EParseFromTextFormatOption::AllowUnknownField);
}

}   // namespace NCloud

/**
 * @file src/anchor_emitter/anchor_emitter.cpp
 * @brief Extraction and emission of per-function retrieval anchors.
 * @copyright (c) 2020 Avast Software, licensed under the MIT license
 */

#include "retdec/anchor_emitter/anchor_emitter.h"

#include <fstream>
#include <set>
#include <utility>

#include <rapidjson/prettywriter.h>
#include <rapidjson/stringbuffer.h>

namespace retdec {
namespace anchor_emitter {

namespace {

const std::string JSON_functions     = "functions";
const std::string JSON_name          = "name";
const std::string JSON_demangledName = "demangledName";
const std::string JSON_linkType      = "linkType";
const std::string JSON_isExternal    = "isExternalCall";
const std::string JSON_constants     = "constants";
const std::string JSON_externalCalls = "externalCalls";

std::string linkTypeToString(common::Function::eLinkType t)
{
	switch (t)
	{
		case common::Function::STATICALLY_LINKED: return "staticallyLinked";
		case common::Function::DYNAMICALLY_LINKED: return "dynamicallyLinked";
		case common::Function::SYSCALL: return "syscall";
		case common::Function::IDIOM: return "idiom";
		case common::Function::DECOMPILER_DEFINED: return "internal";
		case common::Function::USER_DEFINED: return "internal";
		default: return "internal";
	}
}

/**
 * Pick the most informative symbol available for a function. Real (symbol
 * table) names are preferred; failing that the demangled name; failing that
 * RetDec's internal stable id.
 */
std::string pickName(const common::Function& f)
{
	const auto& real = f.getRealName();
	if (!real.empty())
	{
		return real;
	}
	auto demangled = f.getDemangledName();
	if (!demangled.empty())
	{
		return demangled;
	}
	return f.getName();
}

/**
 * A function counts as an "external call" anchor when it is a symbol the
 * binary references but does not define itself: a statically or dynamically
 * linked library function, or a syscall.
 */
bool isExternal(const common::Function& f)
{
	return f.isStaticallyLinked()
		|| f.isDynamicallyLinked()
		|| f.isSyscall();
}

} // anonymous namespace

AnchorSet extractAnchors(const config::Config& config)
{
	AnchorSet result;
	std::set<std::string> seenExternal;

	for (const auto& f : config.functions)
	{
		FunctionAnchors fa;
		fa.name = pickName(f);
		fa.demangledName = f.getDemangledName();
		fa.linkType = linkTypeToString(f.getLinkType());
		fa.isExternalCall = isExternal(f);
		fa.constants.assign(
				f.usedCryptoConstants.begin(),
				f.usedCryptoConstants.end()
		);

		if (fa.isExternalCall && !fa.name.empty()
				&& seenExternal.insert(fa.name).second)
		{
			result.externalCalls.push_back(fa.name);
		}

		result.functions.push_back(std::move(fa));
	}

	return result;
}

std::string serializeJson(const AnchorSet& anchors)
{
	rapidjson::StringBuffer buffer;
	rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(buffer);

	writer.StartObject();

	writer.String(JSON_functions);
	writer.StartArray();
	for (const auto& f : anchors.functions)
	{
		writer.StartObject();
		writer.String(JSON_name);
		writer.String(f.name);
		writer.String(JSON_demangledName);
		writer.String(f.demangledName);
		writer.String(JSON_linkType);
		writer.String(f.linkType);
		writer.String(JSON_isExternal);
		writer.Bool(f.isExternalCall);
		writer.String(JSON_constants);
		writer.StartArray();
		for (const auto& c : f.constants)
		{
			writer.String(c);
		}
		writer.EndArray();
		writer.EndObject();
	}
	writer.EndArray();

	writer.String(JSON_externalCalls);
	writer.StartArray();
	for (const auto& e : anchors.externalCalls)
	{
		writer.String(e);
	}
	writer.EndArray();

	writer.EndObject();
	return buffer.GetString();
}

std::string generateAnchorsJson(const config::Config& config)
{
	return serializeJson(extractAnchors(config));
}

std::string emitAnchorFile(
		const config::Config& config,
		const std::string& outputFile)
{
	if (outputFile.empty())
	{
		return std::string();
	}

	const std::string suffix = ".config.json";
	std::string outPath = outputFile;
	if (outPath.size() >= suffix.size()
			&& outPath.compare(outPath.size() - suffix.size(), suffix.size(), suffix) == 0)
	{
		outPath = outPath.substr(0, outPath.size() - suffix.size());
	}
	outPath += ".anchors.json";

	std::ofstream out(outPath);
	if (!out.is_open())
	{
		return std::string();
	}

	out << generateAnchorsJson(config);
	return outPath;
}

} // namespace anchor_emitter
} // namespace retdec

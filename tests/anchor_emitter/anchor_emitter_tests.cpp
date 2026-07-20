/**
 * @file tests/anchor_emitter/anchor_emitter_tests.cpp
 * @brief Tests for the @c anchor_emitter module.
 * @copyright (c) 2020 Avast Software, licensed under the MIT license
 */

#include <cstdio>
#include <fstream>
#include <sstream>
#include <string>

#include <gtest/gtest.h>
#include <rapidjson/document.h>

#include "retdec/anchor_emitter/anchor_emitter.h"
#include "retdec/common/function.h"
#include "retdec/config/config.h"

using namespace ::testing;

namespace retdec {
namespace anchor_emitter {
namespace tests {

class AnchorEmitterTests : public Test
{
	protected:
		config::Config config;
};

/**
 * Exercises the wiring end to end: a populated @c retdec::config::Config (the
 * existing call-site module) is fed through the emitter and the anchors the
 * paper relies on — function names, external calls, and constants — must show
 * up in the structured JSON.
 */
TEST_F(AnchorEmitterTests, ExtractsExternalCallNameAndConstantAnchors)
{
	// A dynamically linked import -> the paper's "external calls" anchor.
	common::Function import("printf");
	import.setRealName("printf");
	import.setIsDynamicallyLinked();
	config.functions.insert(import);

	// The binary's own code with a recognized constant.
	common::Function internal("sub_1000");
	internal.usedCryptoConstants.insert("AES_SBOX");
	config.functions.insert(internal);

	const auto json = generateAnchorsJson(config);

	rapidjson::Document doc;
	doc.Parse(json);
	ASSERT_FALSE(doc.HasParseError()) << "emitter produced invalid JSON";
	ASSERT_TRUE(doc.IsObject());

	// External-calls anchor: deduplicated imported symbol list.
	ASSERT_TRUE(doc.HasMember("externalCalls"));
	ASSERT_TRUE(doc["externalCalls"].IsArray());
	const auto& externalCalls = doc["externalCalls"].GetArray();
	ASSERT_EQ(1, externalCalls.Size());
	EXPECT_STREQ("printf", externalCalls[0].GetString());

	// Per-function anchors.
	ASSERT_TRUE(doc.HasMember("functions"));
	ASSERT_TRUE(doc["functions"].IsArray());
	const auto& functions = doc["functions"].GetArray();
	ASSERT_EQ(2, functions.Size());

	const rapidjson::Value* printfFn = nullptr;
	const rapidjson::Value* subFn = nullptr;
	for (const auto& f : functions)
	{
		const std::string name = f["name"].GetString();
		if (name == "printf")
		{
			printfFn = &f;
		}
		else if (name == "sub_1000")
		{
			subFn = &f;
		}
	}
	ASSERT_NE(nullptr, printfFn);
	ASSERT_NE(nullptr, subFn);

	EXPECT_TRUE((*printfFn)["isExternalCall"].GetBool());
	EXPECT_STREQ("dynamicallyLinked", (*printfFn)["linkType"].GetString());

	EXPECT_FALSE((*subFn)["isExternalCall"].GetBool());
	EXPECT_STREQ("internal", (*subFn)["linkType"].GetString());
	ASSERT_TRUE((*subFn)["constants"].IsArray());
	ASSERT_EQ(1, (*subFn)["constants"].GetArray().Size());
	EXPECT_STREQ("AES_SBOX", (*subFn)["constants"].GetArray()[0].GetString());
}

TEST_F(AnchorEmitterTests, EmptyConfigYieldsEmptyAnchorSet)
{
	const auto json = generateAnchorsJson(config);

	rapidjson::Document doc;
	doc.Parse(json);
	ASSERT_FALSE(doc.HasParseError());
	ASSERT_TRUE(doc.IsObject());
	EXPECT_TRUE(doc["functions"].GetArray().Empty());
	EXPECT_TRUE(doc["externalCalls"].GetArray().Empty());
}

/**
 * emitAnchorFile strips the ".config.json" suffix and writes ".anchors.json"
 * next to it, returning the path it wrote (the same wiring the decompiler
 * tool invokes at its call site).
 */
TEST_F(AnchorEmitterTests, EmitAnchorFileWritesJsonNextToConfig)
{
	common::Function import("fopen");
	import.setIsDynamicallyLinked();
	config.functions.insert(import);

	const std::string cfgPath = "anchor_emitter_test.config.json";
	const std::string expectedPath = "anchor_emitter_test.anchors.json";
	std::remove(expectedPath.c_str());

	const auto written = emitAnchorFile(config, cfgPath);
	EXPECT_EQ(expectedPath, written);

	std::ifstream in(expectedPath);
	ASSERT_TRUE(in.is_open());
	std::stringstream ss;
	ss << in.rdbuf();
	const auto content = ss.str();
	in.close();
	std::remove(expectedPath.c_str());

	EXPECT_NE(std::string::npos, content.find("\"fopen\""));
	EXPECT_NE(std::string::npos, content.find("externalCalls"));
}

TEST_F(AnchorEmitterTests, EmitAnchorFileSkipsEmptyPath)
{
	EXPECT_EQ(std::string(), emitAnchorFile(config, std::string()));
}

} // namespace tests
} // namespace anchor_emitter
} // namespace retdec

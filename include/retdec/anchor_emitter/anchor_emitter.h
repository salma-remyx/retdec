/**
 * @file include/retdec/anchor_emitter/anchor_emitter.h
 * @brief Extraction and emission of per-function retrieval anchors.
 * @copyright (c) 2020 Avast Software, licensed under the MIT license
 *
 * Anchors are the lightweight signals a binary-to-source retrieval system keys
 * on to find the original source function of a stripped binary function. The
 * concept and anchor taxonomy (function names, external calls, constants) come
 * from practical source code recovery via anchor-based retrieval:
 *
 *   "Practical Source Code Recovery from Binary Functions Using Anchor-Based
 *    Retrieval and LLM Reasoning", arXiv:2607.09452.
 *
 * This module implements only the anchor *extraction + structured emission*
 * half of that pipeline. RetDec already computes the underlying data (function
 * names, link kinds, recognized constants) while decompiling; this gathers it
 * into a single structured artifact. The retrieval database and LLM re-ranking
 * half of the paper is intentionally out of scope (downstream consumer).
 */

#ifndef RETDEC_ANCHOR_EMITTER_ANCHOR_EMITTER_H
#define RETDEC_ANCHOR_EMITTER_ANCHOR_EMITTER_H

#include <string>
#include <vector>

#include "retdec/common/function.h"
#include "retdec/config/config.h"

namespace retdec {
namespace anchor_emitter {

/**
 * Anchors gathered for a single decompiled function.
 *
 * - @c name / @c demangledName: the paper's "available function names" anchor.
 *   @c name is the most informative symbol available (real > demangled > id).
 * - @c isExternalCall: the paper's "external calls" anchor. True when the
 *   function is an imported / library / system symbol (statically or
 *   dynamically linked, or a syscall) rather than the binary's own code.
 * - @c linkType: human-readable form of the function's link kind.
 * - @c constants: the paper's "constants" anchor. Recognized named constants
 *   referenced by the function (e.g. crypto S-box identifiers).
 */
struct FunctionAnchors
{
	std::string name;
	std::string demangledName;
	std::string linkType;
	bool isExternalCall = false;
	std::vector<std::string> constants;
};

/**
 * All anchors extracted from one decompilation run.
 *
 * @c externalCalls is the deduplicated set of imported symbol names across the
 * whole binary (the union of every function's external-call anchor), provided
 * as a flat list for fast inverted-index ingestion by a retriever.
 */
struct AnchorSet
{
	std::vector<FunctionAnchors> functions;
	std::vector<std::string> externalCalls;
};

/**
 * Extract anchors from a populated decompilation @p config (i.e. one that has
 * already been run through the decompiler so its function table is filled).
 */
AnchorSet extractAnchors(const retdec::config::Config& config);

/**
 * Serialize an AnchorSet to the structured JSON shape a downstream
 * binary-to-source retriever consumes.
 */
std::string serializeJson(const AnchorSet& anchors);

/**
 * Convenience: extract anchors from @p config and serialize them to JSON.
 */
std::string generateAnchorsJson(const retdec::config::Config& config);

/**
 * Write the anchor JSON to \c <base>.anchors.json next to an existing output.
 *
 * @param config    Populated decompilation config.
 * @param outputFile  A path whose base name identifies the decompilation
 *                    output. If it ends in \c ".config.json" (the standard
 *                    RetDec config artifact) that suffix is replaced;
 *                    otherwise \c ".anchors.json" is simply appended. Pass an
 *                    empty string to skip emission.
 * @return The path written, or an empty string if nothing was written.
 */
std::string emitAnchorFile(
		const retdec::config::Config& config,
		const std::string& outputFile);

} // namespace anchor_emitter
} // namespace retdec

#endif

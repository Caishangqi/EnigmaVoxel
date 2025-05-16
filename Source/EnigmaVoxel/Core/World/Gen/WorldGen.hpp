#pragma once

struct FChunkHolder;

struct FWorldGen
{
	static void GenerateFullChunk(FChunkHolder& H);
	static void RebuildMesh(FChunkHolder& H);
};
